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

#ifndef CDROM_CHD_IMAGE_H
#define CDROM_CHD_IMAGE_H

#include <libchdr/chd.h>
#include "cdrom_image.h"

class CdRomChdImage : public CdRomImage
{
public:
    CdRomChdImage();
    virtual ~CdRomChdImage();
    virtual void Init() override;
    virtual void Reset() override;
    virtual bool LoadFromFile(const char* path) override;
    virtual bool ReadSector(u32 lba, u8* buffer) override;
    virtual bool ReadSamples(u32 lba, u32 offset, s16* buffer, u32 count) override;
    virtual bool PreloadDisc() override;
    virtual bool PreloadTrack(u32 track_number) override;

private:
    bool ReadTOC();
    void CalculateCRC();
    void InitHunkCache();
    void DestroyHunkCache();
    GG_CdRomTrackType GetTrackType(const char* type_str);

private:
    chd_file* m_chd_file;
    u8** m_hunk_cache;
    u32 m_hunk_bytes;
    u32 m_hunk_count;
    u32 m_sectors_per_hunk;

private:

};

#endif /* CDROM_CHD_IMAGE_H */