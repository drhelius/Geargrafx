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

class Cartridge
{
public:
    Cartridge();
    ~Cartridge();
    void Init();
    void Reset();
    u32 GetCRC();
    bool IsReady();
    int GetROMSize();
    int GetROMBankCount();
    const char* GetFilePath();
    const char* GetFileName();
    u8* GetROM();
    bool LoadFromFile(const char* path);
    bool LoadFromBuffer(const u8* buffer, int size);

private:
    bool LoadFromZipFile(const u8* buffer, int size);

private:
    u8* m_pROM;
    int m_iROMSize;
    bool m_bReady;
    char m_szFilePath[512];
    char m_szFileName[512];
    int m_iROMBankCount;
    u32 m_iCRC;
};