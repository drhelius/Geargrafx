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

#if defined(GG_ENABLE_PHYSICAL_CDROM) && defined(_WIN32)

#include <algorithm>
#include <stddef.h>
#include <winioctl.h>
#include <ntddcdrm.h>
#include <ntddscsi.h>

#define CDROM_DRIVE_RAW_SECTOR_SIZE 2352
#define CDROM_DRIVE_SENSE_SIZE 32
#define CDROM_DRIVE_TIMEOUT_SECONDS 20

struct ScsiPassThroughDirectWithSense
{
    SCSI_PASS_THROUGH_DIRECT pass_through;
    ULONG filler;
    UCHAR sense[CDROM_DRIVE_SENSE_SIZE];
};

struct GeargrafxCdRomSetSpeed
{
    ULONG request_type;
    USHORT read_speed;
    USHORT write_speed;
    ULONG rotation_control;
};

static void init_drive_info(CdRomDriveInfo& info)
{
    info.id[0] = 0;
    info.name[0] = 0;
    info.has_disc = false;
}

static bool drive_track_compare(const CdRomDriveTrackInfo& a, const CdRomDriveTrackInfo& b)
{
    return a.start_lba < b.start_lba;
}

static void format_windows_error(DWORD error, char* buffer, size_t buffer_size)
{
    if (!IsValidPointer(buffer) || (buffer_size == 0))
        return;

    buffer[0] = 0;

    DWORD length = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error, 0, buffer, (DWORD)buffer_size, NULL);

    if (length == 0)
    {
        snprintf(buffer, buffer_size, "Windows error %lu", error);
        return;
    }

    while ((length > 0) && ((buffer[length - 1] == '\r') || (buffer[length - 1] == '\n') || (buffer[length - 1] == '.') || (buffer[length - 1] == ' ')))
    {
        buffer[length - 1] = 0;
        length--;
    }
}

static void make_device_path(const char* device_id, char* path, size_t path_size)
{
    if (!IsValidPointer(path) || (path_size == 0))
        return;

    path[0] = 0;

    if (!IsValidPointer(device_id) || (device_id[0] == 0))
        return;

    if ((strlen(device_id) >= 4) && (device_id[0] == '\\') && (device_id[1] == '\\') && (device_id[2] == '.') && (device_id[3] == '\\'))
    {
        strncpy_fit(path, device_id, path_size);
        return;
    }

    if (((device_id[0] >= 'A') && (device_id[0] <= 'Z')) || ((device_id[0] >= 'a') && (device_id[0] <= 'z')))
    {
        if (device_id[1] == ':')
        {
            char letter = device_id[0];
            if ((letter >= 'a') && (letter <= 'z'))
                letter = (char)(letter - 'a' + 'A');

            snprintf(path, path_size, "\\\\.\\%c:", letter);
            return;
        }
    }

    strncpy_fit(path, device_id, path_size);
}

static u32 toc_address_to_lba(const UCHAR* address, bool msf)
{
    if (msf)
    {
        u32 lba = ((u32)address[1] * 60 * 75) + ((u32)address[2] * 75) + (u32)address[3];
        if (lba < 150)
            return 0;

        return lba - 150;
    }

    return ((u32)address[0] << 24) | ((u32)address[1] << 16) | ((u32)address[2] << 8) | (u32)address[3];
}

static bool read_toc_ioctl(HANDLE file, const char* device_id, CDROM_TOC* toc, bool* msf)
{
    if (!IsValidPointer(toc) || !IsValidPointer(msf))
        return false;

    memset(toc, 0, sizeof(CDROM_TOC));
    *msf = false;

    CDROM_READ_TOC_EX request = {};
    request.Format = CDROM_READ_TOC_EX_FORMAT_TOC;
    request.SessionTrack = 0;
    request.Msf = 0;

    DWORD bytes_returned = 0;
    if (DeviceIoControl(file, IOCTL_CDROM_READ_TOC_EX, &request, sizeof(request), toc, sizeof(CDROM_TOC), &bytes_returned, NULL))
        return true;

    DWORD error = GetLastError();
    char error_text[256];
    format_windows_error(error, error_text, sizeof(error_text));
    Debug("IOCTL_CDROM_READ_TOC_EX failed for %s: %s", device_id, error_text);

    memset(toc, 0, sizeof(CDROM_TOC));
    bytes_returned = 0;
    if (DeviceIoControl(file, IOCTL_CDROM_READ_TOC, NULL, 0, toc, sizeof(CDROM_TOC), &bytes_returned, NULL))
    {
        *msf = true;
        return true;
    }

    error = GetLastError();
    format_windows_error(error, error_text, sizeof(error_text));
    Error("IOCTL_CDROM_READ_TOC failed for %s: %s", device_id, error_text);
    return false;
}

static bool read_cd_spti(HANDLE file, u32 lba, u32 sector_count, u8* buffer, bool audio)
{
    if (!IsValidPointer(buffer) || (sector_count == 0))
        return false;

    ScsiPassThroughDirectWithSense request = {};
    request.pass_through.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    request.pass_through.CdbLength = 12;
    request.pass_through.SenseInfoLength = CDROM_DRIVE_SENSE_SIZE;
    request.pass_through.DataIn = SCSI_IOCTL_DATA_IN;
    request.pass_through.DataTransferLength = sector_count * CDROM_DRIVE_RAW_SECTOR_SIZE;
    request.pass_through.TimeOutValue = CDROM_DRIVE_TIMEOUT_SECONDS;
    request.pass_through.DataBuffer = buffer;
    request.pass_through.SenseInfoOffset = offsetof(ScsiPassThroughDirectWithSense, sense);
    request.pass_through.Cdb[0] = 0xBE;
    request.pass_through.Cdb[2] = (UCHAR)(lba >> 24);
    request.pass_through.Cdb[3] = (UCHAR)(lba >> 16);
    request.pass_through.Cdb[4] = (UCHAR)(lba >> 8);
    request.pass_through.Cdb[5] = (UCHAR)lba;
    request.pass_through.Cdb[6] = (UCHAR)(sector_count >> 16);
    request.pass_through.Cdb[7] = (UCHAR)(sector_count >> 8);
    request.pass_through.Cdb[8] = (UCHAR)sector_count;
    request.pass_through.Cdb[9] = audio ? 0x10 : 0xF8;

    DWORD bytes_returned = 0;
    if (!DeviceIoControl(file, IOCTL_SCSI_PASS_THROUGH_DIRECT, &request, sizeof(request), &request, sizeof(request), &bytes_returned, NULL))
        return false;

    return request.pass_through.ScsiStatus == 0;
}

static HANDLE open_device_handle(const char* device_id, bool read_write)
{
    char path[256];
    make_device_path(device_id, path, sizeof(path));

    if (path[0] == 0)
        return INVALID_HANDLE_VALUE;

    DWORD access = read_write ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ;
    return CreateFileA(path, access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

bool CdRomDrive::ListDrives(std::vector<CdRomDriveInfo>& drives)
{
    drives.clear();
    Debug("Windows physical CD-ROM enumeration started");

    char drive_strings[512];
    DWORD length = GetLogicalDriveStringsA(sizeof(drive_strings), drive_strings);
    if ((length == 0) || (length >= sizeof(drive_strings)))
    {
        DWORD error = GetLastError();
        char error_text[256];
        format_windows_error(error, error_text, sizeof(error_text));
        Error("GetLogicalDriveStringsA failed for physical CD-ROM enumeration: %s", error_text);
        return false;
    }

    const char* drive = drive_strings;
    while (drive[0] != 0)
    {
        if (GetDriveTypeA(drive) == DRIVE_CDROM)
        {
            CdRomDriveInfo info;
            init_drive_info(info);
            snprintf(info.id, sizeof(info.id), "%c:", drive[0]);

            char volume_name[128] = {};
            if (GetVolumeInformationA(drive, volume_name, sizeof(volume_name), NULL, NULL, NULL, NULL, 0) && (volume_name[0] != 0))
                snprintf(info.name, sizeof(info.name), "%s (%c:)", volume_name, drive[0]);
            else
                snprintf(info.name, sizeof(info.name), "%c:", drive[0]);

            CdRomDrive cdrom_drive;
            std::vector<CdRomDriveTrackInfo> tracks;
            u32 lead_out = 0;
            if (cdrom_drive.Open(info.id))
                info.has_disc = cdrom_drive.ReadTOC(tracks, &lead_out);

            Debug("Windows physical CD-ROM candidate: id=%s name=%s has_disc=%s", info.id, info.name, info.has_disc ? "true" : "false");
            drives.push_back(info);
        }

        drive += strlen(drive) + 1;
    }

    Debug("Windows physical CD-ROM enumeration finished: %d drive(s)", (int)drives.size());
    return true;
}

bool CdRomDrive::Eject(const char* device_id)
{
    if (!IsValidPointer(device_id) || (device_id[0] == 0))
    {
        Error("Invalid physical CD-ROM device id for eject");
        return false;
    }

    Debug("Ejecting physical CD-ROM device %s", device_id);

    HANDLE file = open_device_handle(device_id, true);
    if (file == INVALID_HANDLE_VALUE)
        file = open_device_handle(device_id, false);

    if (file == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        char error_text[256];
        format_windows_error(error, error_text, sizeof(error_text));
        Error("Failed to open physical CD-ROM device %s for eject: %s", device_id, error_text);
        return false;
    }

    PREVENT_MEDIA_REMOVAL removal = {};
    removal.PreventMediaRemoval = FALSE;
    DWORD bytes_returned = 0;
    DeviceIoControl(file, IOCTL_STORAGE_MEDIA_REMOVAL, &removal, sizeof(removal), NULL, 0, &bytes_returned, NULL);

    bool ok = DeviceIoControl(file, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &bytes_returned, NULL) != FALSE;
    DWORD error = GetLastError();
    CloseHandle(file);

    if (!ok)
    {
        char error_text[256];
        format_windows_error(error, error_text, sizeof(error_text));
        Error("Failed to eject physical CD-ROM device %s: %s", device_id, error_text);
        return false;
    }

    Log("Physical CD-ROM ejected: %s", device_id);
    return true;
}

bool CdRomDrive::Open(const char* device_id)
{
    if (!IsValidPointer(device_id) || (device_id[0] == 0))
    {
        Error("Invalid physical CD-ROM device id");
        return false;
    }

    Close();

    Debug("Opening physical CD-ROM device %s", device_id);
    m_file = open_device_handle(device_id, true);
    if (m_file == INVALID_HANDLE_VALUE)
    {
        DWORD read_write_error = GetLastError();
        m_file = open_device_handle(device_id, false);
        if (m_file == INVALID_HANDLE_VALUE)
        {
            DWORD read_error = GetLastError();
            char read_write_text[256];
            char read_text[256];
            format_windows_error(read_write_error, read_write_text, sizeof(read_write_text));
            format_windows_error(read_error, read_text, sizeof(read_text));
            Error("Failed to open physical CD-ROM device %s: read/write=%s, read-only=%s", device_id, read_write_text, read_text);
            return false;
        }

        Debug("Physical CD-ROM device %s opened read-only after read/write open failed", device_id);
    }

    strncpy_fit(m_device_id, device_id, sizeof(m_device_id));
    Debug("Physical CD-ROM device opened: %s", m_device_id);
    return true;
}

void CdRomDrive::Close()
{
    if (IsOpen())
        CloseHandle(m_file);

    m_file = INVALID_HANDLE_VALUE;
    m_device_id[0] = 0;
}

bool CdRomDrive::ReadTOC(std::vector<CdRomDriveTrackInfo>& tracks, u32* lead_out_lba)
{
    if (!IsOpen() || !IsValidPointer(lead_out_lba))
    {
        Error("ReadTOC failed - invalid physical CD-ROM drive state");
        return false;
    }

    tracks.clear();
    *lead_out_lba = 0;

    CDROM_TOC toc = {};
    bool msf = false;
    if (!read_toc_ioctl(m_file, m_device_id, &toc, &msf))
        return false;

    u16 toc_length = ((u16)toc.Length[0] << 8) | (u16)toc.Length[1];
    if (toc_length < 2)
    {
        Error("Invalid physical CD-ROM TOC length for %s: %u", m_device_id, toc_length);
        return false;
    }

    u32 descriptor_count = (toc_length - 2) / sizeof(TRACK_DATA);
    Debug("Physical CD-ROM TOC descriptors for %s: %u", m_device_id, descriptor_count);

    for (u32 i = 0; i < descriptor_count; i++)
    {
        TRACK_DATA& descriptor = toc.TrackData[i];

        if ((descriptor.TrackNumber >= 1) && (descriptor.TrackNumber <= 99))
        {
            CdRomDriveTrackInfo track;
            track.number = descriptor.TrackNumber;
            track.data = (descriptor.Control & 0x04) != 0;
            track.start_lba = toc_address_to_lba(descriptor.Address, msf);
            tracks.push_back(track);
            Debug("Physical CD-ROM TOC track %u: start_lba=%u type=%s", track.number, track.start_lba, track.data ? "data" : "audio");
        }
        else if (descriptor.TrackNumber == 0xAA)
        {
            *lead_out_lba = toc_address_to_lba(descriptor.Address, msf);
            Debug("Physical CD-ROM TOC lead-out: lba=%u", *lead_out_lba);
        }
    }

    std::sort(tracks.begin(), tracks.end(), drive_track_compare);

    if (tracks.empty() || (*lead_out_lba == 0))
    {
        Error("Invalid physical CD-ROM TOC for %s", m_device_id);
        tracks.clear();
        *lead_out_lba = 0;
        return false;
    }

    Debug("Physical CD-ROM TOC read complete for %s: tracks=%d lead_out=%u", m_device_id, (int)tracks.size(), *lead_out_lba);
    return true;
}

bool CdRomDrive::ReadRawSectors2352(u32 lba, u32 sector_count, u8* buffer, bool audio)
{
    if (!IsOpen() || !IsValidPointer(buffer) || (sector_count == 0))
        return false;

    if (read_cd_spti(m_file, lba, sector_count, buffer, audio))
        return true;

    if (sector_count == 1)
    {
        Error("SCSI READ CD failed for %s at LBA %u", m_device_id, lba);
        return false;
    }

    Debug("Physical CD-ROM block read fallback for %s at LBA %u (%u sectors)", m_device_id, lba, sector_count);

    for (u32 i = 0; i < sector_count; i++)
    {
        u32 sector_lba = lba + i;
        u8* sector_buffer = buffer + (i * CDROM_DRIVE_RAW_SECTOR_SIZE);
        if (!read_cd_spti(m_file, sector_lba, 1, sector_buffer, audio))
        {
            Error("SCSI READ CD failed for %s at LBA %u", m_device_id, sector_lba);
            return false;
        }
    }

    return true;
}

bool CdRomDrive::SetSpeed(u16 speed)
{
    if (!IsOpen())
        return false;

#if defined(IOCTL_CDROM_SET_SPEED)
    GeargrafxCdRomSetSpeed request = {};
    request.request_type = 0;
    request.read_speed = speed;
    request.write_speed = 0;
    request.rotation_control = 0;

    DWORD bytes_returned = 0;
    bool ok = DeviceIoControl(m_file, IOCTL_CDROM_SET_SPEED, &request, sizeof(request), NULL, 0, &bytes_returned, NULL) != FALSE;
    Debug("Physical CD-ROM set speed %u for %s: %s", speed, m_device_id, ok ? "ok" : "failed");
    return ok;
#else
    Debug("Physical CD-ROM set speed %u for %s: unsupported by this Windows SDK", speed, m_device_id);
    return false;
#endif
}

#endif /* GG_ENABLE_PHYSICAL_CDROM && _WIN32 */