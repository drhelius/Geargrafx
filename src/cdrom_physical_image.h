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
#define CDROM_PHYSICAL_SECTORS_PER_BLOCK 16
#define CDROM_PHYSICAL_CACHE_BITS 10
#define CDROM_PHYSICAL_CACHE_BLOCKS (1 << CDROM_PHYSICAL_CACHE_BITS)
#define CDROM_PHYSICAL_REQUEST_QUEUE_SIZE 64
#define CDROM_PHYSICAL_PREFETCH_BLOCKS 16
#define CDROM_PHYSICAL_STANDARD_PREGAP_SECTORS 150
#define CDROM_PHYSICAL_MAX_PREGAP_SECTORS (75 * 4)

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
        u32 block_lba;
        u8 data[CDROM_PHYSICAL_SECTOR_SIZE * CDROM_PHYSICAL_SECTORS_PER_BLOCK];
    };

private:
    bool ReadTOC();
    bool DetectDataTrackType(Track& track);
    void NormalizeTrackBoundaries();
    bool IsMode1DataSector(u32 lba);
    bool SectorMatchesTrackType(u32 lba, GG_CdRomTrackType type);
    void CalculateCRC();
    u32 CalculateTOCFingerprint();
    bool ReadCachedRange(u32 lba, u32 offset, u8* buffer, u32 size);
    bool ReadRawBlock(u32 block_lba, u8* buffer);
    bool FindCacheRange(u32 block_lba, u32 block_offset, u8* buffer, u32 size);
    void StoreCacheBlock(u32 block_lba, const u8* buffer);
    void QueueReadAhead(u32 lba, s32 track_index);
    void QueueBlock(u32 block_lba);
    void StartWorker();
    void StopWorker();
    void WorkerThread();
    void SetDiscError();
    void ResetCache();
    void ResetQueue();
    u32 CacheIndex(u32 block_lba) const;
    u32 BlockStartLBA(u32 lba) const;
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
    u32 m_request_queue[CDROM_PHYSICAL_REQUEST_QUEUE_SIZE];
    u32 m_request_head;
    u32 m_request_tail;
    u32 m_request_count;
    std::atomic<u32> m_last_block_lba;
};

#endif /* GG_ENABLE_PHYSICAL_CDROM */
#endif /* CDROM_PHYSICAL_IMAGE_H */
