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

#include <algorithm>
#include <cstdarg>
#include <limits>
#include "mmi_archive.h"
#include "media_file.h"
#include "media_file_slice.h"

#if UINTPTR_MAX > UINT32_MAX
#include "json.hpp"

using nlohmann::json;

static const u64 k_mmi_max_media_info_size = 16ULL * 1024ULL * 1024ULL;
static const size_t k_mmi_max_media_count = 128;
static const size_t k_mmi_max_stream_count = 64;
#endif

static const u32 k_mmi_max_entries = 64U * 1024U;
static const size_t k_mmi_max_entry_name_length = 4096;
static const size_t k_mmi_max_zip_allocation = 64U * 1024U * 1024U;

static void* mmi_zip_allocate(void* user_data, size_t count, size_t size)
{
    UNUSED(user_data);

    if (size != 0 && count > k_mmi_max_zip_allocation / size)
        return NULL;

    return malloc(count * size);
}

static void* mmi_zip_reallocate(void* user_data, void* memory, size_t count, size_t size)
{
    UNUSED(user_data);

    if (size != 0 && count > k_mmi_max_zip_allocation / size)
        return NULL;

    return realloc(memory, count * size);
}

static void mmi_zip_free(void* user_data, void* memory)
{
    UNUSED(user_data);
    free(memory);
}

#if UINTPTR_MAX > UINT32_MAX
static bool mmi_read_json_string(const json& object, const char* key, std::string& output, bool required, bool allow_empty)
{
    json::const_iterator value = object.find(key);

    if (value == object.end())
    {
        output.clear();
        return !required;
    }

    if (!value->is_string())
        return false;

    output = value->get_ref<const std::string&>();

    if (output.find('\0') != std::string::npos)
        return false;

    return allow_empty || !output.empty();
}

static bool mmi_read_json_integer(const json& object, const char* key, s64& output, bool required = true)
{
    json::const_iterator value = object.find(key);

    if (value == object.end())
    {
        output = 0;
        return !required;
    }

    if (value->is_number_unsigned())
    {
        const json::number_unsigned_t* number = value->get_ptr<const json::number_unsigned_t*>();

        if (!number || (*number > (json::number_unsigned_t)INT64_MAX))
            return false;

        output = (s64)*number;
        return true;
    }

    if (value->is_number_integer())
    {
        const json::number_integer_t* number = value->get_ptr<const json::number_integer_t*>();

        if (!number)
            return false;

        output = (s64)*number;
        return true;
    }

    return false;
}
#endif

MmiArchive::MmiArchive()
{
    InitPointer(m_file);
    mz_zip_zero_struct(&m_archive);
    m_archive_open = false;
    m_crc = 0;
    m_error[0] = 0;
}

MmiArchive::~MmiArchive()
{
    CloseArchive();
}

bool MmiArchive::Open(const char* path)
{
    Close();

    if (sizeof(void*) < 8)
    {
        SetError("MMI requires a 64-bit build");
        return false;
    }

    if (!IsValidPointer(path) || (path[0] == 0))
    {
        SetError("Invalid MMI path");
        return false;
    }

    m_file = MediaFile::OpenFile(path);

    if (!IsValidPointer(m_file))
    {
        SetError("Unable to open MMI file %s", path);
        return false;
    }

    if (!m_file->CanSeek())
    {
#if defined(__LIBRETRO__)
        SetError("MMI requires random-access VFS with 64-bit size/tell/read/seek callbacks");
#else
        SetError("MMI requires random-access file seeking");
#endif

        CloseArchive();
        return false;
    }

    s64 file_size = m_file->GetSize();

    if (file_size <= 0)
    {
        SetError("Invalid MMI file size %lld", (long long)file_size);
        CloseArchive();
        return false;
    }

    mz_zip_zero_struct(&m_archive);
    m_archive.m_pRead = ReadCallback;
    m_archive.m_pIO_opaque = this;
    m_archive.m_pAlloc = mmi_zip_allocate;
    m_archive.m_pRealloc = mmi_zip_reallocate;
    m_archive.m_pFree = mmi_zip_free;

    if (!mz_zip_reader_init(&m_archive, (mz_uint64)file_size, 0))
    {
        SetError("Invalid MMI ZIP container: %s", mz_zip_get_error_string(mz_zip_get_last_error(&m_archive)));
        CloseArchive();
        return false;
    }

    m_archive_open = true;
    m_path = path;

    if (!ReadEntries() || !ParseMediaInfo())
    {
        CloseArchive();
        return false;
    }

    return true;
}

void MmiArchive::Close()
{
    CloseArchive();
    m_error[0] = 0;
}

void MmiArchive::CloseArchive()
{
    if (m_archive_open)
        mz_zip_reader_end(&m_archive);

    mz_zip_zero_struct(&m_archive);
    m_archive_open = false;
    m_crc = 0;
    SafeDelete(m_file);
    m_path.clear();
    m_entries.clear();
    m_entry_lookup.clear();
    m_mmi_info = GG_MmiInfo();
}

bool MmiArchive::IsOpen() const
{
    return m_archive_open && IsValidPointer(m_file);
}

const char* MmiArchive::GetPath() const
{
    return m_path.c_str();
}

const char* MmiArchive::GetLastError() const
{
    return m_error;
}

u32 MmiArchive::GetCRC() const
{
    return m_crc;
}

const GG_MmiInfo* MmiArchive::GetInfo() const
{
    return IsOpen() ? &m_mmi_info : NULL;
}

const std::vector<GG_MmiEntry>& MmiArchive::GetEntries() const
{
    return m_entries;
}

const GG_MmiEntry* MmiArchive::GetEntry(u32 index) const
{
    if (index >= m_entries.size())
        return NULL;

    return &m_entries[index];
}

const GG_MmiEntry* MmiArchive::FindEntry(const char* name) const
{
    std::string normalized_path;

    if (!NormalizeEntryPath(name, normalized_path))
        return NULL;

    std::string lookup_name = ToLowerAscii(normalized_path);

    std::vector<u32>::const_iterator entry_it = std::lower_bound(m_entry_lookup.begin(),
        m_entry_lookup.end(), lookup_name, [this](u32 index, const std::string& name)
    {
        return m_entries[index].lookup_name < name;
    });

    return (entry_it != m_entry_lookup.end() && m_entries[*entry_it].lookup_name == lookup_name) ?
        &m_entries[*entry_it] : NULL;
}

const GG_MmiEntry* MmiArchive::ResolveEntry(const char* base_entry, const char* reference) const
{
    if (!IsValidPointer(base_entry) || !IsValidPointer(reference))
        return NULL;

    std::string base_path;
    std::string relative_path;

    if (!NormalizeEntryPath(base_entry, base_path) || !NormalizeEntryPath(reference, relative_path))
        return NULL;

    size_t separator = base_path.find_last_of('/');
    std::string resolved_path;

    if (separator != std::string::npos)
        resolved_path = base_path.substr(0, separator + 1);

    resolved_path += relative_path;

    return FindEntry(resolved_path.c_str());
}

MediaFile* MmiArchive::OpenStoredEntry(const GG_MmiEntry* entry) const
{
    if (!IsOpen() || !IsValidPointer(entry) || entry->directory || (entry->method != 0))
        return NULL;

    MediaFileSlice* slice = new MediaFileSlice;

    if (!slice->OpenSlice(m_path.c_str(), entry->data_offset, entry->uncompressed_size))
    {
        SafeDelete(slice);
        return NULL;
    }

    return slice;
}

bool MmiArchive::ExtractSmallEntry(const GG_MmiEntry* entry, std::vector<u8>& output, u64 max_size)
{
    output.clear();

    if (!IsOpen() || !IsValidPointer(entry) || entry->directory || (entry->uncompressed_size > max_size) ||
        (entry->uncompressed_size > (u64)SIZE_MAX))
    {
        return false;
    }

    if (entry->uncompressed_size == 0)
        return true;

    output.resize((size_t)entry->uncompressed_size);

    if (!mz_zip_reader_extract_to_mem(&m_archive, entry->index, &output[0], output.size(), 0))
    {
        output.clear();
        return false;
    }

    return true;
}

size_t MmiArchive::ReadCallback(void* user_data, mz_uint64 offset, void* buffer, size_t size)
{
    MmiArchive* archive = reinterpret_cast<MmiArchive*> (user_data);

    if (!IsValidPointer(archive) || !IsValidPointer(archive->m_file))
        return 0;

    if (!archive->m_file->ReadAt((u64)offset, buffer, (u64)size))
        return 0;

    return size;
}

std::string MmiArchive::ToLowerAscii(const std::string& value)
{
    std::string result = value;

    for (size_t i = 0; i < result.length(); i++)
    {
        unsigned char character = (unsigned char)result[i];

        if ((character >= 'A') && (character <= 'Z'))
            result[i] = (char)(character - 'A' + 'a');
    }

    return result;
}

bool MmiArchive::NormalizeEntryPath(const char* path, std::string& normalized_path)
{
    normalized_path.clear();

    if (!IsValidPointer(path) || (path[0] == 0) || (path[0] == '/') || (path[0] == '\\'))
        return false;

    if (path[1] == ':')
        return false;

    std::string entry_path(path);

    if (entry_path.length() > k_mmi_max_entry_name_length)
        return false;

    std::string component;

    for (size_t i = 0; i <= entry_path.length(); i++)
    {
        char character = (i < entry_path.length()) ? entry_path[i] : '/';

        if ((character == '/') || (character == '\\'))
        {
            if (component.empty() || (component == "."))
            {
                component.clear();
                continue;
            }

            if ((component == "..") || (component.find(':') != std::string::npos))
                return false;

            if (!normalized_path.empty())
                normalized_path += '/';

            normalized_path += component;
            component.clear();
        }
        else
        {
            component += character;
        }
    }

    return !normalized_path.empty();
}

bool MmiArchive::ReadEntries()
{
    mz_uint count = mz_zip_reader_get_num_files(&m_archive);

    if ((count == 0) || (count > k_mmi_max_entries))
    {
        SetError("Invalid MMI entry count %u", count);
        return false;
    }

    m_entries.reserve(count);

    for (mz_uint i = 0; i < count; i++)
    {
        mz_zip_archive_file_stat file_stat;

        memset(&file_stat, 0, sizeof(file_stat));

        if (!mz_zip_reader_file_stat(&m_archive, i, &file_stat))
        {
            SetError("Unable to read MMI ZIP entry %u", i);
            return false;
        }

        mz_uint name_size = mz_zip_reader_get_filename(&m_archive, i, NULL, 0);

        if ((name_size < 2) || (name_size > (k_mmi_max_entry_name_length + 1)))
        {
            SetError("Invalid MMI ZIP entry name at index %u", i);
            return false;
        }

        std::vector<char> name(name_size);

        if (mz_zip_reader_get_filename(&m_archive, i, &name[0], name_size) != name_size)
        {
            SetError("Unable to read MMI ZIP entry name at index %u", i);
            return false;
        }

        GG_MmiEntry entry;

        entry.index = i;
        entry.name = &name[0];

        if (entry.name.length() != name_size - 1)
        {
            SetError("Embedded NUL in MMI ZIP entry name at index %u", i);
            return false;
        }

        entry.flags = file_stat.m_bit_flag;
        entry.method = file_stat.m_method;
        entry.crc32 = file_stat.m_crc32;
        entry.compressed_size = file_stat.m_comp_size;
        entry.uncompressed_size = file_stat.m_uncomp_size;
        entry.local_header_offset = file_stat.m_local_header_ofs;
        entry.data_offset = 0;
        entry.directory = file_stat.m_is_directory != 0;

        if (!NormalizeEntryPath(entry.name.c_str(), entry.normalized_name))
        {
            SetError("Unsafe MMI ZIP entry path: %s", entry.name.c_str());
            return false;
        }

        entry.lookup_name = ToLowerAscii(entry.normalized_name);

        if (file_stat.m_is_encrypted || (entry.flags & 0x0001) || (entry.flags & 0x0040))
        {
            SetError("Encrypted MMI ZIP entry is unsupported: %s", entry.name.c_str());
            return false;
        }

        if (!entry.directory && (entry.method != 0) && (entry.method != 8))
        {
            SetError("Unsupported MMI ZIP compression method %u: %s", entry.method, entry.name.c_str());
            return false;
        }

        if (!entry.directory && !file_stat.m_is_supported)
        {
            SetError("Unsupported MMI ZIP entry: %s", entry.name.c_str());
            return false;
        }

        if ((entry.method == 0) && (entry.compressed_size != entry.uncompressed_size))
        {
            SetError("Invalid stored MMI ZIP entry size: %s", entry.name.c_str());
            return false;
        }

        if (!ReadLocalHeader(entry))
            return false;

        m_entries.push_back(entry);
        m_entry_lookup.push_back(i);

        u8 entry_crc_data[12];

        write_u32_le(entry_crc_data, entry.crc32);
        write_u32_le(entry_crc_data + 4, (u32)entry.uncompressed_size);
        write_u32_le(entry_crc_data + 8, (u32)(entry.uncompressed_size >> 32));
        m_crc = (u32)mz_crc32(m_crc, (const u8*)entry.name.data(), entry.name.size());
        m_crc = (u32)mz_crc32(m_crc, entry_crc_data, sizeof(entry_crc_data));
    }

    std::sort(m_entry_lookup.begin(), m_entry_lookup.end(), [this](u32 left, u32 right)
    {
        return m_entries[left].lookup_name < m_entries[right].lookup_name;
    });

    for (size_t i = 1; i < m_entry_lookup.size(); i++)
    {
        if (m_entries[m_entry_lookup[i - 1]].lookup_name == m_entries[m_entry_lookup[i]].lookup_name)
        {
            SetError("Duplicate MMI ZIP entry name: %s", m_entries[m_entry_lookup[i]].name.c_str());
            return false;
        }
    }

    return ValidateEntryRanges();
}

bool MmiArchive::ReadLocalHeader(GG_MmiEntry& entry)
{
    u8 header[30];

    if (!m_file->ReadAt(entry.local_header_offset, header, sizeof(header)))
    {
        SetError("Truncated MMI ZIP local header: %s", entry.name.c_str());
        return false;
    }

    if (read_u32_le(header) != 0x04034B50)
    {
        SetError("Invalid MMI ZIP local header: %s", entry.name.c_str());
        return false;
    }

    u16 local_flags = read_u16_le(header + 6);
    u16 local_method = read_u16_le(header + 8);
    u16 name_length = read_u16_le(header + 26);
    u16 extra_length = read_u16_le(header + 28);

    if ((local_flags != entry.flags) || (local_method != entry.method))
    {
        SetError("MMI ZIP local/central header mismatch: %s", entry.name.c_str());
        return false;
    }

    u64 data_offset;

    if (!checked_add_u64(entry.local_header_offset, sizeof(header), &data_offset) ||
        !checked_add_u64(data_offset, name_length, &data_offset) ||
        !checked_add_u64(data_offset, extra_length, &data_offset))
    {
        SetError("MMI ZIP local header overflow: %s", entry.name.c_str());
        return false;
    }

    u64 data_end;

    if (!checked_add_u64(data_offset, entry.compressed_size, &data_end) ||
        (data_end > (u64)m_file->GetSize()) ||
        (data_end > m_archive.m_central_directory_file_ofs))
    {
        SetError("MMI ZIP entry payload is out of range: %s", entry.name.c_str());
        return false;
    }

    if (name_length != entry.name.length())
    {
        SetError("MMI ZIP local filename mismatch: %s", entry.name.c_str());
        return false;
    }

    std::vector<char> local_name((size_t)name_length + 1, 0);

    if ((name_length != 0) && !m_file->ReadAt(entry.local_header_offset + sizeof(header), &local_name[0], name_length))
    {
        SetError("Truncated MMI ZIP local filename: %s", entry.name.c_str());
        return false;
    }

    if (entry.name != &local_name[0])
    {
        SetError("MMI ZIP local filename mismatch: %s", entry.name.c_str());
        return false;
    }

    entry.data_offset = data_offset;

    return true;
}

bool MmiArchive::ValidateEntryRanges()
{
    struct EntryRange
    {
        u64 start_offset;
        u64 end_offset;
        size_t entry_index;
    };

    std::vector<EntryRange> ranges;

    for (size_t i = 0; i < m_entries.size(); i++)
    {
        EntryRange range;

        range.start_offset = m_entries[i].local_header_offset;

        if (!checked_add_u64(m_entries[i].data_offset, m_entries[i].compressed_size, &range.end_offset))
        {
            SetError("MMI ZIP entry range overflow: %s", m_entries[i].name.c_str());
            return false;
        }

        range.entry_index = i;
        ranges.push_back(range);
    }

    std::sort(ranges.begin(), ranges.end(), [](const EntryRange& left, const EntryRange& right)
    {
        return left.start_offset < right.start_offset;
    });

    for (size_t i = 1; i < ranges.size(); i++)
    {
        if (ranges[i].start_offset < ranges[i - 1].end_offset)
        {
            SetError("Overlapping MMI ZIP entries: %s and %s",
                m_entries[ranges[i - 1].entry_index].name.c_str(), m_entries[ranges[i].entry_index].name.c_str());
            return false;
        }
    }

    return true;
}

bool MmiArchive::ParseMediaInfo()
{
#if UINTPTR_MAX > UINT32_MAX
    const GG_MmiEntry* media_info_entry = FindEntry("MediaInfo.json");

    if (!media_info_entry)
    {
        SetError("MMI is missing MediaInfo.json");
        return false;
    }

    std::vector<u8> json_data;

    if (!ExtractSmallEntry(media_info_entry, json_data, k_mmi_max_media_info_size) || json_data.empty())
    {
        SetError("Unable to extract bounded MediaInfo.json");
        return false;
    }

    json root = json::parse(json_data.begin(), json_data.end(), NULL, false);

    if (root.is_discarded() || !root.is_object())
    {
        SetError("Invalid MediaInfo.json syntax");
        return false;
    }

    GG_MmiInfo mmi_info;

    if (!mmi_read_json_string(root, "name", mmi_info.name, true, false) ||
        !mmi_read_json_string(root, "system", mmi_info.system, true, false) ||
        !mmi_read_json_string(root, "regionCode", mmi_info.region_code, false, true) ||
        !mmi_read_json_string(root, "catalogId", mmi_info.catalog_id, false, true) ||
        !mmi_read_json_string(root, "card", mmi_info.card, false, true))
    {
        SetError("Invalid MediaInfo.json root fields");
        return false;
    }

    if (!IsSupportedSystem(mmi_info.system))
    {
        SetError("Unsupported MMI system: %s", mmi_info.system.c_str());
        return false;
    }

    if (!mmi_info.card.empty() && (mmi_info.card != "System Card 1.0") && (mmi_info.card != "Games Express"))
    {
        SetError("Unsupported MMI HuCard requirement: %s", mmi_info.card.c_str());
        return false;
    }

    json::const_iterator media_array = root.find("media");

    if ((media_array == root.end()) || !media_array->is_array() || media_array->empty() ||
        (media_array->size() > k_mmi_max_media_count))
    {
        SetError("Invalid MediaInfo.json media array");
        return false;
    }

    for (size_t media_index = 0; media_index < media_array->size(); media_index++)
    {
        const json& media_json = (*media_array)[media_index];
        GG_MmiMediaInfo media;

        media.laserdisc = false;

        if (!media_json.is_object() ||
            !mmi_read_json_string(media_json, "name", media.name, true, false) ||
            !mmi_read_json_string(media_json, "type", media.type, true, false) ||
            !mmi_read_json_string(media_json, "format", media.format, true, false) ||
            !mmi_read_json_integer(media_json, "sequenceNo", media.sequence_number) ||
            !mmi_read_json_integer(media_json, "volumeNo", media.volume_number) ||
            !mmi_read_json_integer(media_json, "sideNo", media.side_number) ||
            !mmi_read_json_string(media_json, "physicalType", media.physical_type, false, true) ||
            !mmi_read_json_string(media_json, "masterReference", media.master_reference, false, true) ||
            (media.sequence_number < 0) || (media.volume_number < 0) || (media.side_number < 0))
        {
            SetError("Invalid MediaInfo.json media item %u", (unsigned)media_index);
            return false;
        }

        json::const_iterator streams_array = media_json.find("streams");

        if ((streams_array == media_json.end()) || !streams_array->is_array() || streams_array->empty() ||
            (streams_array->size() > k_mmi_max_stream_count))
        {
            SetError("Invalid streams for MMI media %s", media.name.c_str());
            return false;
        }

        bool stream_roles[4] = { false, false, false, false };

        for (size_t stream_index = 0; stream_index < streams_array->size(); stream_index++)
        {
            const json& stream_json = (*streams_array)[stream_index];
            GG_MmiStreamInfo stream;

            stream.entry_index = UINT32_MAX;
            stream.role = GG_MMI_STREAM_UNKNOWN;

            if (!stream_json.is_object() ||
                !mmi_read_json_string(stream_json, "name", stream.name, true, false) ||
                !mmi_read_json_string(stream_json, "type", stream.type, true, false) ||
                !mmi_read_json_string(stream_json, "file", stream.file, true, false) ||
                !mmi_read_json_string(stream_json, "format", stream.format, false, true) ||
                !mmi_read_json_integer(stream_json, "channels", stream.channels, false) ||
                !mmi_read_json_integer(stream_json, "framesInActiveRegion", stream.frames_in_active_region, false) ||
                !mmi_read_json_integer(stream_json, "framesInLeadInRegion", stream.frames_in_lead_in_region, false) ||
                !mmi_read_json_integer(stream_json, "framesInLeadOutRegion", stream.frames_in_lead_out_region, false) ||
                (stream.channels < 0) || (stream.frames_in_active_region < 0) ||
                (stream.frames_in_lead_in_region < 0) || (stream.frames_in_lead_out_region < 0))
            {
                SetError("Invalid stream %u for MMI media %s", (unsigned)stream_index, media.name.c_str());
                return false;
            }

            std::string role = ToLowerAscii(stream.type);

            if (role == "redbook")
                stream.role = GG_MMI_STREAM_REDBOOK;
            else if (role == "rawaudio")
                stream.role = GG_MMI_STREAM_RAW_AUDIO;
            else if (role == "rawvideo")
                stream.role = GG_MMI_STREAM_RAW_VIDEO;

            bool has_channels = stream_json.find("channels") != stream_json.end();

            if (!has_channels && stream.role == GG_MMI_STREAM_RAW_AUDIO)
                stream.channels = 2;

            if (((stream.role == GG_MMI_STREAM_RAW_AUDIO) && (stream.channels != 2)) ||
                ((stream.role == GG_MMI_STREAM_RAW_VIDEO) && has_channels &&
                (stream.channels != 3) && (stream.channels != 4)))
            {
                SetError("Unsupported channel count %lld for MMI stream %s",
                    (long long)stream.channels, stream.file.c_str());
                return false;
            }

            if (stream.role == GG_MMI_STREAM_RAW_VIDEO &&
                (!stream_json.count("framesInActiveRegion") ||
                !stream_json.count("framesInLeadInRegion") ||
                !stream_json.count("framesInLeadOutRegion") || stream.frames_in_active_region == 0))
            {
                SetError("Missing video region counts in MMI stream %s", stream.file.c_str());
                return false;
            }

            if (stream.role != GG_MMI_STREAM_UNKNOWN)
            {
                if (stream_roles[stream.role])
                {
                    SetError("Duplicate %s stream for MMI media %s", stream.type.c_str(), media.name.c_str());
                    return false;
                }

                stream_roles[stream.role] = true;
            }

            const GG_MmiEntry* entry = FindEntry(stream.file.c_str());

            if (!entry || entry->directory)
            {
                SetError("Missing MMI stream entry %s", stream.file.c_str());
                return false;
            }

            stream.entry_index = entry->index;

            u64 total_frames;

            if (!checked_add_u64((u64)stream.frames_in_lead_in_region,
                (u64)stream.frames_in_active_region, &total_frames) ||
                !checked_add_u64(total_frames, (u64)stream.frames_in_lead_out_region, &total_frames))
            {
                SetError("MMI stream frame count overflow: %s", stream.file.c_str());
                return false;
            }

            media.streams.push_back(stream);
        }

        bool laserdisc_type = ToLowerAscii(media.type) == "ld";
        bool has_analog_stream = stream_roles[GG_MMI_STREAM_RAW_AUDIO] || stream_roles[GG_MMI_STREAM_RAW_VIDEO];

        media.laserdisc = laserdisc_type && has_analog_stream;

        if (!stream_roles[GG_MMI_STREAM_REDBOOK] ||
            (laserdisc_type && (!stream_roles[GG_MMI_STREAM_RAW_AUDIO] || !stream_roles[GG_MMI_STREAM_RAW_VIDEO])) ||
            (!laserdisc_type && has_analog_stream))
        {
            SetError("MMI media %s does not contain one complete Redbook/analog stream set", media.name.c_str());
            return false;
        }

        mmi_info.media.push_back(media);
    }

    std::sort(mmi_info.media.begin(), mmi_info.media.end(),
        [](const GG_MmiMediaInfo& left, const GG_MmiMediaInfo& right)
    {
        return left.sequence_number < right.sequence_number;
    });

    for (size_t i = 1; i < mmi_info.media.size(); i++)
    {
        if (mmi_info.media[i - 1].sequence_number == mmi_info.media[i].sequence_number)
        {
            SetError("Duplicate MMI media sequence number %lld", (long long)mmi_info.media[i].sequence_number);
            return false;
        }
    }

    m_mmi_info = mmi_info;

    return true;
#else
    SetError("MMI requires a 64-bit build");
    return false;
#endif
}

bool MmiArchive::IsSupportedSystem(const std::string& system) const
{
    return (system == "LDROM2") || (system == "LD") || (system == "CDROM2") ||
        (system == "SuperCDROM2") || (system == "ArcadeCDROM2") || (system == "CD");
}

void MmiArchive::SetError(const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(m_error, sizeof(m_error), format, arguments);
    va_end(arguments);
    m_error[sizeof(m_error) - 1] = 0;
    Error("%s", m_error);
}
