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

#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include "cdrom_media.h"

CdRomMedia::CdRomMedia()
{
    m_ready = false;
    m_file_path[0] = 0;
    m_file_directory[0] = 0;
    m_file_name[0] = 0;
    m_file_extension[0] = 0;
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
        if (img_file)
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
    m_file_path[0] = 0;
    m_file_directory[0] = 0;
    m_file_name[0] = 0;
    m_file_extension[0] = 0;
    m_tracks.clear();
    m_img_files.clear();
}

bool CdRomMedia::IsReady()
{
    return m_ready;
}

const char* CdRomMedia::GetFilePath()
{
    return m_file_path;
}

const char* CdRomMedia::GetFileDirectory()
{
    return m_file_directory;
}

const char* CdRomMedia::GetFileName()
{
    return m_file_name;
}

const char* CdRomMedia::GetFileExtension()
{
    return m_file_extension;
}

const std::vector<CdRomMedia::Track>& CdRomMedia::GetTracks()
{
    return m_tracks;
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
        int size = static_cast<int>(file.tellg());

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
        Log("Loading CD-ROM Media from buffer... Size: %d", size);

        Reset();

        if (IsValidPointer(path))
        {
            GatherPaths(path);
        }

        char* cue_content = new char[size + 1];
        memcpy(cue_content, buffer, size);
        cue_content[size] = 0;

        bool result = ParseCueFile(cue_content);

        SafeDeleteArray(cue_content);

        if (result)
        {
            m_ready = true;
            Debug("CD-ROM Media loaded from buffer. Track count: %d", m_tracks.size());
        }
        else
        {
            Log("ERROR: Failed to parse CUE file");
            m_ready = false;
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
    using namespace std;

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

    ifstream file(img_file->file_path, ios::in | ios::binary | ios::ate);

    if (file.is_open())
    {
        int size = static_cast<int>(file.tellg());

        if (size <= 0)
        {
            Log("ERROR: Unable to open file %s. Size: %d", img_file->file_path, size);
            return false;
        }

        if (file.bad() || file.fail() || !file.good() || file.eof())
        {
            Log("ERROR: Unable to open file %s. Bad file!", img_file->file_path);
            return false;
        }

        file.close();

        img_file->file_size = (u32)size;
        img_file->chunk_size = CDROM_MEDIA_CHUNK_SIZE;
        img_file->chunk_count = img_file->file_size / img_file->chunk_size;
        if (img_file->file_size % img_file->chunk_size != 0)
            img_file->chunk_count++;
        img_file->chunks = new u8*[img_file->chunk_count];

        for (u32 i = 0; i < img_file->chunk_count; i++)
            InitPointer(img_file->chunks[i]);

        Debug("Gathered ImgFile info: %s", img_file->file_path);
        Debug("ImgFile info Size: %d, Chunk size: %d, Chunk count: %d", img_file->file_size, img_file->chunk_size, img_file->chunk_count);
    }
    else
    {
        Log("ERROR: Unable to open file %s", img_file->file_path);
        return false;
    }

    Debug("Gathered ImgFile info for %s", img_file->file_path);

    return true;
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
    ImgFile* current_img_file;
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
            size_t first_quote = line.find_first_of("\"");
            size_t last_quote = line.find_last_of("\"");

            if (first_quote != string::npos && last_quote != string::npos && first_quote != last_quote)
            {
                string current_file_path = line.substr(first_quote + 1, last_quote - first_quote - 1);

                if (!current_file_path.empty() && current_file_path[0] != '/' && current_file_path[0] != '\\' &&
                    (current_file_path.size() < 2 || current_file_path[1] != ':'))
                {
                    current_file_path = string(m_file_directory) + "/" + current_file_path;
                }

                Debug("Found FILE: %s", current_file_path.c_str());

                ImgFile* img_file = new ImgFile();
                strncpy(img_file->file_path, current_file_path.c_str(), sizeof(img_file->file_path));
                if (!GatherImgInfo(img_file))
                {
                    Log("ERROR: Failed to gather ImgFile info for %s", current_file_path.c_str());
                    SafeDelete(img_file);
                    return false;
                }
                m_img_files.push_back(img_file);
                current_img_file = img_file;
            }
            else
            {
                Log("ERROR: Invalid FILE format in CUE: %s", line.c_str());
                return false;
            }
        }
        else if (lowercase_line.find("track") == 0)
        {
            if (in_track)
                m_tracks.push_back(current_track);

            in_track = true;
            current_track = Track();
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

            if (index_number == 1)
            {
                int minutes = 0, seconds = 0, frames = 0;
                char colon1, colon2;

                if (!(index_stream >> minutes >> colon1 >> seconds >> colon2 >> frames) ||
                    colon1 != ':' || colon2 != ':' ||
                    minutes < 0 || seconds < 0 || frames < 0 ||
                    seconds >= 60 || frames >= 75)
                {
                    Log("ERROR: Invalid time format in INDEX entry");
                    continue;
                }

                current_track.start_position.minutes = minutes;
                current_track.start_position.seconds = seconds;
                current_track.start_position.frames = frames;

                Debug("Track %d starts at %02d:%02d:%02d", current_track.number, minutes, seconds, frames);
            }
        }
    }

    if (in_track)
    {
        m_tracks.push_back(current_track);
    }

    // Calculate track lengths (difference between start frames)
    // if (m_tracks.size() > 1)
    // {
    //     for (size_t i = 0; i < m_tracks.size() - 1; i++)
    //     {
    //         m_tracks[i].length_frames = m_tracks[i + 1].start_lba - m_tracks[i].start_lba;
    //     }

    //     // For the last track, we can't calculate exact length without reading the file
    //     // For now, just set a large value
    //     if (!m_tracks.empty())
    //         m_tracks.back().length_frames = 75 * 60 * 80; // 80 minutes worth of frames
    // }
    // else if (m_tracks.size() == 1)
    // {
    //     // For a single track, set a default large value
    //     m_tracks[0].length_frames = 75 * 60 * 80; // 80 minutes worth of frames
    // }

    Log("Successfully parsed CUE file with %d tracks", m_tracks.size());
    return !m_tracks.empty();
}
