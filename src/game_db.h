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

#ifndef GAME_DB_H
#define	GAME_DB_H

#include "common.h"

#define GG_GAMEDB_SGX_REQUIRED          0x0001
#define GG_GAMEDB_SGX_OPTIONAL          0x0002
#define GG_GAMEDB_SF2_MAPPER            0x0004
#define GG_GAMEDB_CARD_RAM_8000         0x0008
#define GG_GAMEDB_AVENUE_PAD_3_SELECT   0x0010
#define GG_GAMEDB_AVENUE_PAD_3_RUN      0x0020
#define GG_GAMEDB_CDROM                 0x0040
#define GG_GAMEDB_BIOS_SYSCARD          0x0080
#define GG_GAMEDB_BIOS_GAME_EXPRESS     0x0100
#define GG_GAMEDB_GAME_EXPRESS_GAME     0x0200

struct GG_DB_Entry
{
    u32 crc;
    const char* title;
    u16 flags;
};

const GG_DB_Entry k_game_database[] =
{
    // BIOS
    { 0x3F9F95A4, "CD-ROM System Card [1.0] (J)", GG_GAMEDB_BIOS_SYSCARD },
    { 0x52520BC6, "CD-ROM System Card [2.0] (J)", GG_GAMEDB_BIOS_SYSCARD },
    { 0x283B74E0, "CD-ROM System Card [2.1] (J)", GG_GAMEDB_BIOS_SYSCARD },
    { 0x6D9A73EF, "Super CD-ROM System [3.0] (J)", GG_GAMEDB_BIOS_SYSCARD },
    { 0xFF2A5EC3, "TurboGrafx CD System Card [2.0] (USA)", GG_GAMEDB_BIOS_SYSCARD },
    { 0x2B5B75FE, "TurboGrafx CD Super System [3.0] (USA)", GG_GAMEDB_BIOS_SYSCARD },
    { 0x51A12D90, "Game Express Card [Blue Version] (J)", GG_GAMEDB_BIOS_GAME_EXPRESS },
    { 0x16AAF05A, "Game Express Card [Green Version] (J)", GG_GAMEDB_BIOS_GAME_EXPRESS },

    // SGX
    { 0x8C4588E2, "1941 - Counter Attack (J) (SGX)", GG_GAMEDB_SGX_REQUIRED },
    { 0x4C2126B0, "Aldynes (J) (SGX)", GG_GAMEDB_SGX_REQUIRED },
    { 0x3B13AF61, "Battle Ace (J) (SGX)", GG_GAMEDB_SGX_REQUIRED },
    { 0xB486A8ED, "Daimakaimura (J) (SGX)", GG_GAMEDB_SGX_REQUIRED },
    { 0x1F041166, "Madouou Granzort (J) (SGX)", GG_GAMEDB_SGX_REQUIRED },

    // OPTIONAL SGX
    { 0xBEBFE042, "Darius Plus (J) (SGX)", GG_GAMEDB_SGX_OPTIONAL },

    // SF2 MAPPER
    { 0xD15CB6BB, "Street Fighter II' - Champion Edition (J)", GG_GAMEDB_SF2_MAPPER },

    // CARD RAM SIZE = 0x8000
    { 0x083C956A, "Populous (J)", GG_GAMEDB_CARD_RAM_8000 },
    { 0x0A9ADE99, "Populous (J) [Alt]", GG_GAMEDB_CARD_RAM_8000 },

    // GAME EXPRESS BIOS GAMES
    { 0x5DAA84AD, "AV Tanjou (Japan) (Unl)", GG_GAMEDB_GAME_EXPRESS_GAME },
    { 0x8498CB7C, "AV Tanjou (Japan) (Unl) [Alt 1]", GG_GAMEDB_GAME_EXPRESS_GAME },
    { 0x65457B0B, "AV Tanjou (Japan) (Unl) [Alt 2]", GG_GAMEDB_GAME_EXPRESS_GAME },
    { 0xFC78C1BF, "CD Bishoujo Pachinko - Kyuuma Yon Shimai (Japan) (Unl)", GG_GAMEDB_GAME_EXPRESS_GAME },
    { 0x5E524EFC, "CD Mahjong - Bishoujo Chuushinha (Japan) (Unl)", GG_GAMEDB_GAME_EXPRESS_GAME },
    { 0x5FA37D41, "CD Hanafuda - Bishoujo Fan Club (Japan) (Unl)", GG_GAMEDB_GAME_EXPRESS_GAME },
    { 0x8F45C7F9, "CD Pachi-Slot - Bishoujo Gambler (Japan) (Unl)", GG_GAMEDB_GAME_EXPRESS_GAME },
    { 0xD62E6983, "Hi-Leg Fantasy (Japan) (Unl)", GG_GAMEDB_GAME_EXPRESS_GAME },

    // AVENUE PAD 3
    { 0x933D5BCC, "Air Zonk (USA)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "Air Zonk (USA) [Wii U]", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0xCA72A828, "After Burner II (J)", GG_GAMEDB_AVENUE_PAD_3_RUN },
    { 0x0, "Akumajou Dracula X - Chi no Rondo (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT | GG_GAMEDB_CDROM },
    { 0xCACC06FB, "Ankoku Densetsu (J)", GG_GAMEDB_AVENUE_PAD_3_RUN },
    { 0x0, "Atlantean [Unl]", GG_GAMEDB_AVENUE_PAD_3_RUN },
    { 0x4A3DF3CA, "Barunba (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0xE70B01AF, "Battle Royale (USA)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0xB4A1B0F6, "Blazing Lazers (USA)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "Blazing Lazers (USA) [Wii U]", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x37BAF6BC, "Bloody Wolf (USA)", GG_GAMEDB_AVENUE_PAD_3_RUN },
    { 0xA98D276A, "Cyber Core (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x4CFB6E3E, "Cyber Core (USA)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x85101C20, "Download (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x4E0DE488, "Download (J) [Alt]", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x560D2305, "Final Match Tennis (J)", GG_GAMEDB_AVENUE_PAD_3_RUN },
    { 0xAF2DD2AF, "Final Soldier (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x02A578C5, "Final Soldier - Special Version (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "Final Soldier (J) [Wii U]", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "Forgotten Worlds (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT | GG_GAMEDB_CDROM },
    { 0x0, "Forgotten Worlds (USA)", GG_GAMEDB_AVENUE_PAD_3_SELECT | GG_GAMEDB_CDROM },
    { 0x0, "Gate of Thunder (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT | GG_GAMEDB_CDROM },
    { 0x0, "Golden Axe (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT | GG_GAMEDB_CDROM },
    { 0x0, "Golden Axe (J) [Alt]", GG_GAMEDB_AVENUE_PAD_3_SELECT | GG_GAMEDB_CDROM },
    { 0x0, "Gradius II - Gofer no Yabou (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT | GG_GAMEDB_CDROM },
    { 0xA17D4D7E, "Gunhed (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x57F183AE, "Gunhed - Special Version (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "Gunhed (J) [Wii U]", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "John Madden Duo CD Fooball (USA)", GG_GAMEDB_AVENUE_PAD_3_RUN | GG_GAMEDB_CDROM },
    { 0x0, "Kunio Soccer", GG_GAMEDB_AVENUE_PAD_3_SELECT | GG_GAMEDB_CDROM },
    { 0x220EBF91, "Legendary Axe 2 (USA)", GG_GAMEDB_AVENUE_PAD_3_RUN },
    { 0x0, "Martial Champion (J)", GG_GAMEDB_AVENUE_PAD_3_RUN | GG_GAMEDB_CDROM },
    { 0x0, "Metamor Jupiter (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT | GG_GAMEDB_CDROM },
    { 0xB01F70C2, "Narazumono Sento Butai - Bloody Wolf (J)", GG_GAMEDB_AVENUE_PAD_3_RUN },
    { 0x0, "Nekketsu Koukou Dodgeball-bu CD - Soccer-hen (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT | GG_GAMEDB_CDROM },
    { 0xDE8AF1C1, "Ninja Spirit (USA)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "Ninja Spirit (USA) [Wii U]", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "Ookami-teki Monshou - Crest of Wolf (J)", GG_GAMEDB_AVENUE_PAD_3_RUN | GG_GAMEDB_CDROM },
    { 0x740491C2, "PC Denjin - Punkic Cyborgs (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x8FB4F228, "PC Denjin - Punkic Cyborgs (J) [Wii U]", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "Riot Zone (USA)", GG_GAMEDB_AVENUE_PAD_3_RUN | GG_GAMEDB_CDROM },
    { 0x0590A156, "Saigou no Nindou - Ninja Spirit (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "Saigou no Nindou - Ninja Spirit (J) [Wii U]", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0xBC655CF3, "Shinobi (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x616EA179, "Silent Debuggers (J)", GG_GAMEDB_AVENUE_PAD_3_RUN },
    { 0xFA7E5D66, "Silent Debuggers (USA)", GG_GAMEDB_AVENUE_PAD_3_RUN },
    { 0x8420B12B, "Soldier Blade (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x4BB68B13, "Soldier Blade (USA)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0xF39F38ED, "Soldier Blade (J) [Special - Caravan Stage] ", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "Soldier Blade (J) [Wii U]", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "Soldier Blade (USA) [Wii U]", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x9DB3C8C7, "Special Criminal Investigation (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "Spriggan Mark 2 - Re Terraform Project (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT | GG_GAMEDB_CDROM },
    { 0x5D0E3105, "Super Star Soldier (J)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0xDB29486F, "Super Star Soldier (USA)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "Super Star Soldier (J) [Wii U]", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "Super Star Soldier (USA) [Wii U]", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0xEB045EDF, "Turrican (USA)", GG_GAMEDB_AVENUE_PAD_3_SELECT },
    { 0x0, "Valis III (J)", GG_GAMEDB_AVENUE_PAD_3_RUN | GG_GAMEDB_CDROM },
    { 0x0, "Valis III (J) [Alt]", GG_GAMEDB_AVENUE_PAD_3_RUN | GG_GAMEDB_CDROM },
    { 0x0, "Valis III (USA)", GG_GAMEDB_AVENUE_PAD_3_RUN | GG_GAMEDB_CDROM },
    { 0x0, "World Heroes 2 (J)", GG_GAMEDB_AVENUE_PAD_3_RUN | GG_GAMEDB_CDROM },
    { 0x0, "World Heroes 2 (J) [Demo]", GG_GAMEDB_AVENUE_PAD_3_RUN | GG_GAMEDB_CDROM },

    {0, 0, 0}
};

#endif	/* GAME_DB_H */