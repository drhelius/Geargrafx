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

#include "cdrom_drive.h"

#if defined(GG_ENABLE_PHYSICAL_CDROM)

#if !defined(_WIN32)
#define CDROM_DRIVE_DATA_SECTOR_SIZE 2048
#define CDROM_DRIVE_DATA_SECTOR_OFFSET 16
#define CDROM_DRIVE_RAW_SECTOR_SIZE 2352
#endif

CdRomDrive::CdRomDrive()
{
#if defined(_WIN32)
    m_file = INVALID_HANDLE_VALUE;
#else
    m_file = -1;
#endif
    m_device_id[0] = 0;
}

CdRomDrive::~CdRomDrive()
{
    Close();
}

bool CdRomDrive::IsOpen() const
{
#if defined(_WIN32)
    return m_file != INVALID_HANDLE_VALUE;
#else
    return m_file >= 0;
#endif
}

bool CdRomDrive::ReadRawSector2352(u32 lba, u8* buffer, bool audio, bool report_errors)
{
    return ReadRawSectors2352(lba, 1, buffer, audio, report_errors);
}

#if !defined(_WIN32)
bool CdRomDrive::ReadDataSector2048(u32 lba, u8* buffer, bool report_errors)
{
    if (!IsValidPointer(buffer))
        return false;

    u8 raw[CDROM_DRIVE_RAW_SECTOR_SIZE];
    if (!ReadRawSector2352(lba, raw, false, report_errors))
        return false;

    memcpy(buffer, raw + CDROM_DRIVE_DATA_SECTOR_OFFSET, CDROM_DRIVE_DATA_SECTOR_SIZE);
    return true;
}
#endif

const char* CdRomDrive::GetDeviceId() const
{
    return m_device_id;
}

#endif /* GG_ENABLE_PHYSICAL_CDROM */
