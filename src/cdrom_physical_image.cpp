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

#include "cdrom_physical_image.h"

#if defined(GG_ENABLE_PHYSICAL_CDROM)

#include <algorithm>
#include <chrono>
#include "crc.h"

static void crc_append_u8(u32* crc, u8 value)
{
    *crc = CalculateCRC32(*crc, &value, 1);
}

static void crc_append_u32(u32* crc, u32 value)
{
    u8 data[4];
    data[0] = (u8)value;
    data[1] = (u8)(value >> 8);
    data[2] = (u8)(value >> 16);
    data[3] = (u8)(value >> 24);
    *crc = CalculateCRC32(*crc, data, sizeof(data));
}

static bool raw_sector_has_mode1_sync(const u8* raw)
{
    static const u8 sync[12] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };

    for (int i = 0; i < 12; i++)
    {
        if (raw[i] != sync[i])
            return false;
    }

    return raw[15] == 1;
}

CdRomPhysicalImage::CdRomPhysicalImage()
{
    m_worker_running.store(false);
    m_disc_error.store(false);
    m_last_read_lba.store(0);
    ResetQueue();
    ResetCache();
}

CdRomPhysicalImage::~CdRomPhysicalImage()
{
    Reset();
}

void CdRomPhysicalImage::Reset()
{
    StopWorker();
    m_drive.Close();
    CdRomImage::Reset();
    m_disc_error.store(false);
    m_last_read_lba.store(0);
    ResetQueue();
    ResetCache();
}

bool CdRomPhysicalImage::LoadFromFile(const char* path, bool preload)
{
    return LoadFromDevice(path, preload);
}

bool CdRomPhysicalImage::LoadFromDevice(const char* device_id, bool preload)
{
    if (!IsValidPointer(device_id) || (device_id[0] == 0))
    {
        Error("Invalid physical CD-ROM device id");
        return false;
    }

    Reset();

    Log("Loading physical CD-ROM device %s", device_id);

    if (!m_drive.Open(device_id))
    {
        Error("Failed to open physical CD-ROM device %s", device_id);
        return false;
    }

    snprintf(m_file_path, sizeof(m_file_path), "physicalcd://%s", device_id);
    m_file_directory[0] = 0;
    strncpy_fit(m_file_name, device_id, sizeof(m_file_name));
    strncpy_fit(m_file_extension, "physicalcd", sizeof(m_file_extension));

    if (!ReadTOC())
    {
        Error("Failed to read physical CD-ROM TOC from %s", device_id);
        Reset();
        return false;
    }

    m_ready = true;
    CalculateCRC();

    if (HasDiscError())
    {
        Error("Physical CD-ROM read failed while loading %s", device_id);
        Reset();
        return false;
    }

    StartWorker();

    if (preload)
    {
        Debug("Physical CD-ROM preload requested for %s", device_id);
        PreloadDisc();
    }

    Log("Physical CD-ROM loaded from %s", device_id);

    return true;
}

bool CdRomPhysicalImage::ReadSector(u32 lba, u8* buffer)
{
    if (!m_ready || m_disc_error.load() || !IsValidPointer(buffer))
        return false;

    s32 track_index = GetTrackFromLBA(lba);
    if (track_index < 0)
        return false;

    Track& track = m_toc.tracks[(size_t)track_index];
    if (track.type == GG_CDROM_AUDIO_TRACK)
    {
        Error("ReadSector failed - LBA %u belongs to an audio track", lba);
        return false;
    }

    if (!ReadCachedRange(lba, 16, buffer, 2048))
        return false;

    SetCurrentSector(lba + 1);

    return true;
}

bool CdRomPhysicalImage::ReadSamples(u32 lba, u32 offset, s16* buffer, u32 count)
{
    if (!m_ready || m_disc_error.load() || !IsValidPointer(buffer))
        return false;

    if ((offset >= CDROM_PHYSICAL_SECTOR_SIZE) || ((count * sizeof(s16)) > CDROM_PHYSICAL_SECTOR_SIZE) || ((offset + (count * sizeof(s16))) > CDROM_PHYSICAL_SECTOR_SIZE))
        return false;

    s32 track_index = GetTrackFromLBA(lba);
    if (track_index < 0)
        return false;

    Track& track = m_toc.tracks[(size_t)track_index];
    if (track.type != GG_CDROM_AUDIO_TRACK)
        return false;

    u32 size = count * sizeof(s16);
    if (!ReadCachedRange(lba, offset, (u8*)buffer, size))
        return false;

#if defined(GG_BIG_ENDIAN)
    u16* samples = (u16*)buffer;
    for (u32 i = 0; i < count; i++)
        samples[i] = (samples[i] >> 8) | (samples[i] << 8);
#endif

    SetCurrentSector(lba);

    return true;
}

bool CdRomPhysicalImage::PreloadDisc()
{
    if (!m_ready || m_disc_error.load())
        return false;

    Debug("Physical CD-ROM queueing disc preload from LBA 0");
    QueueReadAhead(0, GetTrackFromLBA(0));
    return true;
}

bool CdRomPhysicalImage::PreloadTrack(u32 track_number)
{
    if (!m_ready || m_disc_error.load())
        return false;

    if (track_number >= m_toc.tracks.size())
    {
        Error("PreloadTrack failed - Track number %u out of bounds (max: %u)", track_number, (u32)m_toc.tracks.size() - 1);
        return false;
    }

    QueueReadAhead(m_toc.tracks[track_number].start_lba, (s32)track_number);
    Debug("Physical CD-ROM queueing preload for track %u at LBA %u", track_number, m_toc.tracks[track_number].start_lba);
    return true;
}

bool CdRomPhysicalImage::ReadTOC()
{
    std::vector<CdRomDriveTrackInfo> drive_tracks;
    u32 lead_out_lba = 0;

    if (!m_drive.ReadTOC(drive_tracks, &lead_out_lba))
        return false;

    Debug("Physical CD-ROM image TOC conversion: tracks=%d lead_out=%u", (int)drive_tracks.size(), lead_out_lba);

    if (drive_tracks.empty())
    {
        Error("Physical CD-ROM TOC has no tracks");
        return false;
    }

    m_toc.tracks.clear();

    for (size_t i = 0; i < drive_tracks.size(); i++)
    {
        u32 next_lba = ((i + 1) < drive_tracks.size()) ? drive_tracks[i + 1].start_lba : lead_out_lba;

        if (next_lba <= drive_tracks[i].start_lba)
        {
            Error("Invalid physical CD-ROM TOC track %u: start %u, next %u", drive_tracks[i].number, drive_tracks[i].start_lba, next_lba);
            return false;
        }

        Track track;
        InitTrack(track);
        track.type = drive_tracks[i].data ? GG_CDROM_DATA_TRACK_MODE1_2352 : GG_CDROM_AUDIO_TRACK;
        track.sector_size = CDROM_PHYSICAL_SECTOR_SIZE;
        track.start_lba = drive_tracks[i].start_lba;
        track.end_lba = next_lba - 1;
        track.sector_count = next_lba - track.start_lba;
        track.file_offset = track.start_lba * CDROM_PHYSICAL_SECTOR_SIZE;
        LbaToMsf(track.start_lba, &track.start_msf);
        LbaToMsf(track.end_lba, &track.end_msf);

        if (track.type != GG_CDROM_AUDIO_TRACK)
        {
            if (!DetectDataTrackType(track))
                return false;
        }

        m_toc.tracks.push_back(track);
    }

    m_toc.sector_count = lead_out_lba;
    NormalizeTrackBoundaries();

    LbaToMsf(m_toc.sector_count + 150, &m_toc.total_length);

    for (size_t i = 0; i < m_toc.tracks.size(); i++)
    {
        Track& track = m_toc.tracks[i];
        Log("Physical track %2u (%s): Start LBA: %6u, End LBA: %6u, Sectors: %6u",
            (u32)(i + 1), TrackTypeName(track.type), track.start_lba, track.end_lba, track.sector_count);
    }

    Debug("Physical CD-ROM length: %02u:%02u:%02u, Total sectors: %u",
        m_toc.total_length.minutes, m_toc.total_length.seconds, m_toc.total_length.frames,
        m_toc.sector_count);

    return true;
}

void CdRomPhysicalImage::NormalizeTrackBoundaries()
{
    if (m_toc.tracks.size() < 2)
        return;

    for (size_t i = 0; (i + 1) < m_toc.tracks.size(); i++)
    {
        Track& track = m_toc.tracks[i];
        Track& next_track = m_toc.tracks[i + 1];

        if (track.type == next_track.type)
            continue;

        if (next_track.start_lba == 0)
            continue;

        static const u32 audio_pregap_lengths[] = { CDROM_PHYSICAL_STANDARD_PREGAP_SECTORS, (2 * 75) + 74, 75 * 3, 75 * 4 };
        static const u32 data_pregap_lengths[] = { (2 * 75) + 74, CDROM_PHYSICAL_STANDARD_PREGAP_SECTORS, 75 * 3, 75 * 4 };

        const u32* pregap_lengths = (next_track.type == GG_CDROM_AUDIO_TRACK) ? audio_pregap_lengths : data_pregap_lengths;
        u32 pregap_count = (u32)((next_track.type == GG_CDROM_AUDIO_TRACK) ?
            (sizeof(audio_pregap_lengths) / sizeof(audio_pregap_lengths[0])) :
            (sizeof(data_pregap_lengths) / sizeof(data_pregap_lengths[0])));
        u32 lead_in_lba = next_track.start_lba;

        for (u32 j = 0; j < pregap_count; j++)
        {
            u32 pregap_length = pregap_lengths[j];
            if (next_track.start_lba <= pregap_length)
                continue;

            u32 pregap_lba = next_track.start_lba - pregap_length;
            if ((pregap_lba <= track.start_lba) || (pregap_lba > track.end_lba))
                continue;

            if (!SectorMatchesTrackType(pregap_lba, next_track.type))
                continue;

            if (SectorMatchesTrackType(pregap_lba - 1, next_track.type))
                continue;

            lead_in_lba = pregap_lba;
            break;
        }

        if (lead_in_lba == next_track.start_lba)
            continue;

        if (lead_in_lba <= track.start_lba)
            continue;

        if (lead_in_lba > track.end_lba)
            continue;

        Debug("Physical CD-ROM trimming %u-sector %s lead-in before track %u at LBA %u",
            next_track.start_lba - lead_in_lba, TrackTypeName(next_track.type), (u32)(i + 2), lead_in_lba);

        track.end_lba = lead_in_lba - 1;
        track.sector_count = track.end_lba - track.start_lba + 1;
        LbaToMsf(track.end_lba, &track.end_msf);
        next_track.has_lead_in = true;
        next_track.lead_in_lba = lead_in_lba;
    }
}

bool CdRomPhysicalImage::IsMode1DataSector(u32 lba)
{
    if (lba >= m_toc.sector_count)
        return false;

    u8 raw[CDROM_PHYSICAL_SECTOR_SIZE];
    memset(raw, 0, sizeof(raw));

    std::lock_guard<std::mutex> lock(m_drive_mutex);
    if (!m_drive.ReadRawSector2352(lba, raw, false))
        return false;

    return raw_sector_has_mode1_sync(raw);
}

bool CdRomPhysicalImage::SectorMatchesTrackType(u32 lba, GG_CdRomTrackType type)
{
    bool mode1 = IsMode1DataSector(lba);

    if (type == GG_CDROM_AUDIO_TRACK)
        return !mode1;

    return mode1;
}

bool CdRomPhysicalImage::DetectDataTrackType(Track& track)
{
    u8 raw[CDROM_PHYSICAL_SECTOR_SIZE];
    memset(raw, 0, sizeof(raw));

    {
        std::lock_guard<std::mutex> lock(m_drive_mutex);
        if (!m_drive.ReadRawSector2352(track.start_lba, raw, false))
        {
            Error("Failed to read physical data track %u for mode detection", (u32)(m_toc.tracks.size() + 1));
            return false;
        }
    }

    u8 mode = raw[15];
    Debug("Physical CD-ROM data track mode detection at LBA %u: mode=%u", track.start_lba, mode);
    if (mode == 1)
    {
        track.type = GG_CDROM_DATA_TRACK_MODE1_2352;
        track.sector_size = CDROM_PHYSICAL_SECTOR_SIZE;
        return true;
    }

    Error("Unsupported physical CD-ROM data track mode %u", mode);
    return false;
}

void CdRomPhysicalImage::CalculateCRC()
{
    m_crc = 0;
    u32 current_sector = m_current_sector;

    for (size_t track_index = 0; track_index < m_toc.tracks.size(); track_index++)
    {
        Track& track = m_toc.tracks[track_index];
        if (track.type == GG_CDROM_AUDIO_TRACK)
            continue;

        if (track.sector_count <= 1)
        {
            m_current_sector = current_sector;
            return;
        }

        u8 buffer[2048];
        u32 sectors = MIN((u32)64, track.sector_count - 1);

        for (u32 i = 1; i <= sectors; i++)
        {
            u32 lba = track.start_lba + i;
            if (!ReadSector(lba, buffer))
            {
                Error("Physical CD-ROM CRC read failed at LBA %u", lba);
                m_crc = CalculateTOCFingerprint();
                Debug("Physical CD-ROM fallback TOC fingerprint: %08X", m_crc);
                m_current_sector = current_sector;
                return;
            }

            m_crc = CalculateCRC32(m_crc, buffer, 2048);
        }

        Debug("Physical CD-ROM CRC: %08X", m_crc);
        m_current_sector = current_sector;
        return;
    }

    m_crc = CalculateTOCFingerprint();
    Debug("Physical CD-ROM CRC unavailable, fallback TOC fingerprint: %08X", m_crc);
    m_current_sector = current_sector;
}

u32 CdRomPhysicalImage::CalculateTOCFingerprint()
{
    static const u8 tag[] = { 'G', 'G', 'P', 'H', 'Y', 'S', 'T', 'O', 'C' };
    u32 crc = CalculateCRC32(0, tag, sizeof(tag));

    crc_append_u32(&crc, (u32)m_toc.tracks.size());
    crc_append_u32(&crc, m_toc.sector_count);

    for (size_t i = 0; i < m_toc.tracks.size(); i++)
    {
        Track& track = m_toc.tracks[i];
        crc_append_u8(&crc, (u8)track.type);
        crc_append_u32(&crc, track.sector_size);
        crc_append_u32(&crc, track.sector_count);
        crc_append_u32(&crc, track.start_lba);
        crc_append_u32(&crc, track.end_lba);
    }

    if (crc == 0)
        crc = 0xFFFFFFFF;

    return crc;
}

bool CdRomPhysicalImage::ReadCachedRange(u32 lba, u32 offset, u8* buffer, u32 size)
{
    if (m_disc_error.load())
        return false;

    if (!IsValidPointer(buffer) || (offset >= CDROM_PHYSICAL_SECTOR_SIZE) || (size > CDROM_PHYSICAL_SECTOR_SIZE) || ((offset + size) > CDROM_PHYSICAL_SECTOR_SIZE))
        return false;

    if (lba >= m_toc.sector_count)
    {
        Error("Physical CD-ROM raw read out of bounds - LBA %u, max %u", lba, m_toc.sector_count - 1);
        return false;
    }

    u32 block_lba = BlockStartLBA(lba);
    u32 sector_offset = lba - block_lba;
    u32 block_offset = (sector_offset * CDROM_PHYSICAL_SECTOR_SIZE) + offset;
    u8 block[CDROM_PHYSICAL_SECTOR_SIZE * CDROM_PHYSICAL_SECTORS_PER_BLOCK];

    if (!FindCacheRange(block_lba, block_offset, buffer, size))
    {
        if (!ReadRawBlock(block_lba, block))
        {
            u8 sector[CDROM_PHYSICAL_SECTOR_SIZE];
            if (!ReadRawSector(lba, sector))
            {
                SetDiscError();
                return false;
            }

            memcpy(buffer, sector + offset, size);
            m_last_read_lba.store(lba);
            QueueReadAhead(block_lba + CDROM_PHYSICAL_SECTORS_PER_BLOCK, GetTrackFromLBA(lba));
            return true;
        }

        StoreCacheBlock(block_lba, block);
        memcpy(buffer, block + block_offset, size);
    }

    m_last_read_lba.store(lba);
    QueueReadAhead(block_lba + CDROM_PHYSICAL_SECTORS_PER_BLOCK, GetTrackFromLBA(lba));

    return true;
}

bool CdRomPhysicalImage::ReadRawBlock(u32 block_lba, u8* buffer)
{
    if (block_lba >= m_toc.sector_count)
        return false;

    u32 sector_count = MIN((u32)CDROM_PHYSICAL_SECTORS_PER_BLOCK, m_toc.sector_count - block_lba);
    bool audio = IsAudioSector(block_lba);
    bool mixed = false;
    for (u32 i = 1; i < sector_count; i++)
    {
        if (IsAudioSector(block_lba + i) != audio)
        {
            mixed = true;
            break;
        }
    }

    for (u32 attempt = 0; attempt < CDROM_PHYSICAL_READ_RETRIES; attempt++)
    {
        bool read = false;
        memset(buffer, 0, CDROM_PHYSICAL_SECTOR_SIZE * CDROM_PHYSICAL_SECTORS_PER_BLOCK);

        {
            std::lock_guard<std::mutex> lock(m_drive_mutex);

            if (!mixed)
            {
                read = m_drive.ReadRawSectors2352(block_lba, sector_count, buffer, audio);
            }
            else
            {
                read = true;
                for (u32 i = 0; i < sector_count; i++)
                {
                    if (!m_drive.ReadRawSector2352(block_lba + i, buffer + (i * CDROM_PHYSICAL_SECTOR_SIZE), IsAudioSector(block_lba + i)))
                    {
                        read = false;
                        break;
                    }
                }
            }
        }

        if (read)
            return true;

        if ((attempt + 1) < CDROM_PHYSICAL_READ_RETRIES)
        {
            Debug("Physical CD-ROM retrying block read at LBA %u (attempt %u)", block_lba, attempt + 2);
            std::this_thread::sleep_for(std::chrono::milliseconds(CDROM_PHYSICAL_RETRY_DELAY_MS));
        }
    }

    return false;
}

bool CdRomPhysicalImage::ReadRawSector(u32 lba, u8* buffer)
{
    if ((lba >= m_toc.sector_count) || !IsValidPointer(buffer))
        return false;

    for (u32 attempt = 0; attempt < CDROM_PHYSICAL_READ_RETRIES; attempt++)
    {
        bool read = false;
        memset(buffer, 0, CDROM_PHYSICAL_SECTOR_SIZE);

        {
            std::lock_guard<std::mutex> lock(m_drive_mutex);
            read = m_drive.ReadRawSector2352(lba, buffer, IsAudioSector(lba));
        }

        if (read)
            return true;

        if ((attempt + 1) < CDROM_PHYSICAL_READ_RETRIES)
        {
            Debug("Physical CD-ROM retrying sector read at LBA %u (attempt %u)", lba, attempt + 2);
            std::this_thread::sleep_for(std::chrono::milliseconds(CDROM_PHYSICAL_RETRY_DELAY_MS));
        }
    }

    return false;
}

bool CdRomPhysicalImage::FindCacheRange(u32 block_lba, u32 block_offset, u8* buffer, u32 size)
{
    u32 index = CacheIndex(block_lba);

    std::lock_guard<std::mutex> lock(m_cache_mutex);
    if (m_cache[index].valid && (m_cache[index].block_lba == block_lba))
    {
        memcpy(buffer, m_cache[index].data + block_offset, size);
        return true;
    }

    return false;
}

void CdRomPhysicalImage::StoreCacheBlock(u32 block_lba, const u8* buffer)
{
    u32 index = CacheIndex(block_lba);

    std::lock_guard<std::mutex> lock(m_cache_mutex);
    m_cache[index].block_lba = block_lba;
    memcpy(m_cache[index].data, buffer, CDROM_PHYSICAL_SECTOR_SIZE * CDROM_PHYSICAL_SECTORS_PER_BLOCK);
    m_cache[index].valid = true;
}

void CdRomPhysicalImage::QueueReadAhead(u32 lba, s32 track_index)
{
    if (m_disc_error.load())
        return;

    if ((track_index < 0) || (track_index >= (s32)m_toc.tracks.size()))
        return;

    Track& track = m_toc.tracks[(size_t)track_index];
    u32 block_lba = BlockStartLBA(lba);

    if (block_lba < track.start_lba)
        block_lba += CDROM_PHYSICAL_SECTORS_PER_BLOCK;

    for (u32 i = 0; i < CDROM_PHYSICAL_PREFETCH_BLOCKS; i++)
    {
        u32 next_lba = block_lba + (i * CDROM_PHYSICAL_SECTORS_PER_BLOCK);
        if (next_lba >= m_toc.sector_count)
            break;

        u32 block_end_lba = next_lba + CDROM_PHYSICAL_SECTORS_PER_BLOCK - 1;
        if ((next_lba < track.start_lba) || (block_end_lba > track.end_lba))
            break;

        QueueBlock(next_lba);
    }
}

void CdRomPhysicalImage::QueueBlock(u32 block_lba)
{
    if (!m_worker_running.load() || m_disc_error.load())
        return;

    if (block_lba >= m_toc.sector_count)
        return;

    block_lba = BlockStartLBA(block_lba);

    {
        std::lock_guard<std::mutex> lock(m_cache_mutex);
        u32 index = CacheIndex(block_lba);
        if (m_cache[index].valid && (m_cache[index].block_lba == block_lba))
            return;
    }

    std::lock_guard<std::mutex> lock(m_queue_mutex);

    for (u32 i = 0; i < m_request_count; i++)
    {
        u32 index = (m_request_head + i) % CDROM_PHYSICAL_REQUEST_QUEUE_SIZE;
        if (m_request_queue[index] == block_lba)
            return;
    }

    if (m_request_count >= CDROM_PHYSICAL_REQUEST_QUEUE_SIZE)
    {
        Debug("Physical CD-ROM read-ahead queue full, dropping LBA %u", block_lba);
        return;
    }

    m_request_queue[m_request_tail] = block_lba;
    m_request_tail = (m_request_tail + 1) % CDROM_PHYSICAL_REQUEST_QUEUE_SIZE;
    m_request_count++;
    m_queue_condition.notify_one();
}

void CdRomPhysicalImage::StartWorker()
{
    if (m_worker_running.load())
        return;

    m_worker_running.store(true);
    Debug("Physical CD-ROM read-ahead worker starting");
    m_worker_thread = std::thread(&CdRomPhysicalImage::WorkerThread, this);
}

void CdRomPhysicalImage::StopWorker()
{
    if (!m_worker_running.load() && !m_worker_thread.joinable())
        return;

    m_worker_running.store(false);
    m_queue_condition.notify_one();

    if (m_worker_thread.joinable())
        m_worker_thread.join();

    Debug("Physical CD-ROM read-ahead worker stopped");
}

void CdRomPhysicalImage::WorkerThread()
{
    std::chrono::steady_clock::time_point last_keep_alive = std::chrono::steady_clock::now();

    while (m_worker_running.load() && !m_disc_error.load())
    {
        u32 block_lba = 0;
        bool has_request = false;

        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            if (m_request_count == 0)
                m_queue_condition.wait_for(lock, std::chrono::milliseconds(250));

            if (!m_worker_running.load())
                break;

            if (m_disc_error.load())
                break;

            if (m_request_count != 0)
            {
                block_lba = m_request_queue[m_request_head];
                m_request_head = (m_request_head + 1) % CDROM_PHYSICAL_REQUEST_QUEUE_SIZE;
                m_request_count--;
                has_request = true;
            }
        }

        if (has_request)
        {
            u8 block[CDROM_PHYSICAL_SECTOR_SIZE * CDROM_PHYSICAL_SECTORS_PER_BLOCK];
            if (ReadRawBlock(block_lba, block))
                StoreCacheBlock(block_lba, block);
            else
                Debug("Physical CD-ROM read-ahead failed at LBA %u, dropping block", block_lba);

            last_keep_alive = std::chrono::steady_clock::now();
            continue;
        }

        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_keep_alive).count() >= CDROM_PHYSICAL_KEEPALIVE_SECONDS)
        {
            u8 sector[CDROM_PHYSICAL_SECTOR_SIZE];
            if (!ReadRawSector(m_last_read_lba.load(), sector))
                Debug("Physical CD-ROM keepalive read failed at LBA %u", m_last_read_lba.load());

            last_keep_alive = now;
        }
    }
}

bool CdRomPhysicalImage::HasDiscError() const
{
    return m_disc_error.load();
}

void CdRomPhysicalImage::SetDiscError()
{
    if (!m_disc_error.exchange(true))
        Error("Physical CD-ROM media read failed, stopping physical CD-ROM emulation");

    m_worker_running.store(false);
    ResetQueue();
    m_queue_condition.notify_all();
}

void CdRomPhysicalImage::ResetCache()
{
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    for (u32 i = 0; i < CDROM_PHYSICAL_CACHE_BLOCKS; i++)
    {
        m_cache[i].valid = false;
        m_cache[i].block_lba = 0;
    }
}

void CdRomPhysicalImage::ResetQueue()
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_request_head = 0;
    m_request_tail = 0;
    m_request_count = 0;
}

u32 CdRomPhysicalImage::CacheIndex(u32 block_lba) const
{
    u32 block = block_lba / CDROM_PHYSICAL_SECTORS_PER_BLOCK;
    block ^= block >> CDROM_PHYSICAL_CACHE_BITS;
    return block & (CDROM_PHYSICAL_CACHE_BLOCKS - 1);
}

u32 CdRomPhysicalImage::BlockStartLBA(u32 lba) const
{
    return lba & ~(CDROM_PHYSICAL_SECTORS_PER_BLOCK - 1);
}

bool CdRomPhysicalImage::IsAudioSector(u32 lba)
{
    for (size_t i = 0; i < m_toc.tracks.size(); i++)
    {
        const Track& track = m_toc.tracks[i];

        if ((lba >= track.start_lba) && (lba <= track.end_lba))
            return track.type == GG_CDROM_AUDIO_TRACK;

        if (track.has_lead_in && (lba >= track.lead_in_lba) && (lba < track.start_lba))
            return track.type == GG_CDROM_AUDIO_TRACK;
    }

    return false;
}

#endif /* GG_ENABLE_PHYSICAL_CDROM */
