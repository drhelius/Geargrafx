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

#ifndef CDROM_PHYSICAL_IMAGE_H
#define CDROM_PHYSICAL_IMAGE_H

#include "common.h"

#if defined(GG_ENABLE_PHYSICAL_CDROM)

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "cdrom_image.h"
#include "cdrom_drive.h"

#define CDROM_PHYSICAL_SECTOR_SIZE 2352
#define CDROM_PHYSICAL_DATA_SECTOR_SIZE 2048
#define CDROM_PHYSICAL_SECTORS_PER_BLOCK 16
#define CDROM_PHYSICAL_CACHE_BITS 10
#define CDROM_PHYSICAL_CACHE_BLOCKS (1 << CDROM_PHYSICAL_CACHE_BITS)
#define CDROM_PHYSICAL_REQUEST_QUEUE_SIZE 64
#define CDROM_PHYSICAL_PREFETCH_BLOCKS 16
#define CDROM_PHYSICAL_AUDIO_PREFETCH_BLOCKS 1
#define CDROM_PHYSICAL_AUDIO_PRELOAD_BLOCKS 30
#define CDROM_PHYSICAL_DRIVE_SPEED_KBPS 704
#define CDROM_PHYSICAL_KEEPALIVE_SECONDS 10
#define CDROM_PHYSICAL_READ_RETRIES 5
#define CDROM_PHYSICAL_RETRY_DELAY_MS 20
#define CDROM_PHYSICAL_MAX_READ_DIAGNOSTICS 32

class CdRomPhysicalImage : public CdRomImage
{
public:
    CdRomPhysicalImage();
    virtual ~CdRomPhysicalImage();

    virtual void Reset() override;
    virtual bool LoadFromFile(const char* path, bool preload) override;
    bool LoadFromDevice(const char* device_id, bool preload);
    virtual bool ReadSector(u32 lba, u8* buffer) override;
    virtual bool ReadSamples(u32 lba, u32 offset, s16* buffer, u32 count) override;
    virtual bool PreloadDisc() override;
    virtual bool PreloadTrack(u32 track_number) override;
    bool HasDiscError() const;

private:
    struct CacheEntry
    {
        bool valid;
        bool data_block;
        u32 block_lba;
        u8 data[CDROM_PHYSICAL_SECTOR_SIZE * CDROM_PHYSICAL_SECTORS_PER_BLOCK];
    };

private:
    bool ReadTOC();
    void CalculateCRC();
    bool ReadDataSector(u32 lba, u8* buffer);
    bool ReadDataSectorUncached(u32 lba, u8* buffer);
    bool ReadDataBlock(u32 block_lba, u8* buffer);
    bool ReadDataBlockReadAhead(u32 block_lba, u8* buffer);
    bool ReadCachedRange(u32 lba, u32 offset, u8* buffer, u32 size);
    bool ReadRawBlock(u32 block_lba, u8* buffer);
    bool ReadRawBlockReadAhead(u32 block_lba, u8* buffer);
    bool ReadRawSector(u32 lba, u8* buffer);
    bool FindCacheRange(u32 block_lba, bool data_block, u32 block_offset, u8* buffer, u32 size);
    bool HasCachedBlock(u32 block_lba, bool data_block);
    void StoreCacheBlock(u32 block_lba, bool data_block, const u8* buffer);
    void QueueReadAhead(u32 lba, s32 track_index, u32 max_blocks = CDROM_PHYSICAL_PREFETCH_BLOCKS);
    void QueueBlock(u32 block_lba);
    void StartWorker();
    void StopWorker();
    void WorkerThread();
    bool ReadKeepAliveSector(u32 lba);
    void UpdateForegroundTrack(s32 track_index);
    bool TryLockDriveForReadAhead(std::unique_lock<std::mutex>& lock);
    void SetDiscError();
    void ResetCache();
    void ResetQueue();
    bool ShouldLogReadDiagnostic();
    u32 CacheIndex(u32 block_lba) const;
    u32 BlockStartLBA(u32 lba) const;
    u32 BlockStartLBAForTrack(u32 lba, const Track& track) const;
    u32 ReadAheadBlocksForTrack(const Track& track) const;
    bool IsBlockWithinTrack(u32 block_lba, const Track& track) const;
    bool IsAudioSector(u32 lba);

private:
    CdRomDrive m_drive;
    CacheEntry m_cache[CDROM_PHYSICAL_CACHE_BLOCKS];
    std::mutex m_drive_mutex;
    std::mutex m_cache_mutex;
    std::mutex m_queue_mutex;
    std::condition_variable m_queue_condition;
    std::thread m_worker_thread;
    std::atomic<bool> m_worker_running;
    std::atomic<bool> m_disc_error;
    std::atomic<bool> m_foreground_read_pending;
    u32 m_request_queue[CDROM_PHYSICAL_REQUEST_QUEUE_SIZE];
    u32 m_request_head;
    u32 m_request_tail;
    u32 m_request_count;
    std::atomic<u32> m_last_read_lba;
    std::atomic<s32> m_last_foreground_track;
    std::atomic<u32> m_read_diagnostic_count;
};

#endif /* GG_ENABLE_PHYSICAL_CDROM */
#endif /* CDROM_PHYSICAL_IMAGE_H */
