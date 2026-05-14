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

#if defined(GG_ENABLE_PHYSICAL_CDROM) && defined(__linux__)

#include <algorithm>
#include <fcntl.h>
#include <limits.h>
#include <linux/cdrom.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

#define CDROM_DRIVE_RAW_SECTOR_SIZE 2352
#define CDROM_DRIVE_FIRST_PROBED_SR 0
#define CDROM_DRIVE_LAST_PROBED_SR 7

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

static void lba_to_msf(u32 lba, cdrom_msf* msf)
{
    lba += 150;
    msf->cdmsf_min0 = (u8)(lba / (60 * 75));
    msf->cdmsf_sec0 = (u8)((lba / 75) % 60);
    msf->cdmsf_frame0 = (u8)(lba % 75);
    msf->cdmsf_min1 = 0;
    msf->cdmsf_sec1 = 0;
    msf->cdmsf_frame1 = 0;
}

static u32 clamp_linux_lba(int lba)
{
    return (lba < 0) ? 0 : (u32)lba;
}

static void get_drive_name(const char* device_id, char* name, size_t name_size)
{
    const char* slash = strrchr(device_id, '/');
    const char* basename = IsValidPointer(slash) ? slash + 1 : device_id;
    strncpy_fit(name, basename, name_size);
}

static bool canonical_device_id(const char* path, char* canonical, size_t canonical_size)
{
    if (!IsValidPointer(path) || !IsValidPointer(canonical) || (canonical_size == 0))
        return false;

    canonical[0] = 0;

    char resolved[PATH_MAX];
    if (realpath(path, resolved) != NULL)
    {
        strncpy_fit(canonical, resolved, canonical_size);
        return true;
    }

    strncpy_fit(canonical, path, canonical_size);
    return true;
}

static bool already_seen_device(const std::vector<std::string>& seen, const char* canonical)
{
    for (size_t i = 0; i < seen.size(); i++)
    {
        if (seen[i] == canonical)
            return true;
    }

    return false;
}

static bool probe_drive_path(const char* path, std::vector<CdRomDriveInfo>& drives, std::vector<std::string>& seen_devices)
{
    char canonical[PATH_MAX];
    if (!canonical_device_id(path, canonical, sizeof(canonical)))
        return false;

    if (already_seen_device(seen_devices, canonical))
        return false;

    int file = open(path, O_RDONLY | O_NONBLOCK);
    if (file < 0)
        return false;

    int capability = ioctl(file, CDROM_GET_CAPABILITY, 0);
    close(file);

    if (capability < 0)
        return false;

    seen_devices.push_back(canonical);

    CdRomDriveInfo info;
    init_drive_info(info);
    strncpy_fit(info.id, path, sizeof(info.id));
    get_drive_name(path, info.name, sizeof(info.name));

    CdRomDrive drive;
    std::vector<CdRomDriveTrackInfo> tracks;
    u32 lead_out = 0;
    if (drive.Open(info.id))
        info.has_disc = drive.ReadTOC(tracks, &lead_out);

    Debug("Linux physical CD-ROM candidate: id=%s name=%s has_disc=%s", info.id, info.name, info.has_disc ? "true" : "false");
    drives.push_back(info);
    return true;
}

bool CdRomDrive::ListDrives(std::vector<CdRomDriveInfo>& drives)
{
    drives.clear();
    Debug("Linux physical CD-ROM enumeration started");

    std::vector<std::string> seen_devices;
    probe_drive_path("/dev/cdrom", drives, seen_devices);

    for (int i = CDROM_DRIVE_FIRST_PROBED_SR; i <= CDROM_DRIVE_LAST_PROBED_SR; i++)
    {
        char path[64];
        snprintf(path, sizeof(path), "/dev/sr%d", i);
        probe_drive_path(path, drives, seen_devices);
    }

    for (int i = CDROM_DRIVE_FIRST_PROBED_SR; i <= CDROM_DRIVE_LAST_PROBED_SR; i++)
    {
        char path[64];
        snprintf(path, sizeof(path), "/dev/scd%d", i);
        probe_drive_path(path, drives, seen_devices);
    }

    Debug("Linux physical CD-ROM enumeration finished: %d drive(s)", (int)drives.size());
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

    int file = open(device_id, O_RDONLY | O_NONBLOCK);
    if (file < 0)
    {
        Error("Failed to open physical CD-ROM device %s for eject: %s", device_id, strerror(errno));
        return false;
    }

    ioctl(file, CDROM_LOCKDOOR, 0);

    bool ok = ioctl(file, CDROMEJECT, 0) == 0;
    int error = errno;
    close(file);

    if (!ok)
    {
        Error("Failed to eject physical CD-ROM device %s: %s", device_id, strerror(error));
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
    m_file = open(device_id, O_RDONLY | O_NONBLOCK);
    if (m_file < 0)
    {
        Error("Failed to open physical CD-ROM device %s: %s", device_id, strerror(errno));
        return false;
    }

    if (ioctl(m_file, CDROM_GET_CAPABILITY, 0) < 0)
    {
        Error("Physical CD-ROM capability query failed for %s: %s", device_id, strerror(errno));
        Close();
        return false;
    }

    strncpy_fit(m_device_id, device_id, sizeof(m_device_id));
    Debug("Physical CD-ROM device opened: %s", m_device_id);
    return true;
}

void CdRomDrive::Close()
{
    if (m_file >= 0)
    {
        close(m_file);
        m_file = -1;
    }

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

    cdrom_tochdr header = {};
    if (ioctl(m_file, CDROMREADTOCHDR, &header) < 0)
    {
        Error("CDROMREADTOCHDR failed for %s: %s", m_device_id, strerror(errno));
        return false;
    }

    Debug("Physical CD-ROM TOC header for %s: first=%u last=%u", m_device_id, header.cdth_trk0, header.cdth_trk1);

    if ((header.cdth_trk0 == 0) || (header.cdth_trk1 == 0) || (header.cdth_trk1 < header.cdth_trk0) || (header.cdth_trk1 > 99))
    {
        Error("Invalid physical CD-ROM TOC header for %s: first=%u last=%u", m_device_id, header.cdth_trk0, header.cdth_trk1);
        return false;
    }

    for (int track_number = header.cdth_trk0; track_number <= header.cdth_trk1; track_number++)
    {
        cdrom_tocentry entry = {};
        entry.cdte_track = (u8)track_number;
        entry.cdte_format = CDROM_LBA;

        if (ioctl(m_file, CDROMREADTOCENTRY, &entry) < 0)
        {
            Error("CDROMREADTOCENTRY failed for %s track %d: %s", m_device_id, track_number, strerror(errno));
            tracks.clear();
            return false;
        }

        CdRomDriveTrackInfo track;
        track.number = (u8)track_number;
        track.data = (entry.cdte_ctrl & CDROM_DATA_TRACK) != 0;
        track.start_lba = clamp_linux_lba(entry.cdte_addr.lba);
        tracks.push_back(track);
        Debug("Physical CD-ROM TOC track %u: start_lba=%u type=%s", track.number, track.start_lba, track.data ? "data" : "audio");
    }

    cdrom_tocentry lead_out = {};
    lead_out.cdte_track = CDROM_LEADOUT;
    lead_out.cdte_format = CDROM_LBA;
    if (ioctl(m_file, CDROMREADTOCENTRY, &lead_out) < 0)
    {
        Error("CDROMREADTOCENTRY failed for %s lead-out: %s", m_device_id, strerror(errno));
        tracks.clear();
        return false;
    }

    *lead_out_lba = clamp_linux_lba(lead_out.cdte_addr.lba);
    Debug("Physical CD-ROM TOC lead-out: lba=%u", *lead_out_lba);

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
    (void)audio;

    if (!IsOpen() || !IsValidPointer(buffer) || (sector_count == 0))
        return false;

    for (u32 i = 0; i < sector_count; i++)
    {
        u32 sector_lba = lba + i;
        u8* sector_buffer = buffer + (i * CDROM_DRIVE_RAW_SECTOR_SIZE);
        cdrom_msf* msf = (cdrom_msf*)sector_buffer;
        lba_to_msf(sector_lba, msf);

        if (ioctl(m_file, CDROMREADRAW, sector_buffer) < 0)
        {
            Error("CDROMREADRAW failed for %s at LBA %u: %s", m_device_id, sector_lba, strerror(errno));
            return false;
        }
    }

    return true;
}

bool CdRomDrive::SetSpeed(u16 speed)
{
    UNUSED(speed);
    Debug("Physical CD-ROM set speed %u for %s: no-op on Linux", speed, m_device_id);
    return IsOpen();
}

#endif /* GG_ENABLE_PHYSICAL_CDROM && __linux__ */