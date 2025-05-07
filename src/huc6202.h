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

class HuC6202
{
public:
    HuC6202(HuC6270* huc6270_1, HuC6270* huc6270_2);
    ~HuC6202();
    void Init();
    void Reset(bool is_sgx);
    u16 Clock();
    void SetHSyncHigh();
    void SetVSyncLow();
    u8 ReadRegister(u16 address);
    void WriteRegister(u16 address, u8 value);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    template<bool is_supergrafx>
    u16 ClockTemplate();
    template<bool is_supergrafx>
    void SetHSyncHighTemplate();
    template<bool is_supergrafx>
    void SetVSyncLowTemplate();

private:
    HuC6270* m_huc6270_1;
    HuC6270* m_huc6270_2;
    bool m_is_sgx;
    u16 (HuC6202::*m_clock_ptr)();
    void (HuC6202::*m_hsync_high_ptr)();
    void (HuC6202::*m_vsync_low_ptr)();
};

#endif /* HUC6202_H */
