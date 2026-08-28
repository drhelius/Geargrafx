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

#ifndef CDROM_MMI_IMAGE_H
#define CDROM_MMI_IMAGE_H

#include "cdrom_cuebin_image.h"
#include "mmi_archive.h"

struct GG_QonFrameInfo
{
    u64 record_offset;
    u32 compressed_size;
    u16 flags;
};

struct GG_QonInfo
{
    u32 width;
    u32 height;
    u8 channels;
    u8 colorspace;
    u16 flags;
    u32 frame_count;
    u32 frame_duration_us;
    u64 decoded_rgb_size;
    std::vector<GG_QonFrameInfo> frames;
};

class CdRomMmiImage : public CdRomCueBinImage
{
public:
    CdRomMmiImage();
    virtual ~CdRomMmiImage();
    virtual void Init() override;
    virtual void Reset() override;
    virtual bool LoadFromFile(const char* path, bool preload) override;
    virtual bool ReadSector(u32 lba, u8* buffer) override;
    virtual bool ReadSamples(u32 lba, u32 offset, s16* buffer, u32 count) override;
    virtual bool ReadSubchannelQ(s32 lba, u8* buffer) override;

    bool SelectMediaByIndex(u32 index);
    bool SelectMediaBySequence(s64 sequence_number);
    u32 GetSelectedMediaIndex() const;
    const GG_MmiInfo* GetMmiInfo() const;
    const GG_MmiMediaInfo* GetSelectedMedia() const;
    const GG_QonInfo* GetQonInfo() const;
    bool IsLaserDisc() const;
    bool IsEjected() const;
    void SetEjected(bool ejected);
    bool ReadAnalogAudio(u64 offset, void* buffer, u32 size);
    u64 GetAnalogAudioSize() const;
    bool ReadVideoData(u64 offset, void* buffer, u32 size);
    MediaFile* OpenVideoStream() const;
    bool DecodeQonFrame(u32 frame, std::vector<u8>& output);

private:
    static MediaFile* ResolveMmiFile(const char* reference, char* resolved_path,
        size_t resolved_path_size, void* user_data);
    bool LoadSelectedMedia(u32 index);
    bool IndexQon(MediaFile* video_file, const GG_MmiStreamInfo& stream, GG_QonInfo& info);
    bool FindSubchannelEntry(const GG_MmiEntry* cue_entry, const GG_MmiEntry*& entry) const;
    bool OpenSubchannel(const GG_MmiEntry* entry, MediaFile*& file, s32& first_lba);
    static bool DecodeSubchannelQ(const u8* raw, u8* q);
    static bool ValidateSubchannelQ(const u8* q);
    const GG_MmiStreamInfo* FindStream(const GG_MmiMediaInfo& media, GG_MmiStreamRole role) const;
    void ResetSelectedMedia();

private:
    MmiArchive m_archive;
    MediaFile* m_analog_audio_file;
    MediaFile* m_video_file;
    MediaFile* m_subchannel_file;
    GG_QonInfo m_qon;
    u32 m_selected_media_index;
    std::string m_selected_cue_entry;
    bool m_laserdisc;
    bool m_ejected;
    s32 m_subchannel_first_lba;
    u64 m_subchannel_sector_count;
};

#endif /* CDROM_MMI_IMAGE_H */
