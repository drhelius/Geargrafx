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

#include "huc6202.h"
#include "huc6270.h"

HuC6202::HuC6202(HuC6270* huc6270_1, HuC6270* huc6270_2, HuC6280* huc6280)
{
    m_huc6280 = huc6280;
    m_huc6270_1 = huc6270_1;
    m_huc6270_2 = huc6270_2;
    m_is_sgx = false;
    m_priority_1 = 0;
    m_priority_2 = 0;
    m_window_1 = 0;
    m_window_2 = 0;
    m_vdc2_selected = false;
    m_irq1_1 = false;
    m_irq1_2 = false;

    m_state.PRIORITY_1 = &m_priority_1;
    m_state.PRIORITY_2 = &m_priority_2;
    m_state.WINDOW_1 = &m_window_1;
    m_state.WINDOW_2 = &m_window_2;
    m_state.VDC2_SELECTED = &m_vdc2_selected;
    m_state.IRQ1_1 = &m_irq1_1;
    m_state.IRQ1_2 = &m_irq1_2;
    m_state.WINDOW_PRIORITY = m_window_priority;
}

HuC6202::~HuC6202()
{

}

void HuC6202::Init()
{
    Reset(false);
}

void HuC6202::Reset(bool is_sgx)
{
    m_is_sgx = is_sgx;
    m_window_1 = 0;
    m_window_2 = 0;
    m_vdc2_selected = false;
    m_irq1_1 = false;
    m_irq1_2 = false;
    WriteRegister(8, 0x11);
    WriteRegister(9, 0x11);
}

void HuC6202::CalculatePriorityMode(HuC6202_Window_Mode window_mode, u8 value)
{
    m_window_priority[window_mode].vdc_1_enabled = (value & 0x01) != 0;
    m_window_priority[window_mode].vdc_2_enabled = (value & 0x02) != 0;
    u8 priority_mode = (value >> 2) & 0x03;

    switch (priority_mode)
    {
        case 1:
            m_window_priority[window_mode].priority_mode = HuC6270_PRIORITY_SPRITES_2_ABOVE_BG_1;
            break;
        case 2:
            m_window_priority[window_mode].priority_mode = HuC6270_PRIORITY_SPRITES_1_BELOW_BG_2;
            break;
        default:
            m_window_priority[window_mode].priority_mode = HuC6270_PRIORITY_DEFAULT;
            break;
    }
}

void HuC6202::SaveState(std::ostream& stream)
{
    using namespace std;
    stream.write(reinterpret_cast<const char*> (&m_priority_1), sizeof(m_priority_1));
    stream.write(reinterpret_cast<const char*> (&m_priority_2), sizeof(m_priority_2));
    stream.write(reinterpret_cast<const char*> (&m_window_1), sizeof(m_window_1));
    stream.write(reinterpret_cast<const char*> (&m_window_2), sizeof(m_window_2));
    stream.write(reinterpret_cast<const char*> (&m_vdc2_selected), sizeof(m_vdc2_selected));
    stream.write(reinterpret_cast<const char*> (&m_irq1_1), sizeof(m_irq1_1));
    stream.write(reinterpret_cast<const char*> (&m_irq1_2), sizeof(m_irq1_2));
    for (int i = 0; i < 4; i++)
    {
        stream.write(reinterpret_cast<const char*> (&m_window_priority[i].vdc_1_enabled), sizeof(m_window_priority[i].vdc_1_enabled));
        stream.write(reinterpret_cast<const char*> (&m_window_priority[i].vdc_2_enabled), sizeof(m_window_priority[i].vdc_2_enabled));
        stream.write(reinterpret_cast<const char*> (&m_window_priority[i].priority_mode), sizeof(m_window_priority[i].priority_mode));
    }
}

void HuC6202::LoadState(std::istream& stream)
{
    using namespace std;
    stream.read(reinterpret_cast<char*> (&m_priority_1), sizeof(m_priority_1));
    stream.read(reinterpret_cast<char*> (&m_priority_2), sizeof(m_priority_2));
    stream.read(reinterpret_cast<char*> (&m_window_1), sizeof(m_window_1));
    stream.read(reinterpret_cast<char*> (&m_window_2), sizeof(m_window_2));
    stream.read(reinterpret_cast<char*> (&m_vdc2_selected), sizeof(m_vdc2_selected));
    stream.read(reinterpret_cast<char*> (&m_irq1_1), sizeof(m_irq1_1));
    stream.read(reinterpret_cast<char*> (&m_irq1_2), sizeof(m_irq1_2));
    for (int i = 0; i < 4; i++)
    {
        stream.read(reinterpret_cast<char*> (&m_window_priority[i].vdc_1_enabled), sizeof(m_window_priority[i].vdc_1_enabled));
        stream.read(reinterpret_cast<char*> (&m_window_priority[i].vdc_2_enabled), sizeof(m_window_priority[i].vdc_2_enabled));
        stream.read(reinterpret_cast<char*> (&m_window_priority[i].priority_mode), sizeof(m_window_priority[i].priority_mode));
    }
}
