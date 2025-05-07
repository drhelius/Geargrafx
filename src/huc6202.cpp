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
    m_priority_1 = 0x11;
    m_priority_2 = 0x11;
    m_window_1 = 0;
    m_window_2 = 0;
    m_vdc2_selected = false;
    m_irq1_1 = false;
    m_irq1_2 = false;
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
}
