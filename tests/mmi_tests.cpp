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

#include <stdio.h>
#include <string.h>
#include <sstream>
#include <vector>
#include "cdrom_mmi_image.h"
#include "cdrom_media.h"
#include "media_file.h"
#include "mmi_archive.h"
#include "laseractive.h"
#include "geargrafx_core.h"
#include "qoi2.h"
#include "json.hpp"
#include <unistd.h>

static bool add_zip_entry(mz_zip_archive& zip, const char* name, const void* data,
    size_t size, bool compress)
{
    mz_uint flags = compress ? MZ_DEFAULT_COMPRESSION : MZ_NO_COMPRESSION;
    return mz_zip_writer_add_mem(&zip, name, data, size, flags) != 0;
}

static u16 subchannel_crc(const u8* data)
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

static bool create_mmi_fixture(const char* path, bool transport, u64 reserve, u32 sector_count)
{
    static const char media_info[] =
        "{"
        "\"name\":\"Synthetic LD-ROM2\","
        "\"system\":\"LDROM2\","
        "\"regionCode\":\"\","
        "\"catalogId\":\"TEST-0001\","
        "\"media\":[{"
        "\"name\":\"Disc 1 Side A\","
        "\"type\":\"LD\","
        "\"format\":\"NTSC CAV\","
        "\"sequenceNo\":0,"
        "\"volumeNo\":1,"
        "\"sideNo\":1,"
        "\"physicalType\":\"Synthetic\","
        "\"masterReference\":\"Unit test\","
        "\"streams\":["
        "{\"name\":\"DigitalAudio\",\"type\":\"Redbook\",\"file\":\"disc/disc.cue\","
        "\"format\":\"CUE/BIN\",\"channels\":2,\"framesInActiveRegion\":0,"
        "\"framesInLeadInRegion\":0,\"framesInLeadOutRegion\":0},"
        "{\"name\":\"AnalogAudio\",\"type\":\"RawAudio\",\"file\":\"analog/audio.raw\","
        "\"format\":\"S16LE\",\"channels\":2,\"framesInActiveRegion\":1,"
        "\"framesInLeadInRegion\":0,\"framesInLeadOutRegion\":0},"
        "{\"name\":\"AnalogVideo\",\"type\":\"RawVideo\",\"file\":\"analog/video.qon\","
        "\"format\":\"QON/QOI2\",\"channels\":3,\"framesInActiveRegion\":1,"
        "\"framesInLeadInRegion\":0,\"framesInLeadOutRegion\":0}"
        "]}]}";
    static const char cue[] = "FILE \"disc.bin\" BINARY\n  TRACK 01 MODE1/2352\n    INDEX 01 00:00:00\n";

    std::vector<u8> sectors(2352 * sector_count, 0);
    for (u32 sector = 0; sector < sector_count; sector++)
    {
        for (int i = 0; i < 2048; i++)
            sectors[(sector * 2352) + 16 + i] = (u8)((i + sector) & 0xFF);
    }

    std::vector<u8> analog_audio(transport ? 2352 * (sector_count + 151) : 4);
    for (size_t i = 0; i < analog_audio.size(); i += 4)
    {
        write_u16_le(&analog_audio[i], 0x1234);
        write_u16_le(&analog_audio[i + 2], 0xEDCC);
    }
    std::vector<u8> subchannel(96 * sector_count, 0);
    for (u32 sector = 0; sector < sector_count; sector++)
    {
        u8 q[12] = { 0x41, 0x01, 0x01, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x02, (u8)sector, 0x00, 0x00 };
        q[3] = DecToBcd(sector / (60 * 75));
        q[4] = DecToBcd((sector / 75) % 60);
        q[5] = DecToBcd(sector % 75);
        q[7] = DecToBcd((sector + 150) / (60 * 75));
        q[8] = DecToBcd(((sector + 150) / 75) % 60);
        q[9] = DecToBcd((sector + 150) % 75);
        u16 q_crc = subchannel_crc(q);
        q[10] = (u8)(q_crc >> 8);
        q[11] = (u8)q_crc;
        for (u32 bit = 0; bit < 96; bit++)
        {
            subchannel[(sector * 96) + bit] =
                (u8)(((q[bit >> 3] >> (7 - (bit & 7))) & 1) << 6);
        }
    }

    u8 qon[49] = {};
    memcpy(qon, "qon1", 4);
    write_u32_le(qon + 4, 2);
    write_u32_le(qon + 8, 2);
    qon[12] = 3;
    qon[13] = 0;
    write_u16_le(qon + 14, 0);
    write_u32_le(qon + 16, 1);
    write_u32_le(qon + 20, 33367);
    write_u32_le(qon + 32, 13);
    qon[36] = 0xFF;
    qon[37] = 0xFF; qon[38] = 0x00; qon[39] = 0x00;
    qon[40] = 0x00; qon[41] = 0xFF; qon[42] = 0x00;
    qon[43] = 0x00; qon[44] = 0x00; qon[45] = 0xFF;
    qon[46] = 0xFF; qon[47] = 0xFF; qon[48] = 0xFF;

    std::vector<u8> video(qon, qon + sizeof(qon));
    nlohmann::json manifest = nlohmann::json::parse(media_info);
    if (transport)
    {
        const u32 width = 384;
        const u32 height = 525;
        const u32 frames = MAX(120U, (sector_count + 150) * 30 / 75 + 1);
        std::vector<u8> rgb(width * height * 3, 0);
        for (u32 y = 0; y < height; y++)
            for (u32 x = 0; x < width; x++)
                rgb[(y * width + x) * 3 + (y >= 263 ? 1 : 0)] = 255;
        qoi2_desc descriptor = { width, height, 3, 0 };
        std::vector<u8> compressed(rgb.size() * 2);
        size_t size = compressed.size();
        if (!qoi2_encode_data(&rgb[0], 3, &descriptor, NULL, &compressed[0], &size))
            return false;
        video.assign(24 + frames * 8 + 4 + size, 0);
        memcpy(&video[0], "qon1", 4);
        write_u32_le(&video[4], width);
        write_u32_le(&video[8], height);
        video[12] = 3;
        write_u32_le(&video[16], frames);
        write_u32_le(&video[20], 0);
        write_u32_le(&video[24 + frames * 8], (u32)size);
        memcpy(&video[28 + frames * 8], &compressed[0], size);
        manifest["media"][0]["streams"][2]["framesInActiveRegion"] = frames;
        manifest["media"][0]["streams"][2]["framesInLeadOutRegion"] = 1;
        manifest["media"].push_back(manifest["media"][0]);
        manifest["media"][1]["name"] = "Disc 1 Side B";
        manifest["media"][1]["sequenceNo"] = 5;
        manifest["media"][1]["sideNo"] = 2;
        manifest["media"][1]["format"] = "NTSC CLV";
        manifest["media"][1]["streams"][0]["file"] = "disc/other.cue";
        manifest["media"].push_back(manifest["media"][0]);
        manifest["media"][2]["name"] = "Invalid side";
        manifest["media"][2]["sequenceNo"] = 9;
        manifest["media"][2]["streams"][2]["file"] = "bad.qon";
        // Released NEC MMIs omit Redbook/audio region counts and video channels.
        for (size_t side = 0; side < manifest["media"].size(); side++)
        {
            nlohmann::json& streams = manifest["media"][side]["streams"];
            streams[0].erase("format");
            streams[0].erase("channels");
            streams[2].erase("channels");
            for (int role = 0; role < 2; role++)
            {
                streams[role].erase("framesInActiveRegion");
                streams[role].erase("framesInLeadInRegion");
                streams[role].erase("framesInLeadOutRegion");
            }
        }
    }
    std::string manifest_text = manifest.dump();
    static const char other_cue[] = "FILE \"other.bin\" BINARY\n  TRACK 01 MODE1/2352\n    INDEX 01 00:00:00\n";
    std::vector<u8> other_sectors(sectors);
    other_sectors[2352 + 16] ^= 0xFF;

    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);
    if (!mz_zip_writer_init_file_v2(&zip, path, reserve, reserve ? MZ_ZIP_FLAG_WRITE_ZIP64 : 0))
        return false;

    bool ok = add_zip_entry(zip, "MediaInfo.json", manifest_text.data(), manifest_text.size(), true) &&
        add_zip_entry(zip, "disc/disc.cue", cue, sizeof(cue) - 1, true) &&
        add_zip_entry(zip, "disc/disc.bin", &sectors[0], sectors.size(), false) &&
        add_zip_entry(zip, "disc/other.cue", other_cue, sizeof(other_cue) - 1, false) &&
        add_zip_entry(zip, "disc/other.bin", &other_sectors[0], other_sectors.size(), false) &&
        add_zip_entry(zip, "disc/disc.sub", &subchannel[0], subchannel.size(), false) &&
        add_zip_entry(zip, "analog/audio.raw", &analog_audio[0], analog_audio.size(), false) &&
        add_zip_entry(zip, "analog/video.qon", &video[0], video.size(), false) &&
        add_zip_entry(zip, "bad.qon", qon, 4, false) &&
        (mz_zip_writer_finalize_archive(&zip) != 0);

    mz_zip_writer_end(&zip);
    return ok;
}

bool create_mmi_test_fixture(const char* path, bool transport, u64 reserve)
{
    return create_mmi_fixture(path, transport, reserve, 10);
}

static bool create_unsafe_mmi(const char* path)
{
    static const u8 value = 0;
    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);
    if (!mz_zip_writer_init_file(&zip, path, 0))
        return false;

    bool ok = add_zip_entry(zip, "../escape.bin", &value, sizeof(value), false) &&
        (mz_zip_writer_finalize_archive(&zip) != 0);
    mz_zip_writer_end(&zip);
    return ok;
}

static bool expect(bool condition, const char* message)
{
    if (!condition)
        Error("MMI test failed: %s", message);
    return condition;
}

static bool create_malformed_fixture(const char* source, const char* destination, int variant)
{
    mz_zip_archive reader;
    mz_zip_archive writer;
    mz_zip_zero_struct(&reader);
    mz_zip_zero_struct(&writer);
    if (!mz_zip_reader_init_file(&reader, source, 0))
        return false;
    if (!mz_zip_writer_init_file(&writer, destination, 0))
    {
        mz_zip_reader_end(&reader);
        return false;
    }
    bool ok = true;
    for (u32 i = 0; ok && i < mz_zip_reader_get_num_files(&reader); i++)
    {
        mz_zip_archive_file_stat entry;
        ok = mz_zip_reader_file_stat(&reader, i, &entry) != 0;
        if (!ok)
            break;
        std::vector<u8> bytes((size_t)entry.m_uncomp_size);
        ok = mz_zip_reader_extract_to_mem(&reader, i, &bytes[0], bytes.size(), 0) != 0;
        if (!ok)
            break;
        if (strcmp(entry.m_filename, "MediaInfo.json") == 0)
        {
            nlohmann::json manifest = nlohmann::json::parse(bytes);
            nlohmann::json& medium = manifest["media"][0];
            if (variant == 0) medium["sequenceNo"] = -1;
            if (variant == 1) medium["streams"][2]["framesInActiveRegion"] = UINT64_MAX;
            if (variant == 2) medium["streams"][1]["channels"] = "two";
            if (variant == 3) medium["streams"].push_back(medium["streams"][0]);
            if (variant == 4) manifest["media"].push_back(medium);
            if (variant == 5) medium["streams"][0]["file"] = std::string("disc/disc.cue\0suffix", 20);
            if (variant == 6) manifest["system"] = "MegaLD";
            if (variant == 14)
            {
                manifest["system"] = "SuperCDROM2";
                for (size_t side = 0; side < manifest["media"].size(); side++)
                {
                    manifest["media"][side]["type"] = "CD";
                    nlohmann::json& streams = manifest["media"][side]["streams"];
                    streams.erase(streams.begin() + 1, streams.end());
                }
            }
            std::string text = manifest.dump();
            bytes.assign(text.begin(), text.end());
        }
        if (strcmp(entry.m_filename, "analog/video.qon") == 0)
        {
            if (variant == 7) write_u32_le(&bytes[4], UINT32_MAX);
            if (variant == 8) write_u16_le(&bytes[14], 1);
            if (variant == 9) write_u16_le(&bytes[30], 0x8000);
            if (variant == 10) write_u32_le(&bytes[32], UINT32_MAX);
            if (variant == 11) bytes.resize(28);
        }
        if (variant == 12 && strcmp(entry.m_filename, "analog/audio.raw") == 0)
            bytes.resize(4096, 0);
        ok = add_zip_entry(writer, entry.m_filename, &bytes[0], bytes.size(),
            variant == 12 && strcmp(entry.m_filename, "analog/audio.raw") == 0);
        if (variant == 13 && strcmp(entry.m_filename, "MediaInfo.json") == 0)
            ok = ok && add_zip_entry(writer, "mediainfo.JSON", &bytes[0], bytes.size(), false);
    }
    ok = ok && mz_zip_writer_finalize_archive(&writer);
    mz_zip_writer_end(&writer);
    mz_zip_reader_end(&reader);
    return ok;
}

static bool run_archive_tests(const char* source)
{
    char path[] = "/tmp/geargrafx-malformed-XXXXXX.mmi";
    int fixture = mkstemps(path, 4);
    if (fixture < 0)
        return false;
    close(fixture);
    bool ok = true;
    for (int variant = 0; variant < 14; variant++)
    {
        if (!expect(create_malformed_fixture(source, path, variant), "create malformed fixture"))
        {
            ok = false;
            break;
        }
        CdRomMmiImage image;
        image.Init();
        char message[80];
        snprintf(message, sizeof(message), "reject malformed MMI variant %d", variant);
        ok = expect(!image.LoadFromFile(path, false) && !image.IsReady(), message) && ok;
    }
    ok = expect(create_mmi_test_fixture(path, false, 0x100000000ULL + 4096),
        "create sparse ZIP64 archive past 4 GiB") && ok;
    {
        MmiArchive archive;
        ok = expect(archive.Open(path), "open sparse ZIP64 archive") && ok;
        const GG_MmiEntry* entry = archive.FindEntry("disc/disc.bin");
        ok = expect(entry && entry->data_offset > 0x100000000ULL,
            "preserve ZIP64 data offset") && ok;
        MediaFile* slice = entry ? archive.OpenStoredEntry(entry) : NULL;
        u8 data[4] = {};
        ok = expect(slice && slice->ReadAt(17, data, 4) && data[0] == 1 && data[3] == 4,
            "read stored stream beyond 4 GiB") && ok;
        SafeDelete(slice);
    }
    FILE* file = fopen(path, "r+b");
    if (file)
    {
        u8 locator[20] = {};
        bool patched = fseeko(file, -42, SEEK_END) == 0 &&
            fread(locator, 1, sizeof(locator), file) == sizeof(locator) &&
            read_u32_le(locator) == 0x07064B50;
        u64 zip64 = read_u64_le(locator + 8);
        u64 central_size = 128ULL * 1024 * 1024;
        if (patched && zip64 > central_size)
        {
            u64 central_offset = zip64 - central_size;
            u8 directory[16] = {};
            write_u32_le(directory, (u32)central_size);
            write_u32_le(directory + 8, (u32)central_offset);
            write_u32_le(directory + 12, (u32)(central_offset >> 32));
            patched = fseeko(file, (off_t)(zip64 + 40), SEEK_SET) == 0 &&
                fwrite(directory, 1, sizeof(directory), file) == sizeof(directory);
        }
        else
            patched = false;
        fclose(file);
        MmiArchive oversized;
        ok = expect(patched && !oversized.Open(path) &&
            strstr(oversized.GetLastError(), "allocation failed"), "bound ZIP64 central-directory allocation") && ok;
    }
    else
        ok = expect(false, "open malformed ZIP64 fixture") && ok;
    remove(path);
    return ok;
}

static bool send_scsi_command(ScsiController* scsi, const u8* command, size_t size)
{
    scsi->Reset(true);
    scsi->StartSelection();
    scsi->Clock(75000);
    for (size_t i = 0; i < size; i++)
    {
        if (!scsi->IsSignalSet(ScsiController::SCSI_SIGNAL_REQ))
            return false;
        scsi->WriteData(command[i]);
        scsi->SetSignal(ScsiController::SCSI_SIGNAL_ACK);
        scsi->Clock(1);
        scsi->ClearSignal(ScsiController::SCSI_SIGNAL_ACK);
        scsi->Clock(1);
        if (i + 1 < size)
            scsi->Clock(3000);
    }
    return true;
}

static void seek_laseractive_test(LaserActive& laser)
{
    laser.Reset();
    laser.WriteRegister(2, false, 5);
    laser.WriteRegister(0, false, 0);
    laser.WriteRegister(6, false, 2);
    laser.WriteRegister(0x0A, false, 8);
    laser.WriteRegister(0x0B, false, 0x50);
    laser.WriteRegister(2, false, 0x35);
    laser.WriteRegister(0, false, 0x81);
    for (int i = 0; i < 30; i++)
        laser.Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
}

static bool run_laseractive_search_fade_tests()
{
    char path[] = "/tmp/geargrafx-search-fade-XXXXXX.mmi";
    int fixture = mkstemps(path, 4);
    if (fixture < 0)
        return false;
    close(fixture);
    bool ok = expect(create_mmi_fixture(path, true, 0, 2400), "create longer search/fade fixture");
    {
        CdRomMedia media;
        media.Init();
        ok = expect(media.LoadMmiFromFile(path), "load search/fade fixture") && ok;
        LaserActive laser(&media);
        laser.Init(NULL, NULL);
        media.SetLaserActive(&laser);
        LaserActive::Status status;
        for (u32 side = 0; side < 2; side++)
        {
            ok = expect(media.EjectMmi() && media.SelectMmiMedia(side) && media.InsertMmi(),
                "select CAV/CLV search fixture") && ok;
            for (int reverse = 0; reverse < 2; reverse++)
            {
                seek_laseractive_test(laser);
                ok = expect(laser.GetHeadLba() == 500 && laser.GetDriveMode() == LaserActive::DRIVE_PAUSED,
                    "paused absolute Redbook seek starts each search at sector 500") && ok;
                laser.WriteRegister(3, false, reverse ? 0x3F : 0x37);
                laser.WriteRegister(2, false, 5);
                laser.WriteRegister(1, false, 0x80);
                for (int i = 0; i < 28; i++)
                    laser.Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
                s16 left, right;
                laser.Sample(left, right);
                ok = expect(laser.GetHeadLba() == 528 && left == 0x1234 && right == -0x1234 &&
                    laser.ReadRegister(7, true) == (reverse ? 0x3E : 0x36),
                    "both search directions play forwards with audio and normalize speed 7 to 6") && ok;
                laser.WriteRegister(2, false, 0x15);
                laser.Clock(GG_MASTER_CLOCK_RATE);
                laser.GetStatus(status);
                ok = expect(laser.GetHeadLba() == 528 && status.search_sectors == 28,
                    "pause preserves a partially completed search burst") && ok;
                laser.WriteRegister(2, false, 5);
                laser.WriteRegister(3, false, reverse ? 0x3E : 0x36);
                laser.GetStatus(status);
                ok = expect(status.search_sectors == 28, "equivalent speed 6/7 writes preserve the burst") && ok;
                std::stringstream saved;
                laser.SaveState(saved);
                for (int pass = 0; pass < 2; pass++)
                {
                    if (pass)
                    {
                        laser.Reset();
                        laser.LoadState(saved);
                    }
                    for (int i = 0; i < 27; i++)
                        laser.Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
                    ok = expect(laser.GetHeadLba() == 555, "search plays a full burst before jumping") && ok;
                    laser.Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
                    laser.GetStatus(status);
                    ok = expect(laser.GetHeadLba() == (reverse ? 256 : 856) && status.search_sectors == 0,
                        "search jumps four seconds and restores its mid-burst phase") && ok;
                    laser.Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
                    ok = expect(laser.GetHeadLba() == (reverse ? 257 : 857),
                        "playback resumes forwards after either jump direction") && ok;
                }
                laser.WriteRegister(3, false, 0);
                laser.GetStatus(status);
                ok = expect(status.search_sectors == 0, "leaving search clears its burst phase") && ok;
            }
        }

        seek_laseractive_test(laser);
        laser.WriteRegister(3, false, 0x37);
        laser.NotifyAudioStart(500, false);
        laser.SetAudioEnd(501);
        for (int i = 0; i < 40; i++)
            laser.Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
        laser.GetStatus(status);
        ok = expect(status.head_lba == 501 && status.drive_mode == LaserActive::DRIVE_STOPPED &&
            status.search_sectors == 0, "SCSI end-marked playback is not redirected by PD search mode") && ok;

        seek_laseractive_test(laser);
        laser.WriteRegister(1, false, 0x80);
        laser.WriteRegister(0x1E, false, 0x0A);
        s16 left = 0, right = 0;
        laser.Sample(left, right);
        ok = expect(left == 0x1234 && right == -0x1234, "fade starts at the current analog level") && ok;
        for (u32 i = 1; i <= GG_AUDIO_SAMPLE_RATE / 4; i++)
            laser.Sample(left, right);
        ok = expect(left == 0x1234 / 2 && right == -0x1234 / 2,
            "both analog channels reach half gain halfway through the linear fade") && ok;
        std::stringstream saved;
        laser.SaveState(saved);
        for (int pass = 0; pass < 2; pass++)
        {
            if (pass)
            {
                laser.Reset();
                laser.LoadState(saved);
            }
            for (u32 i = 0; i < GG_AUDIO_SAMPLE_RATE / 4; i++)
                laser.Sample(left, right);
            laser.GetStatus(status);
            ok = expect(left == 0 && right == 0 && status.analog_muted_left && status.analog_muted_right &&
                status.analog_fade_samples_left == 0 && status.analog_fade_samples_right == 0,
                "fade reaches true mute and survives mid-fade save/load") && ok;
        }
        // Version 38 stored only the final mute flags; do not invent an in-progress fade.
        std::string old_state = saved.str();
        old_state.resize(old_state.size() - 3 * sizeof(u32));
        std::stringstream legacy(old_state);
        laser.LoadState(legacy, 38);
        laser.Sample(left, right);
        ok = expect(!legacy.fail() && left == 0 && right == 0, "older states retain immediate mute semantics") && ok;

        laser.WriteRegister(0x1F, false, 128);
        laser.WriteRegister(0x1E, false, 2);
        laser.WriteRegister(0x1E, false, 0x0E);
        laser.Sample(left, right);
        ok = expect(left == 0x1234 / 2 && right == -0x1234 / 2,
            "single-channel fade starts from existing attenuation") && ok;
        laser.GetStatus(status);
        u32 remaining = status.analog_fade_samples_left;
        laser.WriteRegister(0x1E, false, 0x0E);
        laser.WriteRegister(0x1E, false, 0x0F);
        laser.GetStatus(status);
        ok = expect(status.analog_fade_samples_left == remaining && status.analog_fade_samples_right == 0,
            "repeated mute preserves the ramp; muting the second channel is immediate") && ok;
        laser.Sample(left, right);
        ok = expect(left > 0 && right == 0, "second-channel mute leaves the first channel fading") && ok;
        laser.WriteRegister(0x1E, false, 2);
        laser.WriteRegister(0, false, 0);
        laser.WriteRegister(0x1E, false, 0x0A);
        laser.GetStatus(status);
        ok = expect(status.input_frozen && status.analog_fade_samples_left == GG_AUDIO_SAMPLE_RATE / 2 &&
            status.analog_fade_samples_right == GG_AUDIO_SAMPLE_RATE / 2,
            "restoring attenuation rearms the fade, including writes while input is frozen") && ok;
        std::stringstream before_peek, after_peek;
        laser.SaveState(before_peek);
        for (int i = 0; i < 100; i++)
            laser.GetStatus(status);
        laser.SaveState(after_peek);
        ok = expect(before_peek.str() == after_peek.str(), "debugger reads do not advance fade or search") && ok;
    }
    remove(path);
    return ok;
}

static bool run_laseractive_tests()
{
    char path[] = "/tmp/geargrafx-transport-XXXXXX.mmi";
    int fixture = mkstemps(path, 4);
    if (fixture < 0)
        return false;
    close(fixture);
    if (!expect(create_mmi_test_fixture(path, true, 0), "create transport fixture"))
        return false;

    bool ok = true;
    {
        CdRomMedia media;
        media.Init();
        ok = expect(media.LoadMmiFromFile(path), "load multi-side transport fixture") && ok;
        LaserActive laser(&media);
        laser.Init(NULL, NULL);
        media.SetLaserActive(&laser);

        laser.WriteRegister(0x02, false, 5);
        std::stringstream before;
        laser.SaveState(before);
        LaserActive::Status status;
        for (int i = 0; i < 100; i++)
            laser.GetStatus(status);
        std::stringstream after;
        laser.SaveState(after);
        ok = expect(before.str() == after.str(), "debugger status must not change any emulated state") && ok;

        laser.WriteRegister(0x10, true, 0x5A);
        for (int i = 0; i < 10; i++)
            ok = expect(laser.PeekRegister(0x10, true) == 0x5A, "peek retains output cooldown") && ok;
        for (int i = 0; i < 5; i++)
            ok = expect(laser.ReadRegister(0x10, true) == 0x5A, "five CPU cooldown reads") && ok;
        ok = expect(laser.ReadRegister(0x10, true) == 0, "CPU reads consume output cooldown") && ok;

        laser.WriteRegister(0, false, 0);
        laser.WriteRegister(3, false, 0x34);
        laser.GetStatus(status);
        ok = expect(status.input_frozen && status.input_registers[3] == 0x34 &&
            status.live_input_registers[3] == 0 && status.output_registers[7] == 0,
            "debugger distinguishes pending frozen input from applied hardware state") && ok;
        laser.WriteRegister(0, false, 0x80);
        ok = expect(laser.ReadRegister(7, true) == 0x34, "unfreeze applies playback mode") && ok;
        laser.WriteRegister(5, false, 0xA0);
        ok = expect(laser.ReadRegister(0x11, true) == 0 && laser.ReadRegister(0x12, true) == 1,
            "TOC summary query has no track control flags") && ok;

        laser.Reset();
        laser.WriteRegister(2, false, 5);
        laser.WriteRegister(0, false, 0);
        laser.WriteRegister(6, false, 6);
        laser.WriteRegister(0x0A, false, 0x13);
        laser.WriteRegister(2, false, 0x35);
        laser.WriteRegister(0, false, 0x81);
        for (int i = 0; i < 30; i++)
            laser.Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
        ok = expect(laser.GetHeadLba() == -120 && laser.GetCurrentVideoFrame() == 12,
            "deferred CAV frame seek applies the complete target before its trigger") && ok;

        laser.Reset();
        laser.WriteRegister(2, false, 4);
        for (int i = 0; i < 30; i++)
            laser.Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
        ok = expect(laser.HasVideoFrame() && laser.GetHeadLba() == -150,
            "paused seek resolves signed head and decodes the target frame") && ok;

        std::vector<u8> pce(1176 * 4, 255);
        std::vector<u8> output(pce.size());
        std::vector<u8> classes(1176);
        for (int i = 0; i < 1176; i++)
            classes[i] = i % 4;
        u8 line_state[8] = { 0x80, 4, 0, 0, 0xFC, 0, 0xFC, 0 };
        laser.ComposeLine(0, &pce[0], &classes[0], line_state, &output[0], GG_PIXEL_RGBA8888);
        ok = expect(output[0] == 0 && output[4] == 255 && output[8] == 0 && output[12] == 255,
            "blank analog video still applies all four graphics faders") && ok;
        std::vector<u16> pce565(1176);
        std::vector<u16> output565(1176);
        std::vector<u8> pce888(1176 * 4, 255);
        for (int i = 0; i < 1176; i++)
        {
            pce565[i] = (u16)(i * 31);
            pce888[i * 4] = (u8)((((pce565[i] >> 11) & 31) * 255 + 15) / 31);
            pce888[i * 4 + 1] = (u8)((((pce565[i] >> 5) & 63) * 255 + 31) / 63);
            pce888[i * 4 + 2] = (u8)(((pce565[i] & 31) * 255 + 15) / 31);
        }
        line_state[0] = 0;
        laser.ComposeLine(0, &pce888[0], &classes[0], line_state,
            (u8*)&output565[0], GG_PIXEL_RGB565);
        ok = expect(pce565 == output565, "disabled mixing preserves every RGB565 bit") && ok;

        laser.WriteRegister(1, false, 0x80);
        laser.WriteRegister(0x0C, false, 0x20);
        laser.WriteRegister(0x1A, false, 0);
        laser.BeginVideoFrame();
        laser.CaptureVideoLineState(line_state);
        laser.ComposeLine(0, &pce[0], &classes[0], line_state, &output[0], GG_PIXEL_RGBA8888);
        ok = expect(output[0] == 255 && output[1] == 0 && output[2] == 0,
            "held odd field shows decoded red analog pixels") && ok;

        laser.WriteRegister(0x0C, false, 0x0B);
        laser.WriteRegister(2, false, 5);
        laser.WriteRegister(3, false, 0x20);
        laser.Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
        laser.CaptureVideoLineState(line_state);
        laser.ComposeLine(0, &pce[0], &classes[0], line_state, &output[0], GG_PIXEL_RGBA8888);
        ok = expect(output[0] == 255 && output[1] == 0,
            "newly latched field cannot tear the current display field") && ok;
        laser.BeginVideoFrame();
        laser.CaptureVideoLineState(line_state);
        laser.ComposeLine(0, &pce[0], &classes[0], line_state, &output[0], GG_PIXEL_RGBA8888);
        ok = expect(output[0] == 0 && output[1] == 255,
            "even field becomes visible at the next frame boundary") && ok;
        laser.WriteRegister(3, false, 0);
        for (int i = 0; i < 3; i++)
            laser.Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
        std::stringstream pending_frame;
        laser.SaveState(pending_frame);
        laser.Reset();
        laser.LoadState(pending_frame);
        std::stringstream pending_restored;
        laser.SaveState(pending_restored);
        ok = expect(!pending_frame.fail() && pending_frame.str() == pending_restored.str(),
            "state restores a displayed field older than the pending decoded frame") && ok;

        laser.WriteRegister(0x1F, false, 128);
        laser.WriteRegister(0x1E, false, 2);
        s16 left, right;
        laser.Sample(left, right);
        ok = expect(left == 0x1234 / 2 && right == -0x1234 / 2,
            "attenuation command uses the existing 1F value") && ok;

        laser.BeginVideoFrame();
        laser.BeginVideoFrame();
        std::stringstream saved;
        laser.SaveState(saved);
        laser.Reset();
        laser.LoadState(saved);
        std::stringstream restored;
        laser.SaveState(restored);
        ok = expect(!saved.fail() && saved.str() == restored.str(),
            "restoring held video preserves all serialized timing and field state") && ok;

        ok = expect(!media.SelectMmiMedia(1), "side change requires eject") && ok;
        ok = expect(media.EjectMmi() && media.SelectMmiMedia(1) && media.IsMmiEjected() &&
            media.GetSelectedMmiMedia()->sequence_number == 5 && media.InsertMmi(),
            "select non-contiguous sequence by index without a reset") && ok;
        laser.Reset();
        laser.WriteRegister(2, false, 5);
        laser.WriteRegister(0, false, 0);
        laser.WriteRegister(6, false, 6);
        laser.WriteRegister(0x0A, false, 1);
        laser.WriteRegister(0x0B, false, 0x15);
        laser.WriteRegister(2, false, 0x35);
        laser.WriteRegister(0, false, 0x81);
        for (int i = 0; i < 30; i++)
            laser.Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
        laser.GetStatus(status);
        ok = expect(laser.GetHeadLba() == -37 && status.paused &&
            laser.GetCurrentVideoFrame() == 45 && laser.ReadRegister(2, true) == 0xC6 &&
            (laser.ReadRegister(3, true) & 0xC0) == 0x80,
            "CLV time seek and second-side status use the selected medium") && ok;
        saved.clear();
        saved.seekg(0);
        laser.LoadState(saved);
        ok = expect(!saved.fail() && media.GetSelectedMmiMediaIndex() == 0 && !media.IsMmiEjected(),
            "state restore selects its original side") && ok;
        ok = expect(media.EjectMmi() && !media.SelectMmiMedia(2) &&
            media.GetSelectedMmiMediaIndex() == 0 && media.IsMmiEjected() && media.InsertMmi(),
            "invalid side preserves the previous usable side") && ok;
        laser.Reset();
        laser.WriteRegister(2, false, 5);
        for (int i = 0; i < 20; i++)
            laser.Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
        laser.WriteRegister(3, false, 0x11);
        for (int i = 0; i < 3; i++)
            laser.Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
        s32 held = laser.GetCurrentVideoFrame();
        u8 playback = laser.ReadRegister(7, true);
        for (int i = 0; i < 20; i++)
            laser.Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
        ok = expect(held == laser.GetCurrentVideoFrame() && playback == laser.ReadRegister(7, true),
            "single-frame skip holds once without toggling direction repeatedly") && ok;
    }
    GeargrafxCore* core = new GeargrafxCore;
    core->Init(NULL);
    core->GetMedia()->SetConsoleType(GG_CONSOLE_SGX);
    ok = expect(core->LoadMedia(path) && !core->GetMedia()->IsSGX(),
        "PAC machine overrides a stale SuperGrafx preference") && ok;
    Memory* memory = core->GetMemory();
    ok = expect(core->GetMedia()->GetMappedBiosSize() == 0x80000 &&
        memory->GetBankType(0x3F) == Memory::MEMORY_BANK_TYPE_BIOS &&
        memory->GetBankType(0x40) == Memory::MEMORY_BANK_TYPE_UNUSED &&
        memory->GetBankType(0x68) == Memory::MEMORY_BANK_TYPE_UNUSED,
        "debugger classifies PAC firmware, unused banks and locked SRAM") && ok;
    core->GetCDROM()->WriteRegister(0x18C0, 0xAA);
    core->GetCDROM()->WriteRegister(0x18C0, 0x55);
    ok = expect(memory->GetBankType(0x68) == Memory::MEMORY_BANK_TYPE_CARD_RAM &&
        memory->GetMemoryMap()[0x68] == memory->GetCardRAM() &&
        memory->GetMemoryMap()[0x7F] == memory->GetCardRAM() + 0x2E000,
        "PAC SRAM latch updates the CPU map and debugger classification") && ok;
    core->GetCDROM()->WriteRegister(0x1923, 0x22);
    ok = expect(core->GetCDROM()->ReadRegister(0x1947) == 0x22,
        "PD6103A input/output register address mapping") && ok;
    u32 crc = core->GetMedia()->GetCRC();
    size_t state_size = 0;
    ok = expect(core->SaveState((u8*)NULL, state_size), "query complete MMI state size") && ok;
    std::vector<u8> state(state_size);
    ok = expect(core->SaveState(&state[0], state_size), "save complete MMI core state") && ok;
    ok = expect(core->EjectLaserDisc() && core->SelectLaserDiscMedia(1) && core->InsertLaserDisc() &&
        core->GetMedia()->GetCRC() == crc, "archive identity remains stable across distinct Redbook sides") && ok;
    ok = expect(core->LoadState(&state[0], state.size()) &&
        core->GetCDROMMedia()->GetSelectedMmiMediaIndex() == 0 && core->GetLaserActive()->IsSramEnabled(),
        "complete core state restores its original side and SRAM latch") && ok;
    core->GetCDROMAudio()->StartAudio(0, true);
    core->GetCDROMAudio()->SetStopLBA(2, CdRomAudio::CD_AUDIO_STOP_EVENT_LOOP);
    for (int i = 0; i < 24; i++)
        core->GetLaserActive()->Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
    ok = expect(core->GetLaserActive()->GetDriveMode() == LaserActive::DRIVE_SEEKING,
        "SCSI audio end marker loops the shared transport") && ok;
    core->GetCDROMAudio()->SetStopLBA(2, CdRomAudio::CD_AUDIO_STOP_EVENT_STOP);
    for (int i = 0; i < 30; i++)
        core->GetLaserActive()->Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
    ok = expect(core->GetCDROMAudio()->GetCurrentState() == CdRomAudio::CD_AUDIO_STATE_STOPPED,
        "SCSI audio end marker stops the shared transport") && ok;

    ScsiController* scsi = core->GetScsiController();
    ok = expect(core->EjectLaserDisc(), "eject for SCSI command dispatch tests") && ok;
    const u8 ready_command[6] = {};
    ok = expect(send_scsi_command(scsi, ready_command, sizeof(ready_command)), "send test-unit-ready") && ok;
    s32 ready_cycles = *scsi->GetState()->NEXT_EVENT_CYCLES;
    u32 generation = core->GetCDROMMedia()->GetMediaGeneration();
    ok = expect(ready_cycles > 0 && core->EjectLaserDisc() && core->SelectLaserDiscMedia(0) &&
        *scsi->GetState()->NEXT_EVENT_CYCLES == ready_cycles &&
        core->GetCDROMMedia()->GetMediaGeneration() == generation,
        "repeated eject and same-side selection do not reset controller or media") && ok;
    scsi->Clock(450000);
    ok = expect(scsi->ReadData() == ScsiController::SCSI_STATUS_GOOD,
        "test-unit-ready retains PCE controller behavior while the tray is open") && ok;
    const u8 pause_command[10] = { 0xDA };
    ok = expect(send_scsi_command(scsi, pause_command, sizeof(pause_command)) &&
        scsi->ReadData() == ScsiController::SCSI_STATUS_CHECK_CONDITION,
        "media-dependent audio command checks generic disc readiness") && ok;
    const u8 read_command[6] = { 0x08, 0, 0, 0, 1, 0 };
    ok = expect(send_scsi_command(scsi, read_command, sizeof(read_command)) &&
        *scsi->GetState()->PHASE == ScsiController::SCSI_PHASE_DATA_IN,
        "sector read reaches its normal data path") && ok;
    scsi->Clock(GG_MASTER_CLOCK_RATE);
    ok = expect(scsi->ReadData() == ScsiController::SCSI_STATUS_CHECK_CONDITION,
        "ejected image read fails through the sector-loading error path") && ok;
    ok = expect(core->InsertLaserDisc(), "reinsert after SCSI dispatch tests") && ok;
    ok = expect(send_scsi_command(scsi, pause_command, sizeof(pause_command)) &&
        scsi->ReadData() == ScsiController::SCSI_STATUS_GOOD,
        "audio command proceeds normally after insertion") && ok;
    ok = expect(send_scsi_command(scsi, read_command, sizeof(read_command)),
        "start a sector read before hardware tray-open") && ok;
    s32 load_cycles = *scsi->GetState()->NEXT_LOAD_CYCLES;
    u32 load_count = *scsi->GetState()->LOAD_SECTOR_COUNT;
    memory->GetWorkingRAM()[0x100] = 0x5A;
    ok = expect(load_cycles > 0 && load_count == 1 && core->SaveState(&state[0], state_size),
        "save an inserted side with a pending sector read") && ok;
    core->GetCDROM()->WriteRegister(0x1922, 1);
    ok = expect(core->GetCDROMMedia()->IsMmiEjected() &&
        *scsi->GetState()->PHASE == ScsiController::SCSI_PHASE_BUS_FREE &&
        *scsi->GetState()->NEXT_LOAD_CYCLES == 0 && *scsi->GetState()->LOAD_SECTOR_COUNT == 0 &&
        memory->GetWorkingRAM()[0x100] == 0x5A && core->GetLaserActive()->IsSramEnabled(),
        "PD6103A tray-open cancels pending CD transactions without resetting machine memory") && ok;
    ok = expect(core->SelectLaserDiscMedia(1) && core->InsertLaserDisc() &&
        core->LoadState(&state[0], state.size()) &&
        core->GetCDROMMedia()->GetSelectedMmiMediaIndex() == 0 &&
        !core->GetCDROMMedia()->IsMmiEjected() &&
        *scsi->GetState()->PHASE == ScsiController::SCSI_PHASE_DATA_IN &&
        *scsi->GetState()->NEXT_LOAD_CYCLES == load_cycles &&
        *scsi->GetState()->LOAD_SECTOR_COUNT == load_count,
        "restoring a different side preserves the serialized SCSI transfer") && ok;
    const u8 subcode_command[10] = { ScsiController::SCSI_CMD_READ_SUBCODE_Q };
    core->GetLaserActive()->NotifyScsiSectorRead(1, true);
    ok = expect(send_scsi_command(scsi, subcode_command, sizeof(subcode_command)), "send captured subcode query") && ok;
    const std::vector<u8>& subcode = *scsi->GetState()->DATA_BUFFER;
    ok = expect(subcode.size() == 10 && subcode[1] == 0x41 && subcode[3] == 1 &&
        subcode[6] == 2 && subcode[8] == 2 && subcode[9] == 2,
        "captured MMI subcode overrides the synthesized response") && ok;
    core->GetLaserActive()->Reset();
    core->GetLaserActive()->WriteRegister(2, false, 4);
    for (int i = 0; i < 30; i++)
        core->GetLaserActive()->Clock(GG_MASTER_CLOCK_RATE / 75 + 1);
    ok = expect(send_scsi_command(scsi, subcode_command, sizeof(subcode_command)) &&
        subcode.size() == 10 && subcode[3] == 0 && subcode[5] == 2 && subcode[8] == 0,
        "LaserActive lead-in fallback retains signed position semantics") && ok;

    core->ResetMedia(false);
    HuC6270* vdc = core->GetHuC6270_1();
    HuC6260* vce = core->GetHuC6260();
    const u16 registers[15] = { 0, 0, 0, 0, 0, 0x80, 0, 0, 0, 0x40,
        0x0100, 0x041F, 0x0F02, 0x00EF, 0x00F6 };
    for (int reg = 5; reg < 15; reg++)
    {
        vdc->WriteRegister(0, (u8)reg);
        vdc->WriteRegister(2, (u8)registers[reg]);
        vdc->WriteRegister(3, (u8)(registers[reg] >> 8));
    }
    for (int i = 0; i < 512; i++)
        vce->GetColorTable()[i] = 0;
    vce->GetColorTable()[15] = 0x1FF;
    for (int i = 0; i < 0x800; i++)
        vdc->GetVRAM()[i] = i < 0x400 ? 0x100 : 0x220;
    for (int i = 0; i < 16; i++)
        vdc->GetVRAM()[0x2200 + i] = 0xFFFF;
    std::vector<u8> pixels(1176 * 263 * 4, 0);
    vce->SetBuffer(&pixels[0]);
    vce->Clock<false, true>(1365 * 263 * 3);
    bool clear = true;
    for (size_t i = 0; i < pixels.size(); i += 4)
        clear = clear && pixels[i] == 0 && pixels[i + 1] == 0 && pixels[i + 2] == 0;
    ok = expect(clear, "short horizontal timing must not advance vertical scroll twice per line") && ok;

    char cd_path[] = "/tmp/geargrafx-cd-mmi-XXXXXX.mmi";
    int cd_fixture = mkstemps(cd_path, 4);
    if (cd_fixture >= 0)
    {
        close(cd_fixture);
        core->GetMedia()->SetConsoleType(GG_CONSOLE_AUTO);
        ok = expect(create_malformed_fixture(path, cd_path, 14) && core->LoadMedia(cd_path) &&
            !core->GetMedia()->IsLaserActive(), "CD-only MMI uses ordinary CD hardware") && ok;
        state_size = 0;
        ok = expect(core->SaveState((u8*)NULL, state_size), "query CD-only MMI state size") && ok;
        state.resize(state_size);
        ok = expect(core->SaveState(&state[0], state_size) && core->EjectLaserDisc() &&
            core->SelectLaserDiscMedia(1) && core->InsertLaserDisc() &&
            core->LoadState(&state[0], state.size()) &&
            core->GetCDROMMedia()->GetSelectedMmiMediaIndex() == 0,
            "CD-only MMI save state restores its selected side") && ok;
        core->GetMedia()->Reset();
        remove(cd_path);
    }
    else
        ok = expect(false, "create CD-only MMI fixture") && ok;
    delete core;
    remove(path);
    return ok;
}

bool run_mmi_tests()
{
    char path[] = "/tmp/geargrafx-synthetic-XXXXXX.mmi";
    char unsafe_path[] = "/tmp/geargrafx-unsafe-XXXXXX.mmi";
    int fixture = mkstemps(path, 4);
    int unsafe_fixture = mkstemps(unsafe_path, 4);
    if (fixture < 0 || unsafe_fixture < 0)
        return false;
    close(fixture);
    close(unsafe_fixture);

    if (!expect(create_mmi_test_fixture(path, false, 0), "create synthetic archive"))
        return false;

    bool ok = expect(create_unsafe_mmi(unsafe_path), "create malformed archive");
    MmiArchive unsafe_archive;
    ok = expect(!unsafe_archive.Open(unsafe_path) &&
        (strstr(unsafe_archive.GetLastError(), "Unsafe") != NULL),
        "reject unsafe archive entry path") && ok;

    MmiArchive archive;
    ok = expect(archive.Open(path), archive.GetLastError()) && ok;
    const GG_MmiInfo* info = archive.GetInfo();
    ok = expect(ok && info && (info->system == "LDROM2"), "parse root metadata") && ok;
    ok = expect(info && (info->media.size() == 1) && info->media[0].laserdisc,
        "parse LaserDisc media") && ok;

    const GG_MmiEntry* entry = archive.ResolveEntry("disc/disc.cue", "disc.bin");
    ok = expect(!archive.ResolveEntry("disc/disc.cue", "/disc.bin") &&
        !archive.ResolveEntry("disc/disc.cue", "../disc.bin") &&
        archive.FindEntry("DISC/DISC.BIN"), "safe case-insensitive entry lookup") && ok;
    ok = expect(entry && (entry->uncompressed_size == (2352 * 10)),
        "resolve archive-backed CUE file") && ok;
    MediaFile* slice = entry ? archive.OpenStoredEntry(entry) : NULL;
    u8 bytes[4] = {};
    ok = expect(slice && slice->ReadAt(16, bytes, sizeof(bytes)) &&
        (bytes[0] == 0) && (bytes[1] == 1) && (bytes[2] == 2) && (bytes[3] == 3),
        "bounded stored-entry read") && ok;
    SafeDelete(slice);
    archive.Close();

    CdRomMmiImage image;
    image.Init();
    ok = expect(image.LoadFromFile(path, false), "load MMI digital disc") && ok;
    const GG_QonInfo* qon = image.GetQonInfo();
    ok = expect(qon && (qon->width == 2) && (qon->height == 2) && (qon->frames.size() == 1),
        "index QON stream") && ok;
    std::vector<u8> decoded_frame;
    ok = expect(image.DecodeQonFrame(0, decoded_frame) && (decoded_frame.size() == 12) &&
        (decoded_frame[0] == 0xFF) && (decoded_frame[1] == 0x00) &&
        (decoded_frame[4] == 0xFF) && (decoded_frame[8] == 0xFF) &&
        (decoded_frame[9] == 0xFF) && (decoded_frame[10] == 0xFF) &&
        (decoded_frame[11] == 0xFF), "decode QOI2 frame exactly") && ok;
    qoi2_desc malformed_descriptor = { 2, 2, 3, 0 };
    u8 malformed_qoi[] = { 0xFF, 0x10, 0x20, 0x30 };
    u8 malformed_output[12] = {};
    ok = expect(!qoi2_decode_data(malformed_qoi, sizeof(malformed_qoi),
        &malformed_descriptor, NULL, malformed_output, 3),
        "reject truncated QOI2 RGB run") && ok;
    u8 data[2048] = {};
    ok = expect(image.ReadSector(0, data) && (data[0] == 0) && (data[1] == 1) &&
        (data[2047] == 0xFF), "read MMI-backed Redbook sector") && ok;
    ok = expect(image.ReadSector(8, data) && (data[0] == 8) && (data[2047] == 7) &&
        image.ReadSector(0, data) && (data[0] == 0),
        "evict and reload bounded MMI Redbook chunks") && ok;
    u8 decoded_q[12] = {};
    u8 expected_q[12] = { 0x41, 0x01, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x02, 0x00, 0x00, 0x00 };
    u16 expected_q_crc = subchannel_crc(expected_q);
    expected_q[10] = (u8)(expected_q_crc >> 8);
    expected_q[11] = (u8)expected_q_crc;
    ok = expect(image.ReadSubchannelQ(0, decoded_q) &&
        (memcmp(decoded_q, expected_q, sizeof(expected_q)) == 0),
        "read captured MMI Q subchannel") && ok;
    image.Reset();

    CdRomMedia cdrom_media;
    cdrom_media.Init();
    ok = expect(cdrom_media.LoadMmiFromFile(path), "load MMI for PD6103A tests") && ok;
    LaserActive laseractive(&cdrom_media);
    laseractive.Init(NULL, NULL);
    cdrom_media.SetLaserActive(&laseractive);
    laseractive.WriteRegister(0x03, false, 0x22);
    ok = expect(laseractive.ReadRegister(0x07, true) == 0x22,
        "latch PD6103A playback mode") && ok;
    laseractive.WriteRegister(0x10, true, 0x5A);
    ok = expect(laseractive.ReadRegister(0x10, true) == 0x5A,
        "retain PD6103A output write during cooldown") && ok;
    laseractive.WriteSramControl(0xAA);
    laseractive.WriteSramControl(0x55);
    ok = expect(laseractive.IsSramEnabled(), "latch PAC SRAM AA55 sequence") && ok;
    ok = expect(cdrom_media.EjectMmi() && cdrom_media.SelectMmiMedia(0) &&
        cdrom_media.IsMmiEjected() && cdrom_media.InsertMmi(),
        "change MMI media only while ejected") && ok;

    std::stringstream state(std::ios::in | std::ios::out | std::ios::binary);
    laseractive.SaveState(state);
    laseractive.Reset();
    state.seekg(0);
    laseractive.LoadState(state);
    LaserActive::Status status;
    laseractive.GetStatus(status);
    ok = expect(!state.fail() && status.sram_enabled &&
        (status.output_registers[0x07] == 0x22), "restore side-aware LaserActive state") && ok;

    GeargrafxCore* integrated_core = new GeargrafxCore;
    integrated_core->Init(NULL);
    ok = expect(integrated_core->LoadMedia(path) && integrated_core->LoadMedia(path),
        "stop MMI worker before replacing loaded media") && ok;
    integrated_core->GetMedia()->Reset();
    SafeDelete(integrated_core);

    ok = run_archive_tests(path) && ok;
    remove(path);
    remove(unsafe_path);
    ok = run_laseractive_tests() && ok;
    ok = run_laseractive_search_fade_tests() && ok;
    return ok;
}
