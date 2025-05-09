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

#ifndef HUC6202_INLINE_H
#define HUC6202_INLINE_H

#include "huc6202.h"
#include "huc6270.h"
#include "huc6280.h"

INLINE u16 HuC6202::Clock(void)
{
    return m_huc6270_1->Clock();
}

INLINE void HuC6202::ClockSGX(u16* pixel_1, u16* pixel_2)
{
    // Although the HuC6202 combines the two VDCs output
    // the combination is performed in the HuC6260
    // for performance reasons
    *pixel_1 = m_huc6270_1->Clock();
    *pixel_2 = m_huc6270_2->Clock();
}

INLINE void HuC6202::SetHSyncHigh()
{
    m_huc6270_1->SetHSyncHigh();
    if (m_is_sgx)
        m_huc6270_2->SetHSyncHigh();
}

INLINE void HuC6202::SetVSyncLow()
{
    m_huc6270_1->SetVSyncLow();
    if (m_is_sgx)
        m_huc6270_2->SetVSyncLow();
}

INLINE u8 HuC6202::ReadRegister(u16 address)
{
    if (m_is_sgx)
    {
        switch(address & 0x1F)
        {
            case 0: case 1: case 2: case 3:
            case 4: case 5: case 6: case 7:
                return m_huc6270_1->ReadRegister(address);
            case 0x08:
                return m_priority_1;
            case 0x09:
                return m_priority_2;
            case 0x0A:
                return (m_window_1 & 0xFF);
            case 0x0B:
                return (m_window_1 >> 8);
            case 0x0C:
                return (m_window_2 & 0xFF);
            case 0x0D:
                return (m_window_2 >> 8);
            case 0x0E: case 0x0F:
                return 0;
            case 0x10: case 0x11: case 0x12: case 0x13:
            case 0x14: case 0x15: case 0x16: case 0x17:
                return m_huc6270_2->ReadRegister(address);
            default:
                Debug("Invalid HuC6202 register read at %04X", address);
                return 0xFF;
        }
    }
    else
    {
        return m_huc6270_1->ReadRegister(address);
    }
}

INLINE void HuC6202::WriteRegister(u16 address, u8 value)
{
    if (m_is_sgx)
    {
        switch(address & 0x1F) {
            case 0: case 1: case 2: case 3:
            case 4: case 5: case 6: case 7:
                m_huc6270_1->WriteRegister(address, value);
                break;
            case 0x08:
                m_priority_1 = value;
                CalculatePriorityMode(HuC6270_WINDOW_BOTH, value & 0x0F);
                CalculatePriorityMode(HuC6270_WINDOW_2, (value & 0xF0) >> 4);
                break;
            case 0x09:
                m_priority_2 = value;
                CalculatePriorityMode(HuC6270_WINDOW_1, value & 0x0F);
                CalculatePriorityMode(HuC6270_WINDOW_NONE, (value & 0xF0) >> 4);
                break;
            case 0x0A:
                m_window_1 = (m_window_1 & 0x300) | value;
                break;
            case 0x0B:
                m_window_1 = (m_window_1 & 0xFF) | ((value & 0x03) << 8);
                break;
            case 0x0C:
                m_window_2 = (m_window_2 & 0x300) | value;
                break;
            case 0x0D:
                m_window_2 = (m_window_2 & 0xFF) | ((value & 0x03) << 8);
                break;
            case 0x0E:
                m_vdc2_selected = (value & 0x01);
                break;
            case 0x10: case 0x11: case 0x12: case 0x13:
            case 0x14: case 0x15: case 0x16: case 0x17:
                m_huc6270_2->WriteRegister(address, value);
                break;
            default:
                Debug("HuC6202: Invalid write at %04X, value=%02X", address, value);
                break;
        }
    }
    else
    {
        m_huc6270_1->WriteRegister(address, value);
    }
}

INLINE void HuC6202::WriteFromCPU(u16 address, u8 value)
{
    if(m_vdc2_selected)
        m_huc6270_2->WriteRegister(address, value);
    else
        m_huc6270_1->WriteRegister(address, value);
}

INLINE void HuC6202::AssertIRQ1(HuC6270* vdc, bool assert)
{
    if (vdc == m_huc6270_1)
        m_irq1_1 = assert;
    else
        m_irq1_2 = assert;

    m_huc6280->AssertIRQ1(m_irq1_1 || m_irq1_2);
}

INLINE u16 HuC6202::GetWindow1Width()
{
    return m_window_1;
}

INLINE u16 HuC6202::GetWindow2Width()
{
    return m_window_2;
}

INLINE HuC6202::HuC6202_Window_Priority* HuC6202::GetWindowPriorities()
{
    return m_window_priority;
}

INLINE HuC6202::HuC6202_State* HuC6202::GetState()
{
    return &m_state;
}

#endif /* HUC6202_INLINE_H */
