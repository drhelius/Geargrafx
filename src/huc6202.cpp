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

HuC6202::HuC6202(HuC6270* huc6270_1, HuC6270* huc6270_2)
{
    m_huc6270_1 = huc6270_1;
    m_huc6270_2 = huc6270_2;
    m_is_sgx = false;
}

HuC6202::~HuC6202()
{

}

void HuC6202::Init()
{

}

void HuC6202::Reset(bool is_sgx)
{
    m_is_sgx = is_sgx;

    if(m_is_sgx)
    {
        m_clock_ptr = &HuC6202::ClockTemplate<true>;
        m_hsync_high_ptr = &HuC6202::SetHSyncHighTemplate<true>;
        m_vsync_low_ptr = &HuC6202::SetVSyncLowTemplate<true>;
    }
    else
    {
        m_clock_ptr = &HuC6202::ClockTemplate<false>;
        m_hsync_high_ptr = &HuC6202::SetHSyncHighTemplate<false>;
        m_vsync_low_ptr = &HuC6202::SetVSyncLowTemplate<false>;
    }
}

u16 HuC6202::Clock()
{
    return (this->*m_clock_ptr)();
}

void HuC6202::SetHSyncHigh()
{
    (this->*m_hsync_high_ptr)();
}

void HuC6202::SetVSyncLow()
{
    (this->*m_vsync_low_ptr)();
}

template<bool is_sgx>
u16 HuC6202::ClockTemplate()
{
    if (is_sgx)
    {
        u16 pixel1 = m_huc6270_1->Clock();
        u16 pixel2 = m_huc6270_2->Clock();
    }
    else
    {
        return m_huc6270_1->Clock();
    }
}

template u16 HuC6202::ClockTemplate<true>();
template u16 HuC6202::ClockTemplate<false>();

template<bool is_sgx>
void HuC6202::SetHSyncHighTemplate()
{
    if (is_sgx)
    {
        m_huc6270_1->SetHSyncHigh();
        m_huc6270_2->SetHSyncHigh();
    }
    else
    {
        m_huc6270_1->SetHSyncHigh();
    }
}

template void HuC6202::SetHSyncHighTemplate<true>();
template void HuC6202::SetHSyncHighTemplate<false>();

template<bool is_sgx>
void HuC6202::SetVSyncLowTemplate()
{
    if (is_sgx)
    {
        m_huc6270_1->SetVSyncLow();
        m_huc6270_2->SetVSyncLow();
    }
    else
    {
        m_huc6270_1->SetVSyncLow();
    }
}

template void HuC6202::SetVSyncLowTemplate<true>();
template void HuC6202::SetVSyncLowTemplate<false>();

u8 HuC6202::ReadRegister(u16 address)
{

    return 0;
}

void HuC6202::WriteRegister(u16 address, u8 value)
{

}

void HuC6202::SaveState(std::ostream& stream)
{

}

void HuC6202::LoadState(std::istream& stream)
{

}
