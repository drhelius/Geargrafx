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

#ifndef MB12_INLINE_H
#define MB12_INLINE_H

#include "mb128.h"

INLINE bool MB128::IsConnected() const
{
    return m_connected;
}

INLINE bool MB128::IsActive() const
{
    return m_active;
}

INLINE u8* MB128::GetRAM()
{
    return m_ram;
}

INLINE const u8* MB128::GetRAM() const
{
    return m_ram;
}

#endif /* MB12_INLINE_H */
