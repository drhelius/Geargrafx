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

#ifndef MMI_ARCHIVE_H
#define MMI_ARCHIVE_H

#include <string>
#include <vector>
#include "common.h"

class MediaFile;

enum GG_MmiStreamRole
{
    GG_MMI_STREAM_UNKNOWN = 0,
    GG_MMI_STREAM_REDBOOK,
    GG_MMI_STREAM_RAW_AUDIO,
    GG_MMI_STREAM_RAW_VIDEO
};

struct GG_MmiEntry
{
    u32 index;
    std::string name;
    std::string normalized_name;
    std::string lookup_name;
    u16 flags;
    u16 method;
    u32 crc32;
    u64 compressed_size;
    u64 uncompressed_size;
    u64 local_header_offset;
    u64 data_offset;
    bool directory;
};

struct GG_MmiStreamInfo
{
    std::string name;
    std::string type;
    std::string file;
    std::string format;
    s64 channels;
    s64 frames_in_active_region;
    s64 frames_in_lead_in_region;
    s64 frames_in_lead_out_region;
    u32 entry_index;
    GG_MmiStreamRole role;
};

struct GG_MmiMediaInfo
{
    std::string name;
    std::string type;
    std::string format;
    s64 sequence_number;
    s64 volume_number;
    s64 side_number;
    std::string physical_type;
    std::string master_reference;
    std::vector<GG_MmiStreamInfo> streams;
    bool laserdisc;
};

struct GG_MmiInfo
{
    std::string name;
    std::string system;
    std::string region_code;
    std::string catalog_id;
    std::string card;
    std::vector<GG_MmiMediaInfo> media;
};

class MmiArchive
{
public:
    MmiArchive();
    ~MmiArchive();

    bool Open(const char* path);
    void Close();
    bool IsOpen() const;
    const char* GetPath() const;
    const char* GetLastError() const;
    u32 GetCRC() const;
    const GG_MmiInfo* GetInfo() const;
    const std::vector<GG_MmiEntry>& GetEntries() const;
    const GG_MmiEntry* GetEntry(u32 index) const;
    const GG_MmiEntry* FindEntry(const char* name) const;
    const GG_MmiEntry* ResolveEntry(const char* base_entry, const char* reference) const;
    MediaFile* OpenStoredEntry(const GG_MmiEntry* entry) const;
    bool ExtractSmallEntry(const GG_MmiEntry* entry, std::vector<u8>& output, u64 max_size);

private:
    MmiArchive(const MmiArchive&);
    MmiArchive& operator=(const MmiArchive&);

    static size_t ReadCallback(void* user_data, mz_uint64 offset, void* buffer, size_t size);
    static std::string ToLowerAscii(const std::string& value);
    static bool NormalizeEntryPath(const char* path, std::string& normalized_path);
    bool ReadEntries();
    bool ReadLocalHeader(GG_MmiEntry& entry);
    bool ValidateEntryRanges();
    bool ParseMediaInfo();
    bool IsSupportedSystem(const std::string& system) const;
    void SetError(const char* format, ...);
    void CloseArchive();

private:
    MediaFile* m_file;
    mz_zip_archive m_archive;
    bool m_archive_open;
    u32 m_crc;
    std::string m_path;
    std::vector<GG_MmiEntry> m_entries;
    std::vector<u32> m_entry_lookup;
    GG_MmiInfo m_mmi_info;
    char m_error[512];
};

#endif /* MMI_ARCHIVE_H */
