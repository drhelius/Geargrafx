/*
 * Geargrafx - PC Engine / TurboGrafx Emulator
 * Copyright (C) 2024  Ignacio Sanchez

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 *
 */

#include <fstream>
#include <sstream>
#include <algorithm>
#include "cdrom_cuebin_image.h"
#include "cdrom_common.h"
#include "crc.h"

CdRomCueBinImage::CdRomCueBinImage() : CdRomImage()
{

}

CdRomCueBinImage::~CdRomCueBinImage()
{
    DestroyImgFiles();
}

void CdRomCueBinImage::Init()
{
    CdRomImage::Init();
    Reset();
}

void CdRomCueBinImage::Reset()
{
    CdRomImage::Reset();
    DestroyImgFiles();
}

bool CdRomCueBinImage::LoadFromFile(const char* path, bool preload)
{
    using namespace std;

    Log("Loading CUE from %s...", path);

    if (!IsValidPointer(path))
    {
        Error("Invalid path %s", path);
        m_ready = false;
        return m_ready;
    }

    Reset();
    GatherPaths(path);

    if (strcmp(m_file_extension, "cue") != 0)
    {
        Error("Invalid file extension %s. Expected .cue", m_file_extension);
        m_ready = false;
        return m_ready;
    }

    ifstream file;
    open_ifstream_utf8(file, path, ios::in | ios::binary | ios::ate);

    if (file.is_open())
    {
        int size = (int)(file.tellg());

        if (size <= 0)
        {
            Error("Unable to open file %s. Size: %d", path, size);
            file.close();
            m_ready = false;
            return m_ready;
        }

        if (file.bad() || file.fail() || !file.good() || file.eof())
        {
            Error("Unable to open file %s. Bad file!", path);
            file.close();
            m_ready = false;
            return m_ready;
        }

        char* buffer = new char[size + 1];
        file.seekg(0, ios::beg);
        file.read(buffer, size);
        file.close();
        buffer[size] = 0;

        for (int i = 0; i < size; i++)
        {
            if (buffer[i] != 0)
                break;

            if (i == size - 1)
            {
                Error("File %s is empty!", path);
                SafeDeleteArray(buffer);
                m_ready = false;
                return m_ready;
            }
        }

        m_ready = ParseCueFile(buffer);

        SafeDeleteArray(buffer);

        if (preload && m_ready)
            m_ready = PreloadDisc();

        CalculateCRC();
    }
    else
    {
        Error("There was a problem loading the file %s...", path);
        m_ready = false;
    }

    if (!m_ready)
        Reset();

    return m_ready;
}

bool CdRomCueBinImage::ReadSector(u32 lba, u8* buffer)
{
    if (!m_ready || buffer == NULL)
    {
        Error("ReadSector failed - Media not ready or buffer is NULL");
        return false;
    }

    size_t track_count = m_toc.tracks.size();

    for (size_t i = 0; i < track_count; i++)
    {
        const Track& track = m_toc.tracks[i];
        const TrackFile& track_file = m_track_files[i];
        u32 sector_size = track.sector_size;
        u32 start = track.start_lba;
        u32 end = start + track.sector_count;

        if (lba >= start && lba < end)
        {
            u32 sector_offset = lba - start;
            ImgFile* img_file = track_file.img_file;

            if (img_file == NULL || img_file->file_size == 0)
            {
                Error("ReadSector failed - ImgFile is NULL or file size is 0");
                return false;
            }

            u32 byte_offset = track.file_offset + (sector_offset * sector_size);

            if (sector_size == 2352)
            {
                byte_offset += 16;
                sector_size = 2048;
            }

            if (byte_offset + sector_size > img_file->file_size)
            {
                Error("ReadSector failed - Byte offset %llu + sector size %d exceeds file size %d",
                    byte_offset, sector_size, img_file->file_size);
                return false;
            }

            m_current_sector = lba + 1;
            if (m_current_sector >= m_toc.sector_count)
                m_current_sector = m_toc.sector_count - 1;

            Debug("Reading sector %d from track %d (offset: %d)", lba, i, byte_offset);

            return ReadFromImgFile(img_file, byte_offset, buffer, sector_size);
        }
    }

    Error("ReadSector failed - LBA %d not found in any track", lba);

    return false;
}

bool CdRomCueBinImage::ReadSamples(u32 lba, u32 offset, s16* buffer, u32 count)
{
    if (!m_ready || buffer == NULL)
    {
        Error("ReadBytes failed - Media not ready or buffer is NULL");
        return false;
    }

    if (lba >= m_toc.sector_count)
    {
        Error("ReadBytes failed - LBA %d out of bounds (max: %d)", lba, m_toc.sector_count - 1);
        return false;
    }

    size_t track_count = m_toc.tracks.size();

    for (size_t i = 0; i < track_count; i++)
    {
        const Track& track = m_toc.tracks[i];
        const TrackFile& track_file = m_track_files[i];
        u32 sector_size = track.sector_size;
        u32 start = track.start_lba;
        u32 end = start + track.sector_count;

        if (lba >= start && lba < end)
        {
            u32 sector_offset = lba - start;
            ImgFile* img_file = track_file.img_file;

            if (img_file == NULL || img_file->file_size == 0)
            {
                Error("ReadBytes failed - ImgFile is NULL or file size is 0");
                return false;
            }

            u32 byte_offset = track.file_offset + (sector_offset * sector_size) + offset;
            u32 size = count * 2;

            if (byte_offset + size > img_file->file_size)
            {
                Error("ReadBytes failed - Byte offset %llu + size %d exceeds file size %d",
                    byte_offset, size, img_file->file_size);
                return false;
            }

            m_current_sector = lba;

            return ReadFromImgFile(img_file, byte_offset, (u8*)buffer, size);
        }
    }

    Error("ReadBytes failed - LBA %d not found in any track", lba);

    return false;
}

bool CdRomCueBinImage::PreloadDisc()
{
    Debug("Preloading all tracks...");

    size_t files_count = m_img_files.size();

    if (files_count == 0)
    {
        Error("No image files found to preload");
        return false;
    }

    for (size_t i = 0; i < files_count; i++)
    {
        ImgFile* img_file = m_img_files[i];

        if (!IsValidPointer(img_file))
        {
            Error("Invalid ImgFile pointer at index %u when preloading", i);
            return false;
        }

        if (!PreloadChunks(img_file, 0, img_file->chunk_count))
        {
            Error("Failed to preload chunks for ImgFile %s", img_file->file_path);
            return false;
        }
    }

    return true;
}

bool CdRomCueBinImage::PreloadTrack(u32 track_number)
{
    if (track_number >= m_toc.tracks.size())
    {
        Error("PreloadTrackChunks failed - Track number %d out of bounds (max: %d)", track_number, m_toc.tracks.size() - 1);
        return false;
    }

    const Track& track = m_toc.tracks[track_number];
    const TrackFile& track_file = m_track_files[track_number];

    u32 sector_size = track.sector_size;
    u32 start_offset = track.file_offset;
    u32 total_bytes = track.sector_count * sector_size;
    u32 start_chunk = start_offset / track_file.img_file->chunk_size;
    u32 chunks_needed = (total_bytes + track_file.img_file->chunk_size - 1) / track_file.img_file->chunk_size;

    Debug("Preloading all sectors for track %d (sectors: %d, bytes: %llu)", track_number, track.sector_count, total_bytes);

    return PreloadChunks(track_file.img_file, start_chunk, chunks_needed);
}

void CdRomCueBinImage::InitImgFile(ImgFile* img_file)
{
    img_file->file_name[0] = 0;
    img_file->file_path[0] = 0;
    img_file->file_size = 0;
    img_file->chunk_size = 0;
    img_file->chunk_count = 0;
    img_file->chunks = NULL;
    img_file->is_wav = false;
    img_file->wav_data_offset = 0;
}

void CdRomCueBinImage::InitParsedCueTrack(ParsedCueTrack& track)
{
    track.number = 0;
    track.type = GG_CDROM_AUDIO_TRACK;
    track.has_index0 = false;
    track.index0_lba = 0;
    track.has_pregap = false;
    track.pregap_length = 0;
    track.index1_lba = 0;
}

void CdRomCueBinImage::InitParsedCueFile(ParsedCueFile& cue_file)
{
    cue_file.img_file = NULL;
    cue_file.tracks.clear();
}

void CdRomCueBinImage::InitTrackFile(TrackFile& track_file)
{
    track_file.img_file = NULL;
}

void CdRomCueBinImage::DestroyImgFiles()
{
    int img_file_count = (int)(m_img_files.size());
    for (int i = 0; i < img_file_count; i++)
    {
        ImgFile* img_file = m_img_files[i];
        if (IsValidPointer(img_file))
        {
            if (IsValidPointer(img_file->chunks))
            {
                for (u32 j = 0; j < img_file->chunk_count; j++)
                    SafeDeleteArray(img_file->chunks[j]);
                SafeDeleteArray(img_file->chunks);
            }
            SafeDelete(img_file);
        }
    }
    m_img_files.clear();
    m_track_files.clear();
}

bool CdRomCueBinImage::GatherImgInfo(ImgFile* img_file)
{
    if (!IsValidPointer(img_file))
    {
        Error("Invalid ImgFile pointer");
        return false;
    }

    if (!IsValidPointer(img_file->file_path))
    {
        Error("Invalid file path in ImgFile");
        return false;
    }

    if (!ValidateFile(img_file->file_path))
        return false;

    if (!ProcessFileFormat(img_file))
        return false;

    SetupFileChunks(img_file);

    Debug("Gathered ImgFile info: %s", img_file->file_path);
    Debug("ImgFile info Size: %d, Chunk size: %d, Chunk count: %d", 
          img_file->file_size, img_file->chunk_size, img_file->chunk_count);

    return true;
}

bool CdRomCueBinImage::ValidateFile(const char* file_path)
{
    using namespace std;

    ifstream file;
    open_ifstream_utf8(file, file_path, ios::in | ios::binary | ios::ate);

    if (file.is_open())
    {
        int size = (int)(file.tellg());

        if (size <= 0)
        {
            Error("Unable to open file %s. Size: %d", file_path, size);
            file.close();
            return false;
        }

        if (file.bad() || file.fail() || !file.good() || file.eof())
        {
            Error("Unable to open file %s. Bad file!", file_path);
            file.close();
            return false;
        }

        file.close();
        return true;
    }

    Error("Unable to open file %s", file_path);
    return false;
}

bool CdRomCueBinImage::ProcessFileFormat(ImgFile* img_file)
{
    using namespace std;

    string file_path(img_file->file_path);
    string extension = file_path.substr(file_path.find_last_of(".") + 1);
    transform(extension.begin(), extension.end(), extension.begin(), (int(*)(int)) tolower);

    ifstream file;
    open_ifstream_utf8(file, img_file->file_path, ios::in | ios::binary | ios::ate);
    int size = (int)(file.tellg());
    file.close();

    img_file->file_size = (u32)size;

    if (extension == "wav")
        return ProcessWavFormat(img_file);

    return true;
}

bool CdRomCueBinImage::ProcessWavFormat(ImgFile* img_file)
{
    using namespace std;

    Debug("WAV file detected: %s", img_file->file_path);

    ifstream file;
    open_ifstream_utf8(file, img_file->file_path, ios::in | ios::binary);
    if (!file.is_open())
        return false;

    char header[44];
    file.read(header, 44);

    if (file.gcount() != 44)
    {
        Error("Failed to read WAV header from %s", img_file->file_path);
        file.close();
        return false;
    }

    if (strncmp(header, "RIFF", 4) != 0 || strncmp(header + 8, "WAVE", 4) != 0)
    {
        Error("Invalid WAV format in %s", img_file->file_path);
        file.close();
        return false;
    }

    u16 channels = *(u16*)(header + 22);
    u32 sample_rate = *(u32*)(header + 24);
    u16 bits_per_sample = *(u16*)(header + 34);

    if (sample_rate != 44100 || bits_per_sample != 16 || channels != 2)
    {
        Error("WAV file %s has incorrect format. Required: 44100Hz, 16-bit, stereo. Found: %dHz, %d-bit, %d channel(s)", img_file->file_path, sample_rate, bits_per_sample, channels);
        file.close();
        return false;
    }

    Debug("WAV format verified: %dHz, %d-bit, %d channels", sample_rate, bits_per_sample, channels);

    bool ret = FindWavDataChunk(img_file, file);
    file.close();

    return ret;
}

bool CdRomCueBinImage::FindWavDataChunk(ImgFile* img_file, std::ifstream& file)
{
    // Reset to beginning of file + RIFF/WAVE headers
    file.seekg(12, std::ios::beg);

    uint32_t data_size = 0;
    uint32_t data_offset = 0;
    bool found_data = false;

    while (!file.eof() && !found_data)
    {
        char chunk_id[4];
        u32 chunk_size;

        file.read(chunk_id, 4);
        file.read((char*)(&chunk_size), 4);

        if (file.eof())
            break;

        if (strncmp(chunk_id, "data", 4) == 0)
        {
            data_size = chunk_size;
            data_offset = (u32)file.tellg();
            found_data = true;
            break;
        }

        file.seekg(chunk_size, std::ios::cur);
    }
    
    if (!found_data)
    {
        Error("Failed to find 'data' chunk in WAV file %s", img_file->file_path);
        return false;
    }
    
    Debug("WAV data chunk found at offset %d with size %d", data_offset, data_size);

    img_file->is_wav = true;
    img_file->wav_data_offset = data_offset;
    img_file->file_size = data_size;

    return true;
}

void CdRomCueBinImage::SetupFileChunks(ImgFile* img_file)
{
    img_file->chunk_size = CDROM_MEDIA_CHUNK_SIZE;
    img_file->chunk_count = img_file->file_size / img_file->chunk_size;

    if (img_file->file_size % img_file->chunk_size != 0)
        img_file->chunk_count++;

    img_file->chunks = new u8*[img_file->chunk_count];

    for (u32 i = 0; i < img_file->chunk_count; i++)
        InitPointer(img_file->chunks[i]);
}

u32 CdRomCueBinImage::CalculateFileOffset(ImgFile* img_file, u32 chunk_index)
{
    u32 offset = chunk_index * img_file->chunk_size;

    if (img_file->is_wav)
        offset += img_file->wav_data_offset;

    return offset;
}

u32 CdRomCueBinImage::CalculateReadSize(ImgFile* img_file, u32 file_offset)
{
    u32 to_read = img_file->chunk_size;
    u32 effective_offset = file_offset;

    if (img_file->is_wav)
        effective_offset -= img_file->wav_data_offset;

    if (effective_offset + to_read > img_file->file_size)
        to_read = img_file->file_size - effective_offset;

    return to_read;
}

bool CdRomCueBinImage::ParseCueFile(const char* cue_content)
{
    using namespace std;

    if (!IsValidPointer(cue_content))
    {
        Error("Invalid CUE content pointer");
        return false;
    }

    istringstream stream(cue_content);
    string line;
    vector<ParsedCueFile> parsed_files;
    ParsedCueTrack current_parsed_track;
    InitParsedCueTrack(current_parsed_track);
    bool in_track = false;

    while (getline(stream, line))
    {
        line.erase(0, line.find_first_not_of(" \t"));

        if (line.empty() || line[0] == '#')
            continue;

        string lowercase_line = line;
        transform(lowercase_line.begin(), lowercase_line.end(), lowercase_line.begin(), [](unsigned char c) { return std::tolower(c); });

        if (lowercase_line.find("file") == 0)
        {
            if (in_track)
            {
                in_track = false;
                parsed_files.back().tracks.push_back(current_parsed_track);
            }

            string current_file_path;
            string file_name;

            size_t first_quote = line.find_first_of("\"");
            size_t last_quote = line.find_last_of("\"");

            if (first_quote != string::npos && last_quote != string::npos && first_quote != last_quote)
            {
                current_file_path = line.substr(first_quote + 1, last_quote - first_quote - 1);
                file_name = current_file_path;
            }
            else
            {
                istringstream file_stream(line.substr(4));
                file_stream >> current_file_path;

                if (current_file_path.empty())
                {
                    Error("Invalid FILE format in CUE: %s", line.c_str());
                    return false;
                }

                file_name = current_file_path;
            }

            if (!current_file_path.empty() && current_file_path[0] != '/' && current_file_path[0] != '\\' &&
                (current_file_path.size() < 2 || current_file_path[1] != ':'))
            {
                current_file_path = string(m_file_directory) + "/" + current_file_path;
            }

            Debug("Found FILE: %s", current_file_path.c_str());

            ImgFile* img_file = new ImgFile;
            InitImgFile(img_file);

            strncpy_fit(img_file->file_path, current_file_path.c_str(), sizeof(img_file->file_path));
            strncpy_fit(img_file->file_name, file_name.c_str(), sizeof(img_file->file_name));

            if (!GatherImgInfo(img_file))
            {
                Error("Failed to gather ImgFile info for %s", current_file_path.c_str());
                SafeDelete(img_file);
                return false;
            }

            m_img_files.push_back(img_file);

            ParsedCueFile parsed_file;
            InitParsedCueFile(parsed_file);
            parsed_file.img_file = img_file;
            parsed_files.push_back(parsed_file);
        }
        else if (lowercase_line.find("track") == 0)
        {
            if (in_track)
                parsed_files.back().tracks.push_back(current_parsed_track);

            in_track = true;
            current_parsed_track = ParsedCueTrack();
            InitParsedCueTrack(current_parsed_track);

            if (parsed_files.empty())
            {
                Error("TRACK found without FILE in CUE");
                return false;
            }

            istringstream track_stream(line.substr(5));
            track_stream >> current_parsed_track.number;

            string type_str;
            track_stream >> type_str;
            transform(type_str.begin(), type_str.end(), type_str.begin(), [](unsigned char c) { return std::tolower(c); });

            if (type_str == "audio")
            {
                current_parsed_track.type = GG_CDROM_AUDIO_TRACK;
                Debug("Found TRACK %d: AUDIO", current_parsed_track.number);
            }
            else if (type_str == "mode1/2048")
            {
                current_parsed_track.type = GG_CDROM_DATA_TRACK_MODE1_2048;
                Debug("Found TRACK %d: DATA (MODE1/2048)", current_parsed_track.number);
            }
            else if (type_str == "mode1/2352")
            {
                current_parsed_track.type = GG_CDROM_DATA_TRACK_MODE1_2352;
                Debug("Found TRACK %d: DATA (MODE1/2352)", current_parsed_track.number);
            }
            else if (type_str.find("mode2/") != string::npos)
            {
                Error("Unsupported track type MODE2: %s", type_str.c_str());
                return false;
            }
            else
            {
                Log("WARNING: Unknown track type: %s", type_str.c_str());
                return false;
            }
        }
        else if (lowercase_line.find("pregap") == 0)
        {
            int m = 0, s = 0, f = 0;
            char colon1, colon2;
            istringstream pregap_stream(line.substr(6));
            if (!(pregap_stream >> m >> colon1 >> s >> colon2 >> f) ||
                colon1 != ':' || colon2 != ':' ||
                m < 0 || s < 0 || f < 0 || s >= 60 || f >= 75 || m > 99)
            {
                Error("Invalid time format in PREGAP entry");
                continue;
            }

            GG_CdRomMSF pregap_msf;
            pregap_msf.minutes = (u8)m;
            pregap_msf.seconds = (u8)s;
            pregap_msf.frames = (u8)f;
            current_parsed_track.pregap_length = MsfToLba(&pregap_msf);
            current_parsed_track.has_pregap = true;

            Debug("Track %d pregap length %02d:%02d:%02d", current_parsed_track.number, m, s, f);
        }
        else if (lowercase_line.find("index") == 0)
        {
            if (!in_track)
            {
                Error("INDEX found outside of TRACK in CUE file");
                return false;
            }

            int index_number;
            istringstream index_stream(line.substr(5));
            index_stream >> index_number;

            int m = 0, s = 0, f = 0;
            char colon1, colon2;

            if (!(index_stream >> m >> colon1 >> s >> colon2 >> f) ||
                colon1 != ':' || colon2 != ':' ||
                m < 0 || s < 0 || f < 0 || s >= 60 || f >= 75 || m > 99)
            {
                Error("Invalid time format in INDEX entry");
                continue;
            }

            GG_CdRomMSF msf;
            msf.minutes = (u8)m;
            msf.seconds = (u8)s;
            msf.frames = (u8)f;

            if (index_number == 0)
            {
                current_parsed_track.index0_lba = MsfToLba(&msf);
                current_parsed_track.has_index0 = true;
                Debug("Track %d lead-in (INDEX 00) at %02d:%02d:%02d", current_parsed_track.number, m, s, f);
            }
            else if (index_number == 1)
            {
                current_parsed_track.index1_lba = MsfToLba(&msf);
                Debug("Track %d starts (INDEX 01) at %02d:%02d:%02d", current_parsed_track.number, m, s, f);
            }
        }
    }

    if (in_track)
        parsed_files.back().tracks.push_back(current_parsed_track);

    if (parsed_files.empty())
    {
        Error("No valid files found in CUE file");
        return false;
    }

    for (size_t i = 0; i < parsed_files.size(); i++)
    {
        ParsedCueFile& f = parsed_files[i];

        if (f.tracks.empty())
        {
            Error("No tracks found for file %s", f.img_file->file_path);
            continue;
        }

        if (!IsValidPointer(f.img_file))
        {
            Error("Invalid ImgFile pointer for file %s", f.img_file->file_path);
            continue;
        }

        u32 start_sector = ((i == 0) ? 0 : m_toc.tracks.back().end_lba + 1);
        u32 total_pregap_length = 0;

        for (size_t j = 0; j < f.tracks.size(); j++)
        {
            ParsedCueTrack& p = f.tracks[j];
            Track track;
            TrackFile track_file;
            InitTrack(track);
            InitTrackFile(track_file);
            track.type = p.type;
            track.sector_size = TrackTypeSectorSize(p.type);
            track_file.img_file = f.img_file;

            if (p.has_pregap)
                total_pregap_length += p.pregap_length;

            track.start_lba = p.index1_lba + total_pregap_length + start_sector;
            LbaToMsf(track.start_lba, &track.start_msf);

            if (p.has_pregap)
            {
                track.has_lead_in = true;
                track.lead_in_lba = p.index1_lba + total_pregap_length - p.pregap_length + start_sector;
            }
            else if (p.has_index0)
            {
                track.has_lead_in = true;
                track.lead_in_lba = p.index0_lba + start_sector;
            }

            u32 current_file_offset = 0;

            if(j != 0)
            {
                Track& prev = m_toc.tracks.back();
                prev.end_lba = track.has_lead_in ? track.lead_in_lba - 1 : track.start_lba - 1;
                LbaToMsf(prev.end_lba, &prev.end_msf);

                prev.sector_count = prev.end_lba - prev.start_lba + 1;
                current_file_offset = prev.file_offset + (prev.sector_count * prev.sector_size);
            }

            track.file_offset = current_file_offset;

            if (track.has_lead_in && !p.has_pregap)
                track.file_offset += (track.start_lba - track.lead_in_lba) * track.sector_size;

            m_toc.tracks.push_back(track);
            m_track_files.push_back(track_file);
        }

        Track& last = m_toc.tracks.back();
        u32 last_size = (f.img_file->file_size - last.file_offset);
        last.sector_count = last_size / last.sector_size;

        if (last_size % last.sector_size != 0)
        {
            Log("WARNING: Last track has remaining bytes that do not fit into a full sector:");
            Log("File size: %u, File offset: %llu, Sector size: %u", f.img_file->file_size, last.file_offset, last.sector_size);
            last.sector_count++;
        }

        last.end_lba = last.start_lba + last.sector_count - 1;
        LbaToMsf(last.end_lba, &last.end_msf);
    }

    for (size_t i = 0; i < m_toc.tracks.size(); ++i)
    {
        Track& track = m_toc.tracks[i];

        Log("Track %2d (%s): Start LBA: %6u, End LBA: %6u, Sectors: %6u, File Offset: %8llu",
                i + 1,
                TrackTypeName(track.type),
                track.start_lba,
                track.end_lba,
                track.sector_count,
                track.file_offset);
    }

    Log("Successfully parsed CUE file with %d tracks", (int)m_toc.tracks.size());

    if (m_toc.tracks.empty())
    {
        m_toc.sector_count = 0;
        m_toc.total_length = {0, 0, 0};
    }
    else
    {
        m_toc.sector_count = m_toc.tracks.back().end_lba + 1;
        LbaToMsf(m_toc.sector_count + 150, &m_toc.total_length);
    }

    Debug("CD-ROM length: %02d:%02d:%02d, Total sectors: %d",
        m_toc.total_length.minutes, m_toc.total_length.seconds, m_toc.total_length.frames,
        m_toc.sector_count);

    return !m_toc.tracks.empty();
}

bool CdRomCueBinImage::ReadFromImgFile(ImgFile* img_file, u32 offset, u8* buffer, u32 size)
{
    if (!IsValidPointer(img_file) || !IsValidPointer(buffer))
    {
        Error("ReadFromImgFile failed - Invalid ImgFile pointer or buffer");
        return false;
    }

    if (offset + size > img_file->file_size)
    {
        Error("ReadFromImgFile failed - Offset %llu + size %d exceeds file size %d",
            offset, size, img_file->file_size);
        return false;
    }

    const u32 chunk_size = img_file->chunk_size;
    u32 chunk_index = offset / chunk_size;
    u32 chunk_offset = offset % chunk_size;

    if (img_file->chunks[chunk_index] == NULL)
    {
        if (!LoadChunk(img_file, chunk_index))
        {
            Error("Failed to load chunk %d", chunk_index);
            return false;
        }
    }

    if (chunk_offset + size <= chunk_size)
    {
        //Debug("Reading %d bytes from chunk %d, offset %d", size, chunk_index, chunk_offset);
        memcpy(buffer, img_file->chunks[chunk_index] + chunk_offset, size);
    }
    else
    {
        u32 first_part = chunk_size - chunk_offset;

        //Debug("Reading %d bytes from chunk %d (crossing), offset %d", first_part, chunk_index, chunk_offset);
        memcpy(buffer, img_file->chunks[chunk_index] + chunk_offset, first_part);

        if (img_file->chunks[chunk_index + 1] == NULL)
        {
            if (!LoadChunk(img_file, chunk_index + 1))
            {
                Error("Failed to load chunk %d", chunk_index + 1);
                return false;
            }
        }

        //Debug("Reading %d bytes from chunk %d (crossing), offset 0", size - first_part, chunk_index + 1);
        memcpy(buffer + first_part, img_file->chunks[chunk_index + 1], size - first_part);
    }

    return true;
}

bool CdRomCueBinImage::LoadChunk(ImgFile* img_file, u32 chunk_index)
{
    using namespace std;

    if (!IsValidPointer(img_file))
    {
        Error("Cannot load chunk - Invalid ImgFile pointer");
        return false;
    }

    if (!img_file->chunks[chunk_index])
    {
        ifstream file;
        open_ifstream_utf8(file, img_file->file_path, ios::in | ios::binary);

        if (!file.is_open())
        {
            Error("Cannot load chunk - Unable to open file %s", img_file->file_path);
            return false;
        }

        u32 file_offset = CalculateFileOffset(img_file, chunk_index);
        file.seekg(file_offset, ios::beg);

        if (file.fail())
        {
            Error("Cannot load chunk - Failed to seek to offset %llu in file %s", file_offset, img_file->file_path);
            return false;
        }

        img_file->chunks[chunk_index] = new u8[img_file->chunk_size];

        u32 to_read = CalculateReadSize(img_file, file_offset);

        Debug("Loading chunk %d from %s", chunk_index, img_file->file_path);
        file.read(reinterpret_cast<char*>(img_file->chunks[chunk_index]), to_read);

        if (file.gcount() != to_read)
        {
            Error("Failed to read chunk %d from %s. Read %d bytes, expected %d bytes",
                chunk_index, img_file->file_path, file.gcount(), to_read);
            file.close();
            return false;
        }

        file.close();
    }

    return true;
}

bool CdRomCueBinImage::PreloadChunks(ImgFile* img_file, u32 start_chunk, u32 count)
{
    if (!IsValidPointer(img_file))
    {
        Error("Cannot preload chunks - Invalid ImgFile pointer");
        return false;
    }

    if (start_chunk >= img_file->chunk_count)
    {
        Error("Cannot preload chunks - Start chunk index %d out of bounds (max: %d)",
            start_chunk, img_file->chunk_count - 1);
        return false;
    }

    u32 end_chunk = start_chunk + count;
    if (end_chunk > img_file->chunk_count)
    {
        end_chunk = img_file->chunk_count;
    }

    Debug("Preloading chunks %d-%d from %s", start_chunk, end_chunk - 1, img_file->file_path);

    for (u32 i = start_chunk; i < end_chunk; i++)
    {
        if (img_file->chunks[i] == NULL)
        {
            if (!LoadChunk(img_file, i))
            {
                Error("Failed to preload chunk %d", i);
                return false;
            }
        }
    }

    return true;
}

void CdRomCueBinImage::CalculateCRC()
{
    using namespace std;
    m_crc = 0;

    if (m_toc.tracks.empty())
    {
        Log("No tracks to calculate CRC from");
        return;
    }

    Track* first_data_track = NULL;
    TrackFile* first_data_track_file = NULL;
    size_t track_count = m_toc.tracks.size();

    for (size_t i = 0; i < track_count; i++)
    {
        if (m_toc.tracks[i].type == GG_CDROM_DATA_TRACK_MODE1_2048 ||
            m_toc.tracks[i].type == GG_CDROM_DATA_TRACK_MODE1_2352)
        {
            first_data_track = &m_toc.tracks[i];
            first_data_track_file = &m_track_files[i];
            break;
        }
    }

    if (!first_data_track)
    {
        Log("No data tracks found for CRC calculation");
        return;
    }

    if (first_data_track->sector_count == 0)
    {
        Log("First data track has no sectors, cannot calculate CRC");
        return;
    }

    u32 sector_data_size = 2048;
    u8* buffer = new u8[sector_data_size];
    ImgFile* img_file = first_data_track_file->img_file;

    if (!IsValidPointer(img_file))
    {
        Error("Invalid ImgFile pointer for first data track");
        SafeDeleteArray(buffer);
        return;
    }

    ifstream file;
    open_ifstream_utf8(file, img_file->file_path, ios::in | ios::binary);
    if (!file.is_open())
    {
        Error("Failed to open file %s for CRC calculation",
            img_file->file_path);
        SafeDeleteArray(buffer);
        return;
    }

    u32 sectors_to_crc = 64;
    u32 first_sector = 1;
    u32 last_needed = first_sector + sectors_to_crc - 1;
    u32 max_index = ((first_data_track->sector_count - 1) >= last_needed)
                     ? last_needed
                     : (first_data_track->sector_count - 1);

    for (u32 sec = first_sector; sec <= max_index; sec++)
    {
        u32 file_offset = first_data_track->file_offset + (first_data_track->sector_size * sec);

        if (first_data_track->sector_size == 2352)
            file_offset += 16;

        file.seekg(file_offset, ios::beg);
        if (file.fail())
        {
            Error("Seek failed for sector %u in file %s",
                sec, img_file->file_path);
            break;
        }

        file.read((char*)(buffer), sector_data_size);
        u32 bytes_read = (u32)(file.gcount());

        if (bytes_read != sector_data_size)
        {
            Error("Incomplete read for sector %u: %u bytes read, expected %u",
                sec, bytes_read, sector_data_size);
            break;
        }

        m_crc = CalculateCRC32(m_crc, buffer, sector_data_size);
    }

    file.close();
    SafeDeleteArray(buffer);
}
