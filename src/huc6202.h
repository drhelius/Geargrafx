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

#ifndef HUC6202_H
#define HUC6202_H

#include <iostream>
#include <fstream>
#include "common.h"

class HuC6270;
class HuC6280;

class HuC6202
{
public:
    enum HuC6270_Window_Mode
    {
        HuC6270_WINDOW_NONE = 0,
        HuC6270_WINDOW_1,
        HuC6270_WINDOW_2,
        HuC6270_WINDOW_BOTH
    };

    enum HuC6270_Priority_Mode
    {
        HuC6270_PRIORITY_DEFAULT = 0,
        HuC6270_PRIORITY_SPRITES_2_ABOVE_BG_1,
        HuC6270_PRIORITY_SPRITES_1_BELOW_BG_2,
    };

    struct HuC6270_Window_Priority
    {
        bool vdc_1_enabled;
        bool vdc_2_enabled;
        HuC6270_Priority_Mode priority_mode;
    };

    struct HuC6202_State
    {
        u8* PRIORITY_1;
        u8* PRIORITY_2;
        u16* WINDOW_1;
        u16* WINDOW_2;
        bool* VDC2_SELECTED;
        bool* IRQ1_1;
        bool* IRQ1_2;
        HuC6270_Window_Priority* WINDOW_PRIORITY;
    };

public:
    HuC6202(HuC6270* huc6270_1, HuC6270* huc6270_2, HuC6280* huc6280);
    ~HuC6202();
    void Init();
    void Reset(bool is_sgx);
    u16 Clock();
    void ClockSGX(u16* pixel_1, u16* pixel_2);
    void SetHSyncHigh();
    void SetVSyncLow();
    u8 ReadRegister(u16 address);
    void WriteRegister(u16 address, u8 value);
    void WriteFromCPU(u16 address, u8 value);
    void AssertIRQ1(HuC6270* vdc, bool assert);
    u16 GetWindow1Width();
    u16 GetWindow2Width();
    HuC6270_Window_Priority* GetWindowPriorities();
    HuC6202_State* GetState();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void CalculatePriorityMode(HuC6270_Window_Mode window_mode, u8 value);

private:
    HuC6280* m_huc6280;
    HuC6270* m_huc6270_1;
    HuC6270* m_huc6270_2;
    HuC6202_State m_state;
    bool m_is_sgx;
    u8 m_priority_1;
    u8 m_priority_2;
    u16 m_window_1;
    u16 m_window_2;
    bool m_vdc2_selected;
    bool m_irq1_1;
    bool m_irq1_2;
    HuC6270_Window_Priority m_window_priority[4];
};

#include "huc6202_inline.h"

#endif /* HUC6202_H */
