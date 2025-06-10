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
#include "gui_debug_huc6260.h"

#include "imgui/imgui.h"
#include "geargrafx.h"
#include "gui_debug_constants.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

void gui_debug_window_cdrom(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(75, 228), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(270, 188), ImGuiCond_FirstUseEver);
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
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", *cdrom_state->ENABLED_IRQS, BYTE_TO_BINARY(*cdrom_state->ENABLED_IRQS));

    ImGui::TextColored((*cdrom_state->ENABLED_IRQS & CDROM_IRQ_ADPCM) ? green : gray, "%s", (*cdrom_state->ENABLED_IRQS & CDROM_IRQ_ADPCM) ? "ADPCM" : "ADPCM"); ImGui::SameLine();

    ImGui::TextColored((*cdrom_state->ENABLED_IRQS & CDROM_IRQ_STOP) ? green : gray, "%s", (*cdrom_state->ENABLED_IRQS & CDROM_IRQ_STOP) ? "STOP" : "STOP"); ImGui::SameLine();

    ImGui::TextColored((*cdrom_state->ENABLED_IRQS & CDROM_IRQ_STATUS_AND_MSG_IN) ? green : gray, "%s", (*cdrom_state->ENABLED_IRQS & CDROM_IRQ_STATUS_AND_MSG_IN) ? "STATUS" : "STATUS"); ImGui::SameLine();

    ImGui::TextColored((*cdrom_state->ENABLED_IRQS & CDROM_IRQ_DATA_IN) ? green : gray, "%s", (*cdrom_state->ENABLED_IRQS & CDROM_IRQ_DATA_IN) ? "DATA" : "DATA");

    ImGui::NewLine(); ImGui::TextColored(cyan, "ACTIVE IRQS"); ImGui::Separator();

    ImGui::TextColored(violet, "ACTIVE "); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", *cdrom_state->ACTIVE_IRQS, BYTE_TO_BINARY(*cdrom_state->ACTIVE_IRQS));

    ImGui::TextColored((*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_ADPCM) ? green : gray, "%s", (*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_ADPCM) ? "ADPCM" : "ADPCM"); ImGui::SameLine();

    ImGui::TextColored((*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_STOP) ? green : gray, "%s", (*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_STOP) ? "STOP" : "STOP"); ImGui::SameLine();

    ImGui::TextColored((*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_STATUS_AND_MSG_IN) ? green : gray, "%s", (*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_STATUS_AND_MSG_IN) ? "STATUS" : "STATUS"); ImGui::SameLine();

    ImGui::TextColored((*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_DATA_IN) ? green : gray, "%s", (*cdrom_state->ACTIVE_IRQS & CDROM_IRQ_DATA_IN) ? "DATA" : "DATA");

    ImGui::NewLine(); ImGui::TextColored(cyan, "SCSI"); ImGui::Separator();

    ImGui::TextColored(violet, "PHASE   "); ImGui::SameLine();
    ImGui::TextColored(blue, "%s", k_scsi_phase_names[*scsi_state->PHASE]);

    ImGui::TextColored(violet, "DATA BUS"); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", *scsi_state->DB, BYTE_TO_BINARY(*scsi_state->DB));

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
    ImGui::TextColored((*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_ACK) ? green : gray, "%s", (*scsi_state->SIGNALS & ScsiController::SCSI_SIGNAL_ACK & CDROM_IRQ_ADPCM) ? "ON " : "OFF"); ImGui::SameLine();

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

    ImGui::NewLine(); ImGui::TextColored(cyan, "MEDIA"); ImGui::Separator();

    ImGui::TextColored(violet, "MEDIA TYPE  "); ImGui::SameLine();
    ImGui::TextColored(white, "%s", cdrom_media->GetFileExtension());

    ImGui::TextColored(violet, "TRACKS      "); ImGui::SameLine();
    ImGui::TextColored(white, "%d", cdrom_media->GetTrackCount());

    ImGui::TextColored(violet, "LENGTH      "); ImGui::SameLine();
    ImGui::TextColored(white, "%02d:%02d:%02d", cdrom_media->GetCdRomLength().minutes, cdrom_media->GetCdRomLength().seconds, cdrom_media->GetCdRomLength().frames);

    ImGui::TextColored(violet, "SECTOR COUNT"); ImGui::SameLine();
    ImGui::TextColored(white, "%d", cdrom_media->GetSectorCount());

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}
