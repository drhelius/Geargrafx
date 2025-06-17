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

#include <string.h>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include "cdrom_media.h"
#include "crc.h"

CdRomMedia::CdRomMedia()
{
    m_ready = false;
    m_crc = 0;
    m_file_path[0] = 0;
    m_file_directory[0] = 0;
    m_file_name[0] = 0;
    m_file_extension[0] = 0;
    m_cdrom_length = {0, 0, 0};
    m_sector_count = 0;
    m_current_sector = 0;
}

CdRomMedia::~CdRomMedia()
{
    DestroyImgFiles();
}

void CdRomMedia::DestroyImgFiles()
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
}

void CdRomMedia::Init()
{
    Reset();
}

void CdRomMedia::Reset()
{
    DestroyImgFiles();
    m_ready = false;
    m_crc = 0;
    m_file_path[0] = 0;
    m_file_directory[0] = 0;
    m_file_name[0] = 0;
    m_file_extension[0] = 0;
    m_tracks.clear();
    m_img_files.clear();
    m_cdrom_length = {0, 0, 0};
    m_sector_count = 0;
    m_current_sector = 0;
}

bool CdRomMedia::LoadCueFromFile(const char* path)
{
    using namespace std;

    Log("Loading CD-ROM Media from %s...", path);

    if (!IsValidPointer(path))
    {
        Log("ERROR: Invalid path %s", path);
        return false;
    }

    Reset();
    GatherPaths(path);

    if (strcmp(m_file_extension, "cue") != 0)
    {
        Log("ERROR: Invalid file extension %s. Expected .cue", m_file_extension);
        return false;
    }

    ifstream file(path, ios::in | ios::binary | ios::ate);

    if (file.is_open())
    {
        int size = (int)(file.tellg());

        if (size <= 0)
        {
            Log("ERROR: Unable to open file %s. Size: %d", path, size);
            file.close();
            return false;
        }

        if (file.bad() || file.fail() || !file.good() || file.eof())
        {
            Log("ERROR: Unable to open file %s. Bad file!", path);
            file.close();
            return false;
        }

        char* memblock = new char[size + 1];
        file.seekg(0, ios::beg);
        file.read(memblock, size);
        file.close();

        for (int i = 0; i < size; i++)
        {
            if (memblock[i] != 0)
                break;

            if (i == size - 1)
            {
                Log("ERROR: File %s is empty!", path);
                SafeDeleteArray(memblock);
                return false;
            }
        }

        m_ready = LoadCueFromBuffer(reinterpret_cast<u8*>(memblock), size, path);

        SafeDeleteArray(memblock);
    }
    else
    {
        Log("ERROR: There was a problem loading the file %s...", path);
        m_ready = false;
    }

    if (!m_ready)
        Reset();

    return m_ready;
}

bool CdRomMedia::LoadCueFromBuffer(const u8* buffer, int size, const char* path)
{
    if (IsValidPointer(buffer))
    {
        Debug("Loading CD-ROM Media from buffer... Size: %d", size);

        Reset();

        if (IsValidPointer(path))
        {
            GatherPaths(path);
        }

        char* cue_content = new char[size + 1];
        memcpy(cue_content, buffer, size);
        cue_content[size] = 0;

        m_ready = ParseCueFile(cue_content);

        SafeDeleteArray(cue_content);

        if (m_ready)
        {
            Debug("CD-ROM Media loaded from buffer. Track count: %d", m_tracks.size());
        }
        else
        {
            Log("ERROR: Failed to parse CUE file");
        }

        return m_ready;
    }
    else
    {
        Log("ERROR: Unable to load CD-ROM Media from buffer: Buffer invalid %p. Size: %d", buffer, size);
        return false;
    }
}

void CdRomMedia::GatherPaths(const char* path)
{
    using namespace std;

    string fullpath(path);
    string directory;
    string filename;
    string extension;

    size_t pos = fullpath.find_last_of("/\\");
    if (pos != string::npos)
    {
        filename = fullpath.substr(pos + 1);
        directory = fullpath.substr(0, pos);
    }
    else
    {
        filename = fullpath;
        directory = "";
    }

    extension = fullpath.substr(fullpath.find_last_of(".") + 1);
    transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });

    snprintf(m_file_path, sizeof(m_file_path), "%s", path);
    snprintf(m_file_directory, sizeof(m_file_directory), "%s", directory.c_str());
    snprintf(m_file_name, sizeof(m_file_name), "%s", filename.c_str());
    snprintf(m_file_extension, sizeof(m_file_extension), "%s", extension.c_str());
}

bool CdRomMedia::GatherImgInfo(ImgFile* img_file)
{
    if (!IsValidPointer(img_file))
    {
        Log("ERROR: Invalid ImgFile pointer");
        return false;
    }

    if (!IsValidPointer(img_file->file_path))
    {
        Log("ERROR: Invalid file path in ImgFile");
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

bool CdRomMedia::ValidateFile(const char* file_path)
{
    using namespace std;

    ifstream file(file_path, ios::in | ios::binary | ios::ate);

    if (file.is_open())
    {
        int size = (int)(file.tellg());

        if (size <= 0)
        {
            Log("ERROR: Unable to open file %s. Size: %d", file_path, size);
            file.close();
            return false;
        }

        if (file.bad() || file.fail() || !file.good() || file.eof())
        {
            Log("ERROR: Unable to open file %s. Bad file!", file_path);
            file.close();
            return false;
        }

        file.close();
        return true;
    }

    Log("ERROR: Unable to open file %s", file_path);
    return false;
}

bool CdRomMedia::ProcessFileFormat(ImgFile* img_file)
{
    using namespace std;

    string file_path(img_file->file_path);
    string extension = file_path.substr(file_path.find_last_of(".") + 1);
    transform(extension.begin(), extension.end(), extension.begin(), (int(*)(int)) tolower);

    ifstream file(img_file->file_path, ios::in | ios::binary | ios::ate);
    int size = (int)(file.tellg());
    file.close();

    img_file->file_size = (u32)size;

    if (extension == "wav")
        return ProcessWavFormat(img_file);

    return true;
}

bool CdRomMedia::ProcessWavFormat(ImgFile* img_file)
{
    using namespace std;

    Debug("WAV file detected: %s", img_file->file_path);

    ifstream file(img_file->file_path, ios::in | ios::binary);
    if (!file.is_open())
        return false;

    char header[44];
    file.read(header, 44);

    if (file.gcount() != 44)
    {
        Log("ERROR: Failed to read WAV header from %s", img_file->file_path);
        file.close();
        return false;
    }

    if (strncmp(header, "RIFF", 4) != 0 || strncmp(header + 8, "WAVE", 4) != 0)
    {
        Log("ERROR: Invalid WAV format in %s", img_file->file_path);
        file.close();
        return false;
    }

    u16 channels = *(u16*)(header + 22);
    u32 sample_rate = *(u32*)(header + 24);
    u16 bits_per_sample = *(u16*)(header + 34);

    if (sample_rate != 44100 || bits_per_sample != 16 || channels != 2)
    {
        Log("ERROR: WAV file %s has incorrect format. Required: 44100Hz, 16-bit, stereo. Found: %dHz, %d-bit, %d channel(s)", img_file->file_path, sample_rate, bits_per_sample, channels);
        file.close();
        return false;
    }

    Debug("WAV format verified: %dHz, %d-bit, %d channels", sample_rate, bits_per_sample, channels);

    bool ret = FindWavDataChunk(img_file, file);
    file.close();

    return ret;
}

bool CdRomMedia::FindWavDataChunk(ImgFile* img_file, std::ifstream& file)
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
        Log("ERROR: Failed to find 'data' chunk in WAV file %s", img_file->file_path);
        return false;
    }
    
    Debug("WAV data chunk found at offset %d with size %d", data_offset, data_size);

    img_file->is_wav = true;
    img_file->wav_data_offset = data_offset;
    img_file->file_size = data_size;

    return true;
}

void CdRomMedia::SetupFileChunks(ImgFile* img_file)
{
    img_file->chunk_size = CDROM_MEDIA_CHUNK_SIZE;
    img_file->chunk_count = img_file->file_size / img_file->chunk_size;
    if (img_file->file_size % img_file->chunk_size != 0)
        img_file->chunk_count++;
    img_file->chunks = new u8*[img_file->chunk_count];

    for (u32 i = 0; i < img_file->chunk_count; i++)
        InitPointer(img_file->chunks[i]);
}

void CdRomMedia::CalculateCRC()
{
    using namespace std;

    m_crc = 0;

    if (m_img_files.empty())
    {
        Debug("No image files to calculate CRC from");
        return;
    }

    Debug("Calculating CRC for %d image files", m_img_files.size());

    const u32 buffer_size = 1024 * 1024; // 1MB buffer for reading
    u8* buffer = new u8[buffer_size];

    for (size_t i = 0; i < m_img_files.size(); i++)
    {
        ImgFile* img_file = m_img_files[i];
        
        if (!IsValidPointer(img_file))
        {
            Log("ERROR: Invalid ImgFile pointer while calculating CRC");
            continue;
        }

        Debug("Processing file %s for CRC calculation", img_file->file_path);

        ifstream file(img_file->file_path, ios::in | ios::binary);
        if (!file.is_open())
        {
            Log("ERROR: Failed to open file %s for CRC calculation", img_file->file_path);
            continue;
        }

        // Skip to data offset for WAV files
        u64 start_offset = img_file->is_wav ? img_file->wav_data_offset : 0;
        file.seekg(start_offset, ios::beg);

        if (file.fail())
        {
            Log("ERROR: Failed to seek to offset %llu in file %s", start_offset, img_file->file_path);
            file.close();
            continue;
        }

        u32 remaining_bytes = img_file->file_size;

        while (remaining_bytes > 0 && !file.eof())
        {
            u32 to_read = (remaining_bytes < buffer_size) ? remaining_bytes : buffer_size;

            file.read(reinterpret_cast<char*>(buffer), to_read);
            u32 bytes_read = static_cast<u32>(file.gcount());

            if (bytes_read > 0)
            {
                m_crc = CalculateCRC32(m_crc, buffer, bytes_read);
                remaining_bytes -= bytes_read;
            }

            if (bytes_read < to_read)
                break;
        }

        file.close();
    }

    SafeDeleteArray(buffer);
}

bool CdRomMedia::ParseCueFile(const char* cue_content)
{
    using namespace std;

    if (!IsValidPointer(cue_content))
    {
        Log("ERROR: Invalid CUE content pointer");
        return false;
    }

    istringstream stream(cue_content);
    string line;

    Track current_track;
    ImgFile* current_img_file = NULL;
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
                istringstream file_stream(line.substr(4)); // Skip "FILE"
                file_stream >> current_file_path;

                if (current_file_path.empty())
                {
                    Log("ERROR: Invalid FILE format in CUE: %s", line.c_str());
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
            img_file->file_size = 0;
            img_file->chunk_size = 0;
            img_file->chunk_count = 0;
            img_file->chunks = NULL;
            img_file->is_wav = false;
            img_file->wav_data_offset = 0;
            img_file->file_name[0] = 0;
            img_file->file_path[0] = 0;

            strncpy_fit(img_file->file_path, current_file_path.c_str(), sizeof(img_file->file_path));
            strncpy_fit(img_file->file_name, file_name.c_str(), sizeof(img_file->file_name));
            if (!GatherImgInfo(img_file))
            {
                Log("ERROR: Failed to gather ImgFile info for %s", current_file_path.c_str());
                SafeDelete(img_file);
                return false;
            }
            m_img_files.push_back(img_file);
            current_img_file = img_file;
        }
        else if (lowercase_line.find("track") == 0)
        {
            if (in_track)
                m_tracks.push_back(current_track);

            in_track = true;
            current_track = Track();

            if (!IsValidPointer(current_img_file))
            {
                Log("ERROR: TRACK found without FILE in CUE");
                return false;
            }

            current_track.img_file = current_img_file;

            istringstream track_stream(line.substr(5));
            track_stream >> current_track.number;

            string type_str;
            track_stream >> type_str;
            transform(type_str.begin(), type_str.end(), type_str.begin(), [](unsigned char c) { return std::tolower(c); });

            if (type_str == "audio")
            {
                current_track.type = AUDIO_TRACK;
                Debug("Found TRACK %d: AUDIO", current_track.number);
            }
            else if (type_str == "mode1/2048")
            {
                current_track.type = DATA_TRACK_MODE1_2048;
                Debug("Found TRACK %d: DATA (MODE1/2048)", current_track.number);
            }
            else if (type_str == "mode1/2352")
            {
                current_track.type = DATA_TRACK_MODE1_2352;
                Debug("Found TRACK %d: DATA (MODE1/2352)", current_track.number);
            }
            else if (type_str.find("mode2/") != string::npos)
            {
                Log("ERROR: Unsupported track type MODE2: %s", type_str.c_str());
                return false;
            }
            else
            {
                Log("WARNING: Unknown track type: %s", type_str.c_str());
                return false;
            }

            current_track.sector_size = GetTrackSectorSize(current_track.type);
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
                Log("ERROR: Invalid time format in PREGAP entry");
                continue;
            }

            current_track.lead_in_msf.minutes = (u8)m;
            current_track.lead_in_msf.seconds = (u8)s;
            current_track.lead_in_msf.frames = (u8)f;
            current_track.lead_in_lba = MsfToLba(&current_track.lead_in_msf);
            current_track.has_lead_in = true;
            Debug("Track %d pregap at %02d:%02d:%02d", current_track.number, m, s, f);
        }
        else if (lowercase_line.find("index") == 0)
        {
            if (!in_track)
            {
                Log("ERROR: INDEX found outside of TRACK in CUE file");
                continue;
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
                Log("ERROR: Invalid time format in INDEX entry");
                continue;
            }

            if (index_number == 0) {
                current_track.lead_in_msf.minutes = (u8)m;
                current_track.lead_in_msf.seconds = (u8)s;
                current_track.lead_in_msf.frames = (u8)f;
                current_track.lead_in_lba = MsfToLba(&current_track.lead_in_msf);
                current_track.has_lead_in = true;

                Debug("Track %d lead-in at %02d:%02d:%02d", current_track.number, m, s, f);
            } else if (index_number == 1) {
                current_track.start_msf.minutes = (u8)m;
                current_track.start_msf.seconds = (u8)s;
                current_track.start_msf.frames = (u8)f;
                current_track.start_lba = MsfToLba(&current_track.start_msf);

                Debug("Track %d starts at %02d:%02d:%02d", current_track.number, m, s, f);
            }
        }
    }

    if (in_track)
        m_tracks.push_back(current_track);

    if (!m_tracks.empty())
    {
        u32 cumulative_offset_lba = 0;
        ImgFile* prev_file = m_tracks[0].img_file;
        u32 prev_sector_size = m_tracks[0].sector_size;

        for (size_t i = 0; i < m_tracks.size(); i++)
        {
            if (m_tracks[i].img_file != prev_file)
            {
                u32 file_sectors = prev_file->file_size / prev_sector_size;
                cumulative_offset_lba += file_sectors;
                prev_file = m_tracks[i].img_file;
                prev_sector_size = m_tracks[i].sector_size;
            }

            m_tracks[i].start_lba += cumulative_offset_lba;

            if (m_tracks[i].has_lead_in)
                m_tracks[i].lead_in_lba += cumulative_offset_lba;
        }

        prev_file = m_tracks[0].img_file;
        u32 file_offset = 0;

        for (size_t i = 0; i < m_tracks.size(); i++)
        {
            if (m_tracks[i].img_file != prev_file)
            {
                prev_file = m_tracks[i].img_file;
                file_offset = 0;
            }

            if ((i + 1) < m_tracks.size())
            {
                if (m_tracks[i + 1].has_lead_in)
                    m_tracks[i].sector_count = m_tracks[i + 1].lead_in_lba - m_tracks[i].start_lba;
                else
                    m_tracks[i].sector_count = m_tracks[i + 1].start_lba - m_tracks[i].start_lba;

                m_tracks[i].end_lba = m_tracks[i].start_lba + m_tracks[i].sector_count - 1;
                LbaToMsf(m_tracks[i].end_lba, &m_tracks[i].end_msf);
            }
            else
            {
                if (IsValidPointer(m_tracks[i].img_file))
                {
                    u32 prev_bytes = 0;
                    for (size_t j = 0; j < i; j++)
                    {
                        if (m_tracks[j].img_file == m_tracks[i].img_file)
                            prev_bytes += m_tracks[j].sector_count * m_tracks[j].sector_size;
                    }

                    u32 pregap_bytes = 0;
                    for (size_t j = 0; j <= i; j++)
                    {
                        if (m_tracks[j].img_file == m_tracks[i].img_file && m_tracks[j].has_lead_in)
                        {
                            u32 pregap_sectors = m_tracks[j].start_lba - m_tracks[j].lead_in_lba;
                            pregap_bytes += pregap_sectors * m_tracks[j].sector_size;
                        }
                    }

                    u32 usable_bytes = m_tracks[i].img_file->file_size - prev_bytes - pregap_bytes;
                    m_tracks[i].sector_count = usable_bytes / m_tracks[i].sector_size;
                    m_tracks[i].end_lba = m_tracks[i].start_lba + m_tracks[i].sector_count - 1;
                    LbaToMsf(m_tracks[i].end_lba, &m_tracks[i].end_msf);
                }
                else
                {
                    m_tracks[i].sector_count = 75 * 60 * 80;
                    m_tracks[i].end_lba = m_tracks[i].start_lba + m_tracks[i].sector_count - 1;
                    LbaToMsf(m_tracks[i].end_lba, &m_tracks[i].end_msf);
                }
            }

            m_tracks[i].file_offset = file_offset;
            file_offset += m_tracks[i].sector_count * m_tracks[i].sector_size;

            if (m_tracks[i].has_lead_in)
            {
                m_tracks[i].file_offset += (m_tracks[i].start_lba - m_tracks[i].lead_in_lba) * m_tracks[i].sector_size;
            }

            Log("Track %d (%s): Start LBA: %d, End LBA: %d, Sectors: %d, File Offset: %d",
                m_tracks[i].number, GetTrackTypeName(m_tracks[i].type),
                m_tracks[i].start_lba, m_tracks[i].end_lba, m_tracks[i].sector_count, m_tracks[i].file_offset);
        }
    }

    Log("Successfully parsed CUE file with %d tracks", m_tracks.size());

    if (m_tracks.empty())
    {
        m_sector_count = 0;
        m_cdrom_length = {0, 0, 0};
    }
    else
    {
        m_sector_count = m_tracks.back().end_lba + 1;
        LbaToMsf(m_sector_count + 150, &m_cdrom_length);
    }

    Debug("CD-ROM length: %02d:%02d:%02d, Total sectors: %d",
        m_cdrom_length.minutes, m_cdrom_length.seconds, m_cdrom_length.frames,
        m_sector_count);

    CalculateCRC();

    return !m_tracks.empty();
}

bool CdRomMedia::ReadSector(u32 lba, u8* buffer)
{
    if (!m_ready || buffer == NULL)
    {
        Debug("ERROR: ReadSector failed - Media not ready or buffer is NULL");
        return false;
    }

    for (size_t i = 0; i < m_tracks.size(); i++)
    {
        const Track& track = m_tracks[i];
        u32 sector_size = track.sector_size;
        u32 start = track.start_lba;
        u32 end = start + track.sector_count;

        if (lba >= start && lba < end)
        {
            u32 sector_offset = lba - start;
            ImgFile* img_file = track.img_file;

            if (img_file == NULL || img_file->file_size == 0)
            {
                Debug("ERROR: ReadSector failed - ImgFile is NULL or file size is 0");
                return false;
            }

            u64 byte_offset = track.file_offset + (sector_offset * sector_size);

            if (sector_size == 2352)
            {
                byte_offset += 16;
                sector_size = 2048;
            }

            if (byte_offset + sector_size > img_file->file_size)
            {
                Debug("ERROR: ReadSector failed - Byte offset %llu + sector size %d exceeds file size %d",
                    byte_offset, sector_size, img_file->file_size);
                return false;
            }

            m_current_sector = lba + 1;
            if (m_current_sector >= m_sector_count)
                m_current_sector = m_sector_count - 1;

            Debug("Reading sector %d from track %d (offset: %d)", lba, i, byte_offset);

            return ReadFromImgFile(img_file, byte_offset, buffer, sector_size);
        }
    }

    Debug("ERROR: ReadSector failed - LBA %d not found in any track", lba);

    return false;
}

bool CdRomMedia::ReadBytes(u32 lba, u32 offset, u8* buffer, u32 size)
{
    if (!m_ready || buffer == NULL)
    {
        Debug("ERROR: ReadBytes failed - Media not ready or buffer is NULL");
        return false;
    }

    if (lba >= m_sector_count)
    {
        Debug("ERROR: ReadBytes failed - LBA %d out of bounds (max: %d)", lba, m_sector_count - 1);
        return false;
    }

    for (size_t i = 0; i < m_tracks.size(); i++)
    {
        const Track& track = m_tracks[i];
        u32 sector_size = track.sector_size;
        u32 start = track.start_lba;
        u32 end = start + track.sector_count;

        if (lba >= start && lba < end)
        {
            u32 sector_offset = lba - start;
            ImgFile* img_file = track.img_file;

            if (img_file == NULL || img_file->file_size == 0)
            {
                Debug("ERROR: ReadBytes failed - ImgFile is NULL or file size is 0");
                return false;
            }

            u64 byte_offset = track.file_offset + (sector_offset * sector_size) + offset;

            if (byte_offset + size > img_file->file_size)
            {
                Debug("ERROR: ReadBytes failed - Byte offset %llu + size %d exceeds file size %d",
                    byte_offset, size, img_file->file_size);
                return false;
            }

            m_current_sector = lba;

            return ReadFromImgFile(img_file, byte_offset, buffer, size);
        }
    }

    Debug("ERROR: ReadBytes failed - LBA %d not found in any track", lba);

    return false;
}

bool CdRomMedia::ReadFromImgFile(ImgFile* img_file, u64 offset, u8* buffer, u32 size)
{
    if (!IsValidPointer(img_file) || !IsValidPointer(buffer))
    {
        Debug("ERROR: ReadFromImgFile failed - Invalid ImgFile pointer or buffer");
        return false;
    }

    if (offset + size > img_file->file_size)
    {
        Debug("ERROR: ReadFromImgFile failed - Offset %llu + size %d exceeds file size %d",
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
            Debug("ERROR: Failed to load chunk %d", chunk_index);
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
                Debug("ERROR: Failed to load chunk %d", chunk_index + 1);
                return false;
            }
        }

        //Debug("Reading %d bytes from chunk %d (crossing), offset 0", size - first_part, chunk_index + 1);
        memcpy(buffer + first_part, img_file->chunks[chunk_index + 1], size - first_part);
    }

    return true;
}

bool CdRomMedia::LoadChunk(ImgFile* img_file, u32 chunk_index)
{
    using namespace std;

    if (!IsValidPointer(img_file))
    {
        Log("ERROR: Cannot load chunk - Invalid ImgFile pointer");
        return false;
    }

    ifstream file(img_file->file_path, ios::in | ios::binary);

    if (!file.is_open())
    {
        Log("ERROR: Cannot load chunk - Unable to open file %s", img_file->file_path);
        return false;
    }

    u64 file_offset = CalculateFileOffset(img_file, chunk_index);
    file.seekg(file_offset, ios::beg);

    if (file.fail())
    {
        Log("ERROR: Cannot load chunk - Failed to seek to offset %llu in file %s", file_offset, img_file->file_path);
        return false;
    }

    u32 to_read = CalculateReadSize(img_file, file_offset);

    if (!img_file->chunks[chunk_index])
        img_file->chunks[chunk_index] = new u8[img_file->chunk_size];

    Debug("Loading chunk %d from %s", chunk_index, img_file->file_path);
    file.read(reinterpret_cast<char*>(img_file->chunks[chunk_index]), to_read);

    if (file.gcount() != to_read)
    {
        Debug("ERROR: Failed to read chunk %d from %s. Read %d bytes, expected %d bytes",
            chunk_index, img_file->file_path, file.gcount(), to_read);
        file.close();
        return false;
    }

    file.close();
    return true;
}

u64 CdRomMedia::CalculateFileOffset(ImgFile* img_file, u32 chunk_index)
{
    u64 offset = (u64)chunk_index * (u64)img_file->chunk_size;

    if (img_file->is_wav)
        offset += img_file->wav_data_offset;

    return offset;
}

u32 CdRomMedia::CalculateReadSize(ImgFile* img_file, u64 file_offset)
{
    u32 to_read = img_file->chunk_size;
    u64 effective_offset = file_offset;

    if (img_file->is_wav)
        effective_offset -= img_file->wav_data_offset;

    if (effective_offset + to_read > img_file->file_size)
        to_read = img_file->file_size - effective_offset;

    return to_read;
}

bool CdRomMedia::PreloadChunks(ImgFile* img_file, u32 start_chunk, u32 count)
{
    if (!IsValidPointer(img_file))
    {
        Log("ERROR: Cannot preload chunks - Invalid ImgFile pointer");
        return false;
    }

    if (start_chunk >= img_file->chunk_count)
    {
        Log("ERROR: Cannot preload chunks - Start chunk index %d out of bounds (max: %d)",
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
                Log("ERROR: Failed to preload chunk %d", i);
                return false;
            }
        }
    }

    return true;
}

bool CdRomMedia::PreloadTrackChunks(u32 track_number)
{
    if (track_number >= m_tracks.size())
    {
        Log("ERROR: PreloadTrackChunks failed - Track number %d out of bounds (max: %d)", track_number, m_tracks.size() - 1);
        return false;
    }

    const Track& track = m_tracks[track_number];

    u32 sector_size = track.sector_size;
    u64 start_offset = track.file_offset;
    u64 total_bytes = track.sector_count * sector_size;
    u32 start_chunk = start_offset / track.img_file->chunk_size;
    u32 chunks_needed = (total_bytes + track.img_file->chunk_size - 1) / track.img_file->chunk_size;

    Debug("Preloading all sectors for track %d (sectors: %d, bytes: %llu)", track_number, track.sector_count, total_bytes);

    return PreloadChunks(track.img_file, start_chunk, chunks_needed);
}

u32 CdRomMedia::GetFirstSectorOfTrack(u8 track)
{
    if (track < m_tracks.size())
    {
        return m_tracks[track].start_lba;
    }
    else if ((track > 0) && (track == m_tracks.size()))
    {
        return m_tracks[track - 1].end_lba;
    }

    Debug("ERROR: GetFirstSectorOfTrack failed - Track number %d out of bounds (max: %d)", track, m_tracks.size());
    return 0;
}

u32 CdRomMedia::GetLastSectorOfTrack(u8 track)
{
    if (track < m_tracks.size())
    {
        return m_tracks[track].end_lba;
    }

    Log("ERROR: GetLastSectorOfTrack failed - Track number %d out of bounds (max: %d)", track, m_tracks.size());

    return 0;
}

s32 CdRomMedia::GetTrackFromLBA(u32 lba)
{
    if (lba >= m_sector_count)
    {
        Debug("ERROR: GetTrackNumber failed - LBA %d out of bounds (max: %d)", lba, m_sector_count - 1);
        return -1;
    }

    for (u8 i = 0; i < m_tracks.size(); i++)
    {
        if ((lba >= m_tracks[i].start_lba) && (lba <= m_tracks[i].end_lba))
        {
            return i;
        }
    }

    Debug("ERROR: GetTrackNumber failed - LBA %d not found in any track", lba);
    return -1;
}

///////////////////////////////////////////////////////////////
// Seek time, based on the work by Dave Shadoff
// https://github.com/pce-devel/PCECD_seek

u32 CdRomMedia::SeekFindGroup(u32 lba)
{
    for (u32 i = 0; i < GG_SEEK_NUM_SECTOR_GROUPS; i++)
        if ((lba >= k_seek_sector_list[i].sec_start) && (lba <= k_seek_sector_list[i].sec_end))
            return i;
    return 0;
}

// In milliseconds
u32 CdRomMedia::SeekTime(u32 start_lba, u32 end_lba)
{
    u32 start_index = SeekFindGroup(start_lba);
    u32 target_index = SeekFindGroup(end_lba);
    u32 lba_difference = (u32)std::abs((int)end_lba - (int)start_lba);
    double track_difference = 0.0;

    // Now we find the track difference
    //
    // Note: except for the first and last sector groups, all groups are 1606.48 tracks per group.
    //
    if (target_index == start_index)
    {
        track_difference = (lba_difference / k_seek_sector_list[target_index].sec_per_revolution);
    }
    else if (target_index > start_index)
    {
        track_difference = (k_seek_sector_list[start_index].sec_end - start_lba) / k_seek_sector_list[start_index].sec_per_revolution;
        track_difference += (end_lba - k_seek_sector_list[target_index].sec_start) / k_seek_sector_list[target_index].sec_per_revolution;
        track_difference += (1606.48 * (target_index - start_index - 1));
    }
    else // start_index > target_index
    {
        track_difference = (start_lba - k_seek_sector_list[start_index].sec_start) / k_seek_sector_list[start_index].sec_per_revolution;
        track_difference += (k_seek_sector_list[target_index].sec_end - end_lba) / k_seek_sector_list[target_index].sec_per_revolution;
        track_difference += (1606.48 * (start_index - target_index - 1));
    }

    // Now, we use the algorithm to determine how long to wait
    if (lba_difference < 2)
        return (9 * 1000 / 60);
    if (lba_difference < 5)
        return (9 * 1000 / 60) + (k_seek_sector_list[target_index].rotation_ms / 2);
    else if (track_difference <= 80)
        return (18 * 1000 / 60) + (k_seek_sector_list[target_index].rotation_ms / 2);
    else if (track_difference <= 160)
        return (22 * 1000 / 60) + (k_seek_sector_list[target_index].rotation_ms / 2);
    else if (track_difference <= 644)
        return (22 * 1000 / 60) + (k_seek_sector_list[target_index].rotation_ms / 2) + ((track_difference - 161) * 16.66 / 80);
    else
        return (48 * 1000 / 60) + ((track_difference - 644) * 16.66 / 195);
}