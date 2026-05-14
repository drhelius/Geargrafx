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

#ifndef CDROM_DRIVE_H
#define CDROM_DRIVE_H

#include "common.h"

#if defined(GG_ENABLE_PHYSICAL_CDROM)

#include <vector>

struct CdRomDriveInfo
{
    char id[256];
    char name[256];
    bool has_disc;
};

struct CdRomDriveTrackInfo
{
    u8 number;
    bool data;
    u32 start_lba;
};

class CdRomDrive
{
public:
    CdRomDrive();
    ~CdRomDrive();

    static bool ListDrives(std::vector<CdRomDriveInfo>& drives);
    static bool Eject(const char* device_id);

    bool Open(const char* device_id);
    void Close();
    bool IsOpen() const;
    bool ReadTOC(std::vector<CdRomDriveTrackInfo>& tracks, u32* lead_out_lba);
    bool ReadRawSectors2352(u32 lba, u32 sector_count, u8* buffer, bool audio);
    bool ReadRawSector2352(u32 lba, u8* buffer, bool audio = false);
    bool SetSpeed(u16 speed);
    const char* GetDeviceId() const;

private:
#if defined(_WIN32)
    HANDLE m_file;
#else
    int m_file;
#endif
    char m_device_id[256];
};

#endif /* GG_ENABLE_PHYSICAL_CDROM */
#endif /* CDROM_DRIVE_H */
