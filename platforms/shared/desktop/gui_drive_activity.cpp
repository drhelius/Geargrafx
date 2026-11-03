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

#define GUI_DRIVE_ACTIVITY_IMPORT
#include "gui_drive_activity.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <float.h>
#include "imgui_internal.h"
#include "fonts/IconsMaterialDesign.h"
#include "gui.h"
#include "gui_colors.h"
#include "config.h"
#include "emu.h"
#include "geargrafx.h"

#define GUI_DRIVE_ACTIVITY_SLOTS 2
#define GUI_DRIVE_ACTIVITY_FFWD_SLOT 1

struct Drive_Slot
{
    u32 access_count;
    u32 write_count;
    Uint64 lit_until;
    Uint64 write_until;
    Uint64 hold_until;
    float offset;
    bool placed;
};

static const Uint64 k_drive_activity_pulse = 50;
static const Uint64 k_drive_activity_hold = 300;
static const float k_drive_activity_slide = 100.0f;
static const float k_drive_activity_margin = 8.0f;
static const float k_drive_activity_spacing = 4.0f;
static const float k_drive_activity_padding = 3.0f;
static const float k_drive_activity_rounding = 4.0f;
static const float k_drive_activity_icon_size = 16.0f;
static const float k_drive_activity_label_size = 11.0f;
static const float k_drive_activity_label_spacing = 1.0f;
static const float k_drive_activity_background = 0.85f;
static const char* const k_drive_activity_icons[GUI_DRIVE_ACTIVITY_SLOTS] = { ICON_MD_ALBUM, ICON_MD_FAST_FORWARD };
static const char* const k_drive_activity_labels[GUI_DRIVE_ACTIVITY_SLOTS] = { NULL, NULL };

static Drive_Slot slots[GUI_DRIVE_ACTIVITY_SLOTS];
static int sample_frame = -1;

static void sample(Uint64 now, bool resync);
static void update_slot(Drive_Slot* slot, u32 access_count, u32 write_count, bool busy, Uint64 now, bool resync);
static float slot_width(ImFont* font, int index);
static void draw_slot(ImDrawList* draw_list, ImFont* font, int index, const ImVec2& pos, const ImVec2& size, bool write);

void gui_drive_activity_render_output(const ImVec2& min, const ImVec2& max)
{
    Uint64 now = SDL_GetTicks();
    int frame = ImGui::GetFrameCount();
    sample(now, frame != sample_frame + 1);
    sample_frame = frame;

    ImFont* font = gui_roboto_font;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    float height = k_drive_activity_icon_size + (k_drive_activity_padding * 2.0f);
    float slide = ImGui::GetIO().DeltaTime * 1000.0f / k_drive_activity_slide;
    float offset = 0.0f;

    draw_list->PushClipRect(min, max, true);

    int first = config_emulator.show_cdrom_activity ? 0 : GUI_DRIVE_ACTIVITY_FFWD_SLOT;
    int last = config_emulator.ffwd_loads_notification ? GUI_DRIVE_ACTIVITY_SLOTS : GUI_DRIVE_ACTIVITY_FFWD_SLOT;

    for (int i = first; i < last; i++)
    {
        Drive_Slot* slot = &slots[i];

        if (now >= slot->hold_until)
        {
            slot->placed = false;
            continue;
        }

        float width = slot_width(font, i);

        if (!slot->placed || slot->offset < offset)
            slot->offset = offset;
        else
            slot->offset = fmaxf(offset, slot->offset - (slide * (width + k_drive_activity_spacing)));

        slot->placed = true;

        if (now < slot->lit_until)
        {
            float x = max.x - k_drive_activity_margin - slot->offset - width;
            ImVec2 pos(roundf(x), roundf(min.y + k_drive_activity_margin));
            draw_slot(draw_list, font, i, pos, ImVec2(width, height), now < slot->write_until);
        }

        offset = slot->offset + width + k_drive_activity_spacing;
    }

    draw_list->PopClipRect();
}

static void sample(Uint64 now, bool resync)
{
    GeargrafxCore* core = emu_get_core();
    ScsiController* scsi_controller = core->GetScsiController();
    bool cdrom_ready = core->GetCDROMMedia()->IsReady();

    update_slot(&slots[0], scsi_controller->GetAccessCount(), 0, cdrom_ready && scsi_controller->IsBusy(), now,
        resync || !cdrom_ready);
    update_slot(&slots[GUI_DRIVE_ACTIVITY_FFWD_SLOT], 0, 0, emu_is_load_ffwd(), now, resync);
}

static void update_slot(Drive_Slot* slot, u32 access_count, u32 write_count, bool busy, Uint64 now, bool resync)
{
    bool accessed = busy || (access_count != slot->access_count);
    bool written = write_count != slot->write_count;
    slot->access_count = access_count;
    slot->write_count = write_count;

    if (resync)
    {
        slot->lit_until = 0;
        slot->write_until = 0;
        slot->hold_until = 0;
        slot->placed = false;
        return;
    }

    if (accessed)
    {
        slot->lit_until = now + k_drive_activity_pulse;
        slot->hold_until = now + k_drive_activity_hold;
    }

    if (written)
        slot->write_until = now + k_drive_activity_pulse;
}

static float slot_width(ImFont* font, int index)
{
    float width = k_drive_activity_icon_size + (k_drive_activity_padding * 2.0f);
    const char* label = k_drive_activity_labels[index];

    if (label != NULL)
    {
        float label_width = font->CalcTextSizeA(k_drive_activity_label_size, FLT_MAX, 0.0f, label).x;
        width += k_drive_activity_label_spacing + label_width;
    }

    return roundf(width);
}

static void draw_slot(ImDrawList* draw_list, ImFont* font, int index, const ImVec2& pos, const ImVec2& size, bool write)
{
    const char* icon = k_drive_activity_icons[index];
    const char* label = k_drive_activity_labels[index];
    ImVec4 background = ImGui::GetStyle().Colors[ImGuiCol_PopupBg];
    background.w = k_drive_activity_background;
    bool ffwd = index == GUI_DRIVE_ACTIVITY_FFWD_SLOT;
    ImU32 color = ImGui::ColorConvertFloat4ToU32(ffwd ? violet : (write ? amber : green));

    draw_list->AddRectFilled(pos, pos + size, ImGui::ColorConvertFloat4ToU32(background), k_drive_activity_rounding);

    unsigned int codepoint = 0;
    ImTextCharFromUtf8(&codepoint, icon, NULL);
    ImFontGlyph* glyph = font->GetFontBaked(k_drive_activity_icon_size)->FindGlyph((ImWchar)codepoint);
    float icon_x = pos.x + k_drive_activity_padding - glyph->X0 +
        ((k_drive_activity_icon_size - (glyph->X1 - glyph->X0)) * 0.5f);
    float icon_y = pos.y - glyph->Y0 + ((size.y - (glyph->Y1 - glyph->Y0)) * 0.5f);

    draw_list->AddText(font, k_drive_activity_icon_size, ImVec2(roundf(icon_x), roundf(icon_y)), color, icon);

    if (label == NULL)
        return;

    ImFontGlyph* digit = font->GetFontBaked(k_drive_activity_label_size)->FindGlyph((ImWchar)label[0]);
    float label_x = pos.x + k_drive_activity_padding + k_drive_activity_icon_size + k_drive_activity_label_spacing;
    float label_y = pos.y + size.y - k_drive_activity_padding - digit->Y1;

    draw_list->AddText(font, k_drive_activity_label_size, ImVec2(roundf(label_x), roundf(label_y)), color, label);
}
