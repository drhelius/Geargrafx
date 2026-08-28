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

static bool create_synthetic_mmi(const char* path)
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

    u8 sectors[2352 * 10] = {};
    for (int sector = 0; sector < 10; sector++)
    {
        for (int i = 0; i < 2048; i++)
            sectors[(sector * 2352) + 16 + i] = (u8)((i + sector) & 0xFF);
    }

    u8 analog_audio[4] = { 0x34, 0x12, 0xCC, 0xED };
    u8 subchannel[96 * 10] = {};
    for (u32 sector = 0; sector < 10; sector++)
    {
        u8 q[12] = { 0x41, 0x01, 0x01, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x02, (u8)sector, 0x00, 0x00 };
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

    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);
    if (!mz_zip_writer_init_file(&zip, path, 0))
        return false;

    bool ok = add_zip_entry(zip, "MediaInfo.json", media_info, sizeof(media_info) - 1, true) &&
        add_zip_entry(zip, "disc/disc.cue", cue, sizeof(cue) - 1, true) &&
        add_zip_entry(zip, "disc/disc.bin", sectors, sizeof(sectors), false) &&
        add_zip_entry(zip, "disc/disc.sub", subchannel, sizeof(subchannel), false) &&
        add_zip_entry(zip, "analog/audio.raw", analog_audio, sizeof(analog_audio), false) &&
        add_zip_entry(zip, "analog/video.qon", qon, sizeof(qon), false) &&
        (mz_zip_writer_finalize_archive(&zip) != 0);

    mz_zip_writer_end(&zip);
    return ok;
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

bool run_mmi_tests()
{
    const char* path = "/private/tmp/geargrafx-synthetic-mmi-test.mmi";
    const char* unsafe_path = "/private/tmp/geargrafx-unsafe-mmi-test.mmi";
    remove(path);
    remove(unsafe_path);

    if (!expect(create_synthetic_mmi(path), "create synthetic archive"))
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

    remove(path);
    remove(unsafe_path);
    return ok;
}
