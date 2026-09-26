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
#include "cdrom_mmi_image.h"
#include "media_file.h"
#include "../platforms/shared/dependencies/qon/qoi2.h"

static const u64 k_mmi_max_cue_size = 16ULL * 1024ULL * 1024ULL;
static const u32 k_qon_max_dimension = 4096;
static const u32 k_qon_max_frame_count = 2U * 1024U * 1024U;
static const u64 k_qon_max_decoded_rgb_size = 64ULL * 1024ULL * 1024ULL;
static const u16 k_qon_interframe_file_flag = 0x0001;
static const u16 k_qon_interframe_frame_flag = 0x8000;
static const u64 k_mmi_subchannel_record_size = 96;

static u16 mmi_subchannel_q_crc(const u8* data)
{
    u16 crc = 0;

    for (u32 i = 0; i < 10; i++)
    {
        crc ^= (u16)data[i] << 8;

        for (u32 bit = 0; bit < 8; bit++)
            crc = (crc & 0x8000) ? (u16)((crc << 1) ^ 0x1021) : (u16)(crc << 1);
    }

    return (u16)~crc;
}

CdRomMmiImage::CdRomMmiImage() : CdRomCueBinImage()
{
    InitPointer(m_analog_audio_file);
    InitPointer(m_video_file);
    InitPointer(m_subchannel_file);
    m_selected_media_index = 0;
    m_laserdisc = false;
    m_ejected = false;
    m_subchannel_first_lba = 0;
    m_subchannel_sector_count = 0;
}

CdRomMmiImage::~CdRomMmiImage()
{
    Reset();
}

void CdRomMmiImage::Init()
{
    CdRomCueBinImage::Init();
    Reset();
}

void CdRomMmiImage::Reset()
{
    CdRomCueBinImage::Reset();
    ResetSelectedMedia();
    m_archive.Close();
}

void CdRomMmiImage::ResetSelectedMedia()
{
    SafeDelete(m_analog_audio_file);
    SafeDelete(m_video_file);
    SafeDelete(m_subchannel_file);
    m_qon_info = GG_QonInfo();
    m_selected_media_index = 0;
    m_selected_cue_path.clear();
    m_laserdisc = false;
    m_ejected = false;
    m_subchannel_first_lba = 0;
    m_subchannel_sector_count = 0;
}

bool CdRomMmiImage::LoadFromFile(const char* path, bool preload)
{
    UNUSED(preload);
    Reset();

    if (!IsValidPointer(path))
        return false;

    GatherPaths(path);

    if (strcmp(m_file_extension, "mmi") != 0)
    {
        Error("Invalid file extension %s. Expected .mmi", m_file_extension);
        return false;
    }

    if (!m_archive.Open(path))
    {
        Error("Failed to open MMI %s: %s", path, m_archive.GetLastError());
        Reset();
        return false;
    }

    const GG_MmiInfo* mmi_info = m_archive.GetInfo();

    if (!mmi_info || mmi_info->media.empty() || !LoadSelectedMedia(0))
    {
        Reset();
        return false;
    }

    return true;
}

bool CdRomMmiImage::ReadSector(u32 lba, u8* buffer)
{
    if (m_ejected)
        return false;

    return CdRomCueBinImage::ReadSector(lba, buffer);
}

bool CdRomMmiImage::ReadSamples(u32 lba, u32 offset, s16* buffer, u32 count)
{
    if (m_ejected)
        return false;

    return CdRomCueBinImage::ReadSamples(lba, offset, buffer, count);
}

bool CdRomMmiImage::ReadSubchannelQ(s32 lba, u8* buffer)
{
    if (m_ejected || !IsValidPointer(m_subchannel_file) || !IsValidPointer(buffer))
        return false;

    s64 record_index = (s64)lba - m_subchannel_first_lba;

    if ((record_index < 0) || ((u64)record_index >= m_subchannel_sector_count))
        return false;

    u8 raw_subchannel[k_mmi_subchannel_record_size];
    u64 offset;

    if (!checked_multiply_u64((u64)record_index, k_mmi_subchannel_record_size, &offset) ||
        !m_subchannel_file->ReadAt(offset, raw_subchannel, sizeof(raw_subchannel)))
    {
        return false;
    }

    return DecodeSubchannelQ(raw_subchannel, buffer);
}

bool CdRomMmiImage::SelectMediaByIndex(u32 index)
{
    const GG_MmiInfo* mmi_info = m_archive.GetInfo();

    if (!mmi_info || (index >= mmi_info->media.size()))
        return false;

    if (mmi_info->media[index].laserdisc != m_laserdisc)
    {
        Error("MMI media selection cannot change the machine type without a reset");
        return false;
    }

    if ((index == m_selected_media_index) && IsReady())
        return true;

    u32 previous_index = m_selected_media_index;
    bool previous_ejected = m_ejected;

    if (LoadSelectedMedia(index))
        return true;

    if (IsReady())
        return false;

    Error("MMI media selection failed; restoring media index %u", previous_index);

    if (LoadSelectedMedia(previous_index))
        m_ejected = previous_ejected;

    return false;
}

bool CdRomMmiImage::SelectMediaBySequence(s64 sequence_number)
{
    const GG_MmiInfo* mmi_info = m_archive.GetInfo();

    if (!mmi_info)
        return false;

    for (size_t i = 0; i < mmi_info->media.size(); i++)
    {
        if (mmi_info->media[i].sequence_number == sequence_number)
            return SelectMediaByIndex((u32)i);
    }

    return false;
}

u32 CdRomMmiImage::GetSelectedMediaIndex() const
{
    return m_selected_media_index;
}

const GG_MmiInfo* CdRomMmiImage::GetMmiInfo() const
{
    return m_archive.GetInfo();
}

const GG_MmiMediaInfo* CdRomMmiImage::GetSelectedMedia() const
{
    const GG_MmiInfo* mmi_info = m_archive.GetInfo();

    if (!mmi_info || (m_selected_media_index >= mmi_info->media.size()))
        return NULL;

    return &mmi_info->media[m_selected_media_index];
}

const GG_QonInfo* CdRomMmiImage::GetQonInfo() const
{
    return m_laserdisc ? &m_qon_info : NULL;
}

bool CdRomMmiImage::IsLaserDisc() const
{
    return m_laserdisc;
}

bool CdRomMmiImage::IsEjected() const
{
    return m_ejected;
}

void CdRomMmiImage::SetEjected(bool ejected)
{
    m_ejected = ejected;
}

bool CdRomMmiImage::ReadAnalogAudio(u64 offset, void* buffer, u32 size)
{
    return !m_ejected && IsValidPointer(m_analog_audio_file) && m_analog_audio_file->ReadAt(offset, buffer, size);
}

u64 CdRomMmiImage::GetAnalogAudioSize() const
{
    if (!IsValidPointer(m_analog_audio_file))
        return 0;

    s64 size = m_analog_audio_file->GetSize();

    return size > 0 ? (u64)size : 0;
}

bool CdRomMmiImage::ReadVideoData(u64 offset, void* buffer, u32 size)
{
    return !m_ejected && IsValidPointer(m_video_file) && m_video_file->ReadAt(offset, buffer, size);
}

MediaFile* CdRomMmiImage::OpenVideoStream() const
{
    const GG_MmiMediaInfo* media = GetSelectedMedia();

    if (!media)
        return NULL;

    const GG_MmiStreamInfo* stream = FindStream(*media, GG_MMI_STREAM_RAW_VIDEO);
    const GG_MmiEntry* entry = stream ? m_archive.GetEntry(stream->entry_index) : NULL;

    return entry ? m_archive.OpenStoredEntry(entry) : NULL;
}

bool CdRomMmiImage::DecodeQonFrame(u32 frame_index, std::vector<u8>& output)
{
    output.clear();

    if (!m_video_file || (frame_index >= m_qon_info.frames.size()) || (m_qon_info.decoded_rgb_size > (u64)SIZE_MAX))
    {
        return false;
    }

    const GG_QonFrameInfo& frame_info = m_qon_info.frames[frame_index];
    std::vector<u8> compressed_data(frame_info.compressed_size);

    if (compressed_data.empty() || !m_video_file->ReadAt(frame_info.record_offset + 4,
        &compressed_data[0], compressed_data.size()))
    {
        return false;
    }

    output.resize((size_t)m_qon_info.decoded_rgb_size);

    qoi2_desc image_desc;

    image_desc.width = m_qon_info.width;
    image_desc.height = m_qon_info.height;
    image_desc.channels = m_qon_info.channels;
    image_desc.colorspace = m_qon_info.colorspace;

    if (!qoi2_decode_data(&compressed_data[0], compressed_data.size(), &image_desc, NULL, &output[0], 3))
    {
        output.clear();
        return false;
    }

    return true;
}

MediaFile* CdRomMmiImage::ResolveMmiFile(const char* reference, char* resolved_path,
    size_t resolved_path_size, void* user_data)
{
    CdRomMmiImage* image = reinterpret_cast<CdRomMmiImage*> (user_data);

    if (!IsValidPointer(image) || !IsValidPointer(reference) || !IsValidPointer(resolved_path) ||
        (resolved_path_size == 0))
    {
        return NULL;
    }

    const GG_MmiEntry* entry = image->m_archive.ResolveEntry(image->m_selected_cue_path.c_str(), reference);

    if (!entry)
        return NULL;

    if (entry->method != 0)
    {
        Error("MMI Redbook bulk entry must be stored: %s", entry->name.c_str());
        return NULL;
    }

    std::string display_path = image->m_archive.GetPath();

    display_path += "::";
    display_path += entry->normalized_name;
    strncpy_fit(resolved_path, display_path.c_str(), resolved_path_size);

    return image->m_archive.OpenStoredEntry(entry);
}

bool CdRomMmiImage::LoadSelectedMedia(u32 index)
{
    const GG_MmiInfo* mmi_info = m_archive.GetInfo();

    if (!mmi_info || (index >= mmi_info->media.size()))
        return false;

    const GG_MmiMediaInfo& media = mmi_info->media[index];
    const GG_MmiStreamInfo* redbook_stream = FindStream(media, GG_MMI_STREAM_REDBOOK);
    const GG_MmiStreamInfo* analog_audio_stream = FindStream(media, GG_MMI_STREAM_RAW_AUDIO);
    const GG_MmiStreamInfo* analog_video_stream = FindStream(media, GG_MMI_STREAM_RAW_VIDEO);

    if (!redbook_stream)
        return false;

    const GG_MmiEntry* cue_entry = m_archive.GetEntry(redbook_stream->entry_index);

    if (!cue_entry)
        return false;

    std::vector<u8> cue_data;

    if (!m_archive.ExtractSmallEntry(cue_entry, cue_data, k_mmi_max_cue_size) || cue_data.empty())
    {
        Error("Unable to extract MMI CUE entry %s", cue_entry->name.c_str());
        return false;
    }

    MediaFile* new_audio_file = NULL;
    MediaFile* new_video_file = NULL;
    MediaFile* new_subchannel_file = NULL;
    s32 new_subchannel_first_lba = 0;
    GG_QonInfo new_qon_info = GG_QonInfo();

    if (media.laserdisc)
    {
        const GG_MmiEntry* audio_entry = analog_audio_stream ?
            m_archive.GetEntry(analog_audio_stream->entry_index) : NULL;
        const GG_MmiEntry* video_entry = analog_video_stream ?
            m_archive.GetEntry(analog_video_stream->entry_index) : NULL;

        if (!audio_entry || !video_entry || (audio_entry->method != 0) || (video_entry->method != 0) ||
            ((audio_entry->uncompressed_size & 3) != 0))
        {
            Error("MMI analog audio/video streams must be seekable stored entries");
            return false;
        }

        new_audio_file = m_archive.OpenStoredEntry(audio_entry);
        new_video_file = m_archive.OpenStoredEntry(video_entry);

        if (!new_audio_file || !new_video_file || !ReadQonIndex(new_video_file, *analog_video_stream, new_qon_info))
        {
            SafeDelete(new_audio_file);
            SafeDelete(new_video_file);
            return false;
        }
    }

    GG_CdRomCueBinLoadOptions load_options = GG_CdRomCueBinStreamingLoadOptions();

    load_options.chunk_size = 2352;
    load_options.allow_disc_preload = false;
    load_options.max_cached_chunks = 8;
    SetLoadOptions(load_options);
    m_selected_cue_path = cue_entry->normalized_name;

    if (!LoadFromCueData(m_archive.GetPath(), &cue_data[0], cue_data.size(), false, ResolveMmiFile, this))
    {
        SafeDelete(new_audio_file);
        SafeDelete(new_video_file);
        m_selected_cue_path.clear();
        return false;
    }

    if (m_toc.sector_count == 0 || m_toc.sector_count > (u32)INT32_MAX - 150)
    {
        Error("MMI Redbook sector count exceeds the signed transport range");
        SafeDelete(new_audio_file);
        SafeDelete(new_video_file);
        CdRomCueBinImage::Reset();
        return false;
    }

    const GG_MmiEntry* subchannel_entry = NULL;

    if (!FindSubchannelEntry(cue_entry, subchannel_entry))
    {
        SafeDelete(new_audio_file);
        SafeDelete(new_video_file);
        m_selected_cue_path.clear();
        CdRomCueBinImage::Reset();
        return false;
    }

    if (subchannel_entry && !OpenSubchannel(subchannel_entry, new_subchannel_file, new_subchannel_first_lba))
    {
        SafeDelete(new_audio_file);
        SafeDelete(new_video_file);
        m_selected_cue_path.clear();
        CdRomCueBinImage::Reset();
        return false;
    }

    SafeDelete(m_analog_audio_file);
    SafeDelete(m_video_file);
    SafeDelete(m_subchannel_file);
    m_analog_audio_file = new_audio_file;
    m_video_file = new_video_file;
    m_subchannel_file = new_subchannel_file;
    m_qon_info = new_qon_info;
    m_selected_media_index = index;
    m_laserdisc = media.laserdisc;
    m_ejected = false;
    m_subchannel_first_lba = new_subchannel_first_lba;
    m_subchannel_sector_count = new_subchannel_file ?
        (u64)new_subchannel_file->GetSize() / k_mmi_subchannel_record_size : 0;
    m_crc = m_archive.GetCRC();

    return true;
}

bool CdRomMmiImage::FindSubchannelEntry(const GG_MmiEntry* cue_entry, const GG_MmiEntry*& entry) const
{
    entry = NULL;

    if (!cue_entry)
        return false;

    std::string subchannel_path = cue_entry->normalized_name;
    size_t extension_pos = subchannel_path.find_last_of('.');

    if (extension_pos != std::string::npos)
    {
        subchannel_path.resize(extension_pos);
        subchannel_path += ".sub";
        entry = m_archive.FindEntry(subchannel_path.c_str());

        if (entry)
            return true;
    }

    size_t separator_pos = cue_entry->normalized_name.find_last_of('/');
    std::string directory = separator_pos == std::string::npos ? "" :
        cue_entry->normalized_name.substr(0, separator_pos + 1);
    const GG_MmiEntry* match = NULL;
    const std::vector<GG_MmiEntry>& entries = m_archive.GetEntries();

    for (size_t i = 0; i < entries.size(); i++)
    {
        const std::string& name = entries[i].normalized_name;

        if (entries[i].directory || (name.length() < 4) || (name.compare(0, directory.length(), directory) != 0))
        {
            continue;
        }

        std::string file_extension = name.substr(name.length() - 4);

        for (size_t c = 0; c < file_extension.length(); c++)
        {
            if ((file_extension[c] >= 'A') && (file_extension[c] <= 'Z'))
                file_extension[c] = (char)(file_extension[c] - 'A' + 'a');
        }

        if (file_extension != ".sub")
            continue;

        if (match)
        {
            Error("Ambiguous MMI SUB entries beside %s", cue_entry->name.c_str());
            return false;
        }

        match = &entries[i];
    }

    entry = match;

    return true;
}

bool CdRomMmiImage::OpenSubchannel(const GG_MmiEntry* entry, MediaFile*& file, s32& first_lba)
{
    file = NULL;
    first_lba = 0;

    if (!entry || (entry->method != 0) || (entry->uncompressed_size == 0) ||
        ((entry->uncompressed_size % k_mmi_subchannel_record_size) != 0))
    {
        Error("MMI SUB entry must be a non-empty stored 96-byte record stream: %s",
            entry ? entry->name.c_str() : "unknown");
        return false;
    }

    u64 record_count = entry->uncompressed_size / k_mmi_subchannel_record_size;

    if (record_count < m_toc.sector_count)
    {
        Error("MMI SUB entry is shorter than the Redbook stream: %s", entry->name.c_str());
        return false;
    }

    file = m_archive.OpenStoredEntry(entry);

    if (!file)
        return false;

    if (record_count == m_toc.sector_count)
    {
        u8 raw_subchannel[96];
        u8 subchannel_q[12];
        u64 validation_records = MIN(record_count, 75ULL);

        for (u64 record_index = 0; record_index < validation_records; record_index++)
        {
            if (file->ReadAt(record_index * k_mmi_subchannel_record_size, raw_subchannel, sizeof(raw_subchannel)) &&
                DecodeSubchannelQ(raw_subchannel, subchannel_q))
            {
                return true;
            }
        }

        Error("MMI SUB entry does not contain valid Q-channel records: %s", entry->name.c_str());
        SafeDelete(file);
        return false;
    }

    const u64 max_scan_records = 20000;
    u64 scan_records = MIN(record_count, max_scan_records);
    u64 scan_size;

    if (!checked_multiply_u64(scan_records, k_mmi_subchannel_record_size, &scan_size) || (scan_size > (u64)SIZE_MAX))
    {
        SafeDelete(file);
        return false;
    }

    std::vector<u8> raw_subchannel((size_t)scan_size);

    if (!file->ReadAt(0, &raw_subchannel[0], scan_size))
    {
        SafeDelete(file);
        return false;
    }

    u8 subchannel_q[12];

    for (u64 record_index = 0; record_index < scan_records; record_index++)
    {
        const u8* source = &raw_subchannel[(size_t)(record_index * k_mmi_subchannel_record_size)];

        if (DecodeSubchannelQ(source, subchannel_q) && (subchannel_q[1] == 0x01) && (subchannel_q[2] == 0x01) &&
            (subchannel_q[7] == 0x00) && (subchannel_q[8] == 0x02) && (subchannel_q[9] == 0x00) &&
            (record_index <= (u64)INT32_MAX))
        {
            first_lba = -(s32)record_index;
            return true;
        }
    }

    Error("Unable to align captured MMI SUB lead-in: %s", entry->name.c_str());
    SafeDelete(file);

    return false;
}

bool CdRomMmiImage::DecodeSubchannelQ(const u8* raw_subchannel, u8* subchannel_q)
{
    if (!IsValidPointer(raw_subchannel) || !IsValidPointer(subchannel_q))
        return false;

    memset(subchannel_q, 0, 12);

    for (u32 bit = 0; bit < 96; bit++)
        subchannel_q[bit >> 3] |= ((raw_subchannel[bit] >> 6) & 1) << (7 - (bit & 7));

    if (ValidateSubchannelQ(subchannel_q))
        return true;

    memcpy(subchannel_q, raw_subchannel + 12, 12);

    return ValidateSubchannelQ(subchannel_q);
}

bool CdRomMmiImage::ValidateSubchannelQ(const u8* subchannel_q)
{
    if (!subchannel_q || ((subchannel_q[0] & 0x0F) != 1))
        return false;

    u16 expected_crc = (u16)(((u16)subchannel_q[10] << 8) | subchannel_q[11]);

    return mmi_subchannel_q_crc(subchannel_q) == expected_crc;
}

bool CdRomMmiImage::ReadQonIndex(MediaFile* video_file, const GG_MmiStreamInfo& stream, GG_QonInfo& qon_info)
{
    if (!IsValidPointer(video_file))
        return false;

    s64 signed_file_size = video_file->GetSize();

    if (signed_file_size < 24)
    {
        Error("Truncated QON header in %s", stream.file.c_str());
        return false;
    }

    u64 file_size = (u64)signed_file_size;

    u8 header[24];

    if (!video_file->ReadAt(0, header, sizeof(header)) || (memcmp(header, "qon1", 4) != 0))
    {
        Error("Invalid QON header in %s", stream.file.c_str());
        return false;
    }

    qon_info.width = read_u32_le(header + 4);
    qon_info.height = read_u32_le(header + 8);
    qon_info.channels = header[12];
    qon_info.colorspace = header[13];
    qon_info.flags = read_u16_le(header + 14);
    qon_info.frame_count = read_u32_le(header + 16);
    qon_info.frame_duration_us = read_u32_le(header + 20);

    u64 pixel_count;

    if ((qon_info.width == 0) || (qon_info.height == 0) || (qon_info.width > k_qon_max_dimension) ||
        (qon_info.height > k_qon_max_dimension) || (qon_info.channels < 3) || (qon_info.channels > 4) ||
        ((stream.channels != 0) && ((s64)qon_info.channels != stream.channels)) ||
        (qon_info.colorspace > 1) || (qon_info.frame_count == 0) ||
        (qon_info.frame_count > k_qon_max_frame_count) ||
        (qon_info.flags & k_qon_interframe_file_flag) ||
        !checked_multiply_u64(qon_info.width, qon_info.height, &pixel_count) ||
        !checked_multiply_u64(pixel_count, 3, &qon_info.decoded_rgb_size) ||
        (qon_info.decoded_rgb_size > k_qon_max_decoded_rgb_size))
    {
        Error("Unsupported or unsafe QON dimensions/flags in %s", stream.file.c_str());
        return false;
    }

    u64 max_compressed_size;

    if (!checked_multiply_u64(pixel_count, qon_info.channels, &max_compressed_size) ||
        !checked_add_u64(max_compressed_size,
        qon_info.channels == 3 ? (pixel_count / 4) + 1 : pixel_count,
        &max_compressed_size))
    {
        Error("QON frame size overflow in %s", stream.file.c_str());
        return false;
    }

    u64 index_size;
    u64 frame_data_offset;

    if (!checked_multiply_u64(qon_info.frame_count, 8, &index_size) ||
        !checked_add_u64(24, index_size, &frame_data_offset) || (frame_data_offset > file_size) ||
        (index_size > (u64)SIZE_MAX))
    {
        Error("Invalid QON index size in %s", stream.file.c_str());
        return false;
    }

    // Released NEC MMIs leave frame_duration_us unset. The transport is NTSC
    // (30000/1001 Hz), independent of that optional QON duration field.
    u64 total_frames;
    u64 program_end_frame;

    if (!checked_add_u64((u64)stream.frames_in_lead_in_region,
        (u64)stream.frames_in_active_region, &program_end_frame) ||
        !checked_add_u64(program_end_frame, (u64)stream.frames_in_lead_out_region, &total_frames) ||
        (program_end_frame > qon_info.frame_count))
    {
        Error("QON frame count is smaller than the MediaInfo program region in %s", stream.file.c_str());
        return false;
    }

    std::vector<u8> index_data((size_t)index_size);

    if (!index_data.empty() && !video_file->ReadAt(24, &index_data[0], index_data.size()))
    {
        Error("Truncated QON index in %s", stream.file.c_str());
        return false;
    }

    qon_info.frames.resize(qon_info.frame_count);

    for (u32 i = 0; i < qon_info.frame_count; i++)
    {
        const u8* entry = &index_data[(size_t)i * 8];
        u64 index_entry = read_u64_le(entry);
        u64 relative_offset = index_entry & 0x0000FFFFFFFFFFFFULL;
        u16 flags = (u16)(index_entry >> 48);
        u64 record_offset;

        if ((flags & k_qon_interframe_frame_flag) ||
            !checked_add_u64(frame_data_offset, relative_offset, &record_offset) ||
            (record_offset > file_size) || ((file_size - record_offset) < 4))
        {
            Error("Invalid QON frame index %u in %s", i, stream.file.c_str());
            return false;
        }

        u8 size_data[4];

        if (!video_file->ReadAt(record_offset, size_data, sizeof(size_data)))
            return false;

        u32 compressed_size = read_u32_le(size_data);
        u64 frame_end_offset;

        if ((compressed_size == 0) || ((u64)compressed_size > max_compressed_size) ||
            !checked_add_u64(record_offset, 4, &frame_end_offset) ||
            !checked_add_u64(frame_end_offset, compressed_size, &frame_end_offset) || (frame_end_offset > file_size))
        {
            Error("Invalid QON frame payload %u in %s", i, stream.file.c_str());
            return false;
        }

        qon_info.frames[i].record_offset = record_offset;
        qon_info.frames[i].compressed_size = compressed_size;
        qon_info.frames[i].flags = flags;
    }

    std::vector<GG_QonFrameInfo> sorted_frames = qon_info.frames;

    std::sort(sorted_frames.begin(), sorted_frames.end(), [](const GG_QonFrameInfo& left, const GG_QonFrameInfo& right)
    {
        return left.record_offset < right.record_offset;
    });

    for (size_t i = 1; i < sorted_frames.size(); i++)
    {
        if (sorted_frames[i].record_offset == sorted_frames[i - 1].record_offset)
        {
            if (sorted_frames[i].compressed_size != sorted_frames[i - 1].compressed_size)
                return false;

            continue;
        }

        u64 previous_end = sorted_frames[i - 1].record_offset + 4 + sorted_frames[i - 1].compressed_size;

        if (sorted_frames[i].record_offset < previous_end)
        {
            Error("Overlapping QON frame payloads in %s", stream.file.c_str());
            return false;
        }
    }

    if (total_frames < qon_info.frame_count)
        Log("MMI QON stream %s contains %u trailing frame(s)", stream.file.c_str(),
            (unsigned)(qon_info.frame_count - total_frames));
    else if (total_frames > qon_info.frame_count)
        Log("MMI QON stream %s has %llu fewer captured lead-out frames than declared",
            stream.file.c_str(), (unsigned long long)(total_frames - qon_info.frame_count));

    return true;
}

const GG_MmiStreamInfo* CdRomMmiImage::FindStream(const GG_MmiMediaInfo& media, GG_MmiStreamRole role) const
{
    for (size_t i = 0; i < media.streams.size(); i++)
    {
        if (media.streams[i].role == role)
            return &media.streams[i];
    }

    return NULL;
}
