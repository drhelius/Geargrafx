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

#define GUI_DEBUG_CDROM_IMPORT
#include "gui_debug_cdrom.h"

#include "imgui.h"
#include "geargrafx.h"
#include "gui_debug_constants.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

static void format_cdrom_msf(u32 lba, char* buffer, size_t size);

void gui_debug_window_cdrom(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(75, 80), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(210, 620), ImGuiCond_FirstUseEver);
    ImGui::Begin("CD-ROM", &config_debug.show_cdrom);

    ImGui::PushFont(gui_default_font);

    GeargrafxCore* core = emu_get_core();
    CdRom* cdrom = core->GetCDROM();
    CdRomMedia* cdrom_media = core->GetCDROMMedia();
    ScsiController* scsi_controller = core->GetScsiController();
    CdRom::CdRom_State* cdrom_state = cdrom->GetState();
    ScsiController::Scsi_State* scsi_state = scsi_controller->GetState();

    ImGui::TextColored(violet, "RESET  "); ImGui::SameLine();
    ImGui::TextColored(*cdrom_state->RESET ? green : gray, "%s", *cdrom_state->RESET ? "ON" : "OFF");

    ImGui::TextColored(violet, "BRAM   "); ImGui::SameLine();
    ImGui::TextColored(*cdrom_state->BRAM_ENABLED ? green : red, "%s", *cdrom_state->BRAM_ENABLED ? "UNLOCKED" : "LOCKED");

    ImGui::NewLine(); ImGui::TextColored(cyan, "ENABLED IRQS"); ImGui::Separator();

    ImGui::TextColored(violet, "ENABLED"); ImGui::SameLine();
    ImGui::TextColored(white, "$%02X ", *cdrom_state->ENABLED_IRQS); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(*cdrom_state->ENABLED_IRQS));

    ImGui::TextColored((*cdrom_state->ENABLED_IRQS & CDROM_IRQ_ADPCM_HALF) ? green : gray, "%s", (*cdrom_state->ENABLED_IRQS & CDROM_IRQ_ADPCM_HALF) ? "ADPCM H" : "ADPCM H"); ImGui::SameLine();

    ImGui::TextColored((*cdrom_state->ENABLED_IRQS & CDROM_IRQ_ADPCM_END) ? green : gray, "%s", (*cdrom_state->ENABLED_IRQS & CDROM_IRQ_ADPCM_END) ? "ADPCM E" : "ADPCM E"); ImGui::SameLine();

    ImGui::TextColored((*cdrom_state->ENABLED_IRQS & CDROM_IRQ_STATUS_AND_MSG_IN) ? green : gray, "%s", (*cdrom_state->ENABLED_IRQS & CDROM_IRQ_STATUS_AND_MSG_IN) ? "STATUS" : "STATUS"); ImGui::SameLine();

    ImGui::TextColored((*cdrom_state->ENABLED_IRQS & CDROM_IRQ_DATA_IN) ? green : gray, "%s", (*cdrom_state->ENABLED_IRQS & CDROM_IRQ_DATA_IN) ? "DATA" : "DATA");

    ImGui::NewLine(); ImGui::TextColored(cyan, "ACTIVE IRQS"); ImGui::Separator();

    ImGui::TextColored(violet, "ACTIVE "); ImGui::SameLine();
    ImGui::TextColored(white, "$%02X ", *cdrom_state->ACTIVE_IRQS); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(*cdrom_state->ACTIVE_IRQS));

    ImGui::TextColored((*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_ADPCM_HALF) ? green : gray, "%s", (*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_ADPCM_HALF) ? "ADPCM H" : "ADPCM H"); ImGui::SameLine();

    ImGui::TextColored((*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_ADPCM_END) ? green : gray, "%s", (*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_ADPCM_END) ? "ADPCM E" : "ADPCM E"); ImGui::SameLine();

    ImGui::TextColored((*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_STATUS_AND_MSG_IN) ? green : gray, "%s", (*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_STATUS_AND_MSG_IN) ? "STATUS" : "STATUS"); ImGui::SameLine();

    ImGui::TextColored((*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_DATA_IN) ? green : gray, "%s", (*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_DATA_IN) ? "DATA" : "DATA");

    ImGui::NewLine(); ImGui::TextColored(cyan, "SCSI"); ImGui::Separator();

    ImGui::TextColored(violet, "PHASE   "); ImGui::SameLine();
    ImGui::TextColored(blue, "%s", k_scsi_phase_names[*scsi_state->PHASE]);

    ImGui::TextColored(violet, "DATA BUS"); ImGui::SameLine();
    ImGui::TextColored(white, "$%02X ", *scsi_state->DB); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(*scsi_state->DB));

    ImGui::TextColored(violet, "BSY"); ImGui::SameLine();
    ImGui::TextColored((*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_BSY) ? green : gray, "%s", (*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_BSY) ? "ON " : "OFF"); ImGui::SameLine();

    ImGui::TextColored(violet, " SEL"); ImGui::SameLine();
    ImGui::TextColored((*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_SEL) ? green : gray, "%s", (*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_SEL) ? "ON " : "OFF"); ImGui::SameLine();

    ImGui::TextColored(violet, " C/D"); ImGui::SameLine();
    ImGui::TextColored((*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_CD) ? green : gray, "%s", (*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_CD) ? "ON " : "OFF");

    ImGui::TextColored(violet, "I/O"); ImGui::SameLine();
    ImGui::TextColored((*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_IO) ? green : gray, "%s", (*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_IO) ? "ON " : "OFF"); ImGui::SameLine();

    ImGui::TextColored(violet, " MSG"); ImGui::SameLine();
    ImGui::TextColored((*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_MSG) ? green : gray, "%s", (*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_MSG) ? "ON " : "OFF"); ImGui::SameLine();

    ImGui::TextColored(violet, " REQ"); ImGui::SameLine();
    ImGui::TextColored((*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_REQ) ? green : gray, "%s", (*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_REQ) ? "ON " : "OFF");

    ImGui::TextColored(violet, "ACK"); ImGui::SameLine();
    ImGui::TextColored((*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_ACK) ? green : gray, "%s", (*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_ACK) ? "ON " : "OFF"); ImGui::SameLine();

    ImGui::TextColored(violet, " ATN"); ImGui::SameLine();
    ImGui::TextColored((*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_ATN) ? green : gray, "%s", (*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_ATN) ? "ON " : "OFF"); ImGui::SameLine();

    ImGui::TextColored(violet, " RST"); ImGui::SameLine();
    ImGui::TextColored((*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_RST) ? green : gray, "%s", (*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_RST) ? "ON " : "OFF");

    ImGui::NewLine(); ImGui::TextColored(cyan, "EVENTS"); ImGui::Separator();

    ImGui::TextColored(violet, "NEXT EVENT "); ImGui::SameLine();
    ImGui::TextColored((*scsi_state->NEXT_EVENT == 0) ? gray : yellow, "%s", k_scsi_event_names[*scsi_state->NEXT_EVENT]);

    ImGui::TextColored(violet, "CYCLES TO EVENT "); ImGui::SameLine();
    ImGui::TextColored((*scsi_state->NEXT_EVENT_CYCLES == 0) ? gray : white, "%d", *scsi_state->NEXT_EVENT_CYCLES);

    ImGui::TextColored(violet, "NEXT SECTOR LOAD"); ImGui::SameLine();
    ImGui::TextColored((*scsi_state->NEXT_LOAD_CYCLES <= 0) ? gray : yellow, "%d", *scsi_state->LOAD_SECTOR);
    
    ImGui::TextColored(violet, "CYCLES TO LOAD  "); ImGui::SameLine();
    ImGui::TextColored((*scsi_state->NEXT_LOAD_CYCLES <= 0) ? gray : white, "%d", *scsi_state->NEXT_LOAD_CYCLES);

    ImGui::TextColored(violet, "SECTORS LEFT    "); ImGui::SameLine();
    ImGui::TextColored((*scsi_state->NEXT_LOAD_CYCLES <= 0) ? gray : white, "%d", *scsi_state->LOAD_SECTOR_COUNT);

    ImGui::TextColored(violet, "FADER   "); ImGui::SameLine();
    ImGui::TextColored(white, "$%02X ", *cdrom_state->FADER); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(*cdrom_state->FADER));

    ImGui::NewLine(); ImGui::TextColored(cyan, "MEDIA"); ImGui::Separator();

    ImGui::TextColored(violet, "MEDIA TYPE  "); ImGui::SameLine();
    ImGui::TextColored(white, "%s", cdrom_media->GetFileExtension());

    ImGui::TextColored(violet, "TRACKS      "); ImGui::SameLine();
    ImGui::TextColored(white, "%d", cdrom_media->GetTrackCount());

    ImGui::TextColored(violet, "LENGTH      "); ImGui::SameLine();
    ImGui::TextColored(white, "%02d:%02d:%02d", cdrom_media->GetCdRomLength().minutes, cdrom_media->GetCdRomLength().seconds, cdrom_media->GetCdRomLength().frames);

    ImGui::TextColored(violet, "SECTOR COUNT"); ImGui::SameLine();
    ImGui::TextColored(white, "%d", cdrom_media->GetSectorCount());

    u32 current_lba = cdrom_media->GetCurrentSector();
    s32 current_track = cdrom_media->FindTrackFromLBA(current_lba, true);

    ImGui::TextColored(violet, "HEAD LBA    "); ImGui::SameLine();
    ImGui::TextColored(white, "%06u", current_lba);

    ImGui::TextColored(violet, "HEAD TRACK  "); ImGui::SameLine();
    if (current_track >= 0)
        ImGui::TextColored(blue, "%02d", current_track + 1);
    else
        ImGui::TextColored(gray, "--");

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_cdrom_toc(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(105, 95), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(950, 498), ImGuiCond_FirstUseEver);
    ImGui::Begin("CD-ROM TOC", &config_debug.show_cdrom_toc);

    ImGui::PushFont(gui_default_font);

    CdRomMedia* cdrom_media = emu_get_core()->GetCDROMMedia();
    const std::vector<CdRomImage::Track>& tracks = cdrom_media->GetTracks();
    s32 current_track = cdrom_media->FindTrackFromLBA(cdrom_media->GetCurrentSector(), true);
    int audio_tracks = 0;
    int data_tracks = 0;

    for (size_t i = 0; i < tracks.size(); i++)
    {
        if (tracks[i].type == GG_CDROM_AUDIO_TRACK)
            audio_tracks++;
        else
            data_tracks++;
    }

    GG_CdRomMSF total_length = cdrom_media->GetCdRomLength();

    ImGui::TextColored(violet, "MEDIA  "); ImGui::SameLine();
    ImGui::TextColored(white, "%s (%s)", cdrom_media->GetFileName(), cdrom_media->GetFileExtension());

    ImGui::TextColored(violet, "TRACKS "); ImGui::SameLine();
    ImGui::TextColored(white, "%d", (int)tracks.size()); ImGui::SameLine();
    ImGui::TextColored(green, " (AUDIO %d)", audio_tracks); ImGui::SameLine();
    ImGui::TextColored(yellow, " (DATA %d)", data_tracks);

    ImGui::TextColored(violet, "LENGTH "); ImGui::SameLine();
    ImGui::TextColored(white, "%02u:%02u:%02u", total_length.minutes, total_length.seconds, total_length.frames);
    ImGui::TextColored(violet, "SECTORS "); ImGui::SameLine();
    ImGui::TextColored(white, "%u", cdrom_media->GetSectorCount());

    ImGui::Separator();

    ImGuiTableFlags flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit;

    if (ImGui::BeginTable("cdrom_toc", 12, flags))
    {
        ImGui::TableSetupScrollFreeze(1, 1);
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 28.0f);
        ImGui::TableSetupColumn("TYPE", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("BYTES", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("START MSF", ImGuiTableColumnFlags_WidthFixed, 76.0f);
        ImGui::TableSetupColumn("END MSF", ImGuiTableColumnFlags_WidthFixed, 76.0f);
        ImGui::TableSetupColumn("DURATION", ImGuiTableColumnFlags_WidthFixed, 76.0f);
        ImGui::TableSetupColumn("START LBA", ImGuiTableColumnFlags_WidthFixed, 72.0f);
        ImGui::TableSetupColumn("END LBA", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("SECTORS", ImGuiTableColumnFlags_WidthFixed, 68.0f);
        ImGui::TableSetupColumn("LEAD-IN LBA", ImGuiTableColumnFlags_WidthFixed, 84.0f);
        ImGui::TableSetupColumn("LEAD-IN", ImGuiTableColumnFlags_WidthFixed, 68.0f);
        ImGui::TableSetupColumn("FILE OFFSET", ImGuiTableColumnFlags_WidthFixed, 86.0f);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < tracks.size(); i++)
        {
            const CdRomImage::Track& track = tracks[i];
            char start_msf[16];
            char end_msf[16];
            char duration_msf[16];
            char lead_in_msf[16];
            u32 lead_in_sectors = track.has_lead_in ? track.start_lba - track.lead_in_lba : 0;

            format_cdrom_msf(track.start_lba, start_msf, sizeof(start_msf));
            format_cdrom_msf(track.end_lba, end_msf, sizeof(end_msf));
            format_cdrom_msf(track.sector_count, duration_msf, sizeof(duration_msf));
            format_cdrom_msf(lead_in_sectors, lead_in_msf, sizeof(lead_in_msf));

            ImGui::TableNextRow();

            if ((s32)i == current_track)
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(dark_blue));

            ImGui::TableNextColumn();
            ImGui::TextColored(((s32)i == current_track) ? orange : white, "%d", (int)i + 1);

            ImGui::TableNextColumn();
            ImGui::TextColored((track.type == GG_CDROM_AUDIO_TRACK) ? green : yellow,
                "%s", TrackTypeName(track.type));

            ImGui::TableNextColumn();
            ImGui::Text("%u", track.sector_size);

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(start_msf);

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(end_msf);

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(duration_msf);

            ImGui::TableNextColumn();
            ImGui::Text("%u", track.start_lba);

            ImGui::TableNextColumn();
            ImGui::Text("%u", track.end_lba);

            ImGui::TableNextColumn();
            ImGui::Text("%u", track.sector_count);

            ImGui::TableNextColumn();
            if (track.has_lead_in)
                ImGui::Text("%u", track.lead_in_lba);
            else
                ImGui::TextColored(gray, "--");

            ImGui::TableNextColumn();
            if (track.has_lead_in)
                ImGui::Text("%s", lead_in_msf);
            else
                ImGui::TextColored(gray, "--");

            ImGui::TableNextColumn();
            ImGui::Text("%u", track.file_offset);
        }

        ImGui::EndTable();
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_arcade_card(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(85, 90), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(220, 470), ImGuiCond_FirstUseEver);
    ImGui::Begin("Arcade Card", &config_debug.show_arcade_card);

    ImGui::PushFont(gui_default_font);

    GeargrafxCore* core = emu_get_core();
    Memory *memory = core->GetMemory();
    ArcadeCardMapper* arcade_card_mapper = memory->GetArcadeCardMapper();
    ArcadeCardMapper::ArcadeCard_State* arcade_card_state = arcade_card_mapper->GetState();

    ImGui::TextColored(violet, "REGISTER     "); ImGui::SameLine();
    ImGui::Text("$%08X", *arcade_card_state->REGISTER);

    ImGui::TextColored(violet, "SHIFT AMOUNT "); ImGui::SameLine();
    ImGui::Text("$%02X", *arcade_card_state->SHIFT_AMOUNT);

    ImGui::TextColored(violet, "ROTATE AMOUNT"); ImGui::SameLine();
    ImGui::Text("$%02X", *arcade_card_state->ROTATE_AMOUNT);

    for (int i = 0; i < 4; i++)
    {
        ImGui::NewLine(); ImGui::TextColored(cyan, "PORT %d", i); ImGui::Separator();
        ArcadeCardMapper::ArcadeCard_Port* port = &arcade_card_state->PORTS[i];

        ImGui::TextColored(violet, "BASE ADDRESS"); ImGui::SameLine();
        ImGui::Text("$%08X", port->base);
        ImGui::TextColored(violet, "OFFSET      "); ImGui::SameLine();
        ImGui::Text("$%04X", port->offset);
        ImGui::TextColored(violet, "INCREMENT   "); ImGui::SameLine();
        ImGui::Text("$%04X", port->increment);
        ImGui::TextColored(violet, "CONTROL     "); ImGui::SameLine();
        ImGui::TextColored(white, "$%02X ", port->control); ImGui::SameLine(0, 0);
        ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(port->control));

        ImGui::TextColored(violet, "ADD OFFSET    "); ImGui::SameLine();
        ImGui::TextColored(port->add_offset ? green : gray, "%s", port->add_offset ? "ON" : "OFF");
        ImGui::TextColored(violet, "AUTO INCREMENT"); ImGui::SameLine();
        ImGui::TextColored(port->auto_increment ? green : gray, "%s", port->auto_increment ? "ON" : "OFF");
        ImGui::TextColored(violet, "SIGNED OFFSET "); ImGui::SameLine();
        ImGui::TextColored(port->signed_offset ? green : gray, "%s", port->signed_offset ? "ON" : "OFF");
        ImGui::TextColored(violet, "INCREMENT BASE"); ImGui::SameLine();
        ImGui::TextColored(port->increment_base ? green : gray, "%s", port->increment_base ? "ON" : "OFF");

        const char* k_arcade_card_offset_trigger_names[] = { "NONE     ", "LOW BYTE ", "HIGH BYTE", "REG 0A   " };
        ImGui::TextColored(violet, "OFFSET TRIGGER"); ImGui::SameLine();
        ImGui::TextColored(port->offset_trigger == ArcadeCardMapper::OFFSET_TRIGGER_NONE ? gray : yellow, "%s", k_arcade_card_offset_trigger_names[port->offset_trigger]);
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

static void format_cdrom_msf(u32 lba, char* buffer, size_t size)
{
    GG_CdRomMSF msf;
    LbaToMsf(lba, &msf);
    snprintf(buffer, size, "%02u:%02u:%02u", msf.minutes, msf.seconds, msf.frames);
}
