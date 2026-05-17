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

const char* CdRomDrive::GetDeviceId() const
{
    return m_device_id;
}

#endif /* GG_ENABLE_PHYSICAL_CDROM */
