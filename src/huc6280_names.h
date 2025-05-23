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
#ifndef HUC6280_NAME_H
#define HUC6280_NAME_H

enum GG_OPCode_Type
{
    GG_OPCode_Type_Implied,
    GG_OPCode_Type_1b,
    GG_OPCode_Type_1b_1b,
    GG_OPCode_Type_1b_2b,
    GG_OPCode_Type_2b,
    GG_OPCode_Type_2b_2b_2b,
    GG_OPCode_Type_1b_Relative,
    GG_OPCode_Type_1b_1b_Relative,
    GG_OPCode_Type_ST0,
};

struct GG_OPCode_Info
{
    const char* name;
    GG_OPCode_Type type;
};

static const GG_OPCode_Info k_huc6280_opcode_names[256] = {
    { "{n}BRK", GG_OPCode_Type_Implied },
    { "{n}ORA {o}$(%02X,X)", GG_OPCode_Type_1b },
    { "{n}SXY", GG_OPCode_Type_Implied },
    { "{n}ST0 {o}#$%02X  {e}[REG = %s]", GG_OPCode_Type_ST0 },
    { "{n}TSB {o}$%02X", GG_OPCode_Type_1b },
    { "{n}ORA {o}$%02X", GG_OPCode_Type_1b },
    { "{n}ASL {o}$%02X", GG_OPCode_Type_1b },
    { "{n}RMB {o}0,$%02X", GG_OPCode_Type_1b },
    { "{n}PHP", GG_OPCode_Type_Implied },
    { "{n}ORA {o}#$%02X", GG_OPCode_Type_1b },
    { "{n}ASL {o}A", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}TSB {o}$%04X", GG_OPCode_Type_2b },
    { "{n}ORA {o}$%04X", GG_OPCode_Type_2b },
    { "{n}ASL {o}$%04X", GG_OPCode_Type_2b },
    { "{n}BBR {o}0,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative },

    { "{n}BPL {o}$%04X  {e}[%+d]", GG_OPCode_Type_1b_Relative },
    { "{n}ORA {o}($%02X),Y", GG_OPCode_Type_1b },
    { "{n}ORA {o}($%02X)", GG_OPCode_Type_1b },
    { "{n}ST1 {o}#$%02X  {e}[LSB]", GG_OPCode_Type_1b },
    { "{n}TRB {o}$%02X", GG_OPCode_Type_1b },
    { "{n}ORA {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}ASL {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}RMB {o}1,$%02X", GG_OPCode_Type_1b },
    { "{n}CLC", GG_OPCode_Type_Implied },
    { "{n}ORA {o}$%04X,Y", GG_OPCode_Type_2b },
    { "{n}INC {o}A", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}TRB {o}$%04X", GG_OPCode_Type_2b },
    { "{n}ORA {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}ASL {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}BBR {o}1,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative },

    { "{n}JSR {o}$%04X", GG_OPCode_Type_2b },
    { "{n}AND {o}$(%02X,X)", GG_OPCode_Type_1b },
    { "{n}SAX", GG_OPCode_Type_Implied },
    { "{n}ST2 {o}#$%02X  {e}[MSB]", GG_OPCode_Type_1b },
    { "{n}BIT {o}$%02X", GG_OPCode_Type_1b },
    { "{n}AND {o}$%02X", GG_OPCode_Type_1b },
    { "{n}ROL {o}$%02X", GG_OPCode_Type_1b },
    { "{n}RMB {o}2,$%02X", GG_OPCode_Type_1b },
    { "{n}PLP", GG_OPCode_Type_Implied },
    { "{n}AND {o}#$%02X", GG_OPCode_Type_1b },
    { "{n}ROL {o}A", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}BIT {o}$%04X", GG_OPCode_Type_2b },
    { "{n}AND {o}$%04X", GG_OPCode_Type_2b },
    { "{n}ROL {o}$%04X", GG_OPCode_Type_2b },
    { "{n}BBR {o}2,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative },

    { "{n}BMI {o}$%04X  {e}[%+d]", GG_OPCode_Type_1b_Relative },
    { "{n}AND {o}($%02X),Y", GG_OPCode_Type_1b },
    { "{n}AND {o}($%02X)", GG_OPCode_Type_1b },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}BIT {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}AND {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}ROL {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}RMB {o}3,$%02X", GG_OPCode_Type_1b },
    { "{n}SEC", GG_OPCode_Type_Implied },
    { "{n}AND {o}$%04X,Y", GG_OPCode_Type_2b },
    { "{n}DEC {o}A", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}BIT {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}AND {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}ROL {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}BBR {o}3,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative },

    { "{n}RTI", GG_OPCode_Type_Implied },
    { "{n}EOR {o}$(%02X,X)", GG_OPCode_Type_1b },
    { "{n}SAY", GG_OPCode_Type_Implied },
    { "{n}TMA {o}#$%02X", GG_OPCode_Type_1b },
    { "{n}BSR {o}$%04X  {e}[%+d]", GG_OPCode_Type_1b_Relative },
    { "{n}EOR {o}$%02X", GG_OPCode_Type_1b },
    { "{n}LSR {o}$%02X", GG_OPCode_Type_1b },
    { "{n}RMB {o}4,$%02X", GG_OPCode_Type_1b },
    { "{n}PHA", GG_OPCode_Type_Implied },
    { "{n}EOR {o}#$%02X", GG_OPCode_Type_1b },
    { "{n}LSR {o}A", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}JMP {o}$%04X", GG_OPCode_Type_2b },
    { "{n}EOR {o}$%04X", GG_OPCode_Type_2b },
    { "{n}LSR {o}$%04X", GG_OPCode_Type_2b },
    { "{n}BBR {o}4,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative },

    { "{n}BVC {o}$%04X  {e}[%+d]", GG_OPCode_Type_1b_Relative },
    { "{n}EOR {o}($%02X),Y", GG_OPCode_Type_1b },
    { "{n}EOR {o}($%02X)", GG_OPCode_Type_1b },
    { "{n}TAM {o}#$%02X", GG_OPCode_Type_1b },
    { "{n}CSL", GG_OPCode_Type_Implied },
    { "{n}EOR {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}LSR {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}RMB {o}5,$%02X", GG_OPCode_Type_1b },
    { "{n}CLI", GG_OPCode_Type_Implied },
    { "{n}EOR {o}$%04X,Y", GG_OPCode_Type_2b },
    { "{n}PHY", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}EOR {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}LSR {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}BBR {o}5,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative },

    { "{n}RTS", GG_OPCode_Type_Implied },
    { "{n}ADC {o}$(%02X,X)", GG_OPCode_Type_1b },
    { "{n}CLA", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}STZ {o}$%02X", GG_OPCode_Type_1b },
    { "{n}ADC {o}$%02X", GG_OPCode_Type_1b },
    { "{n}ROR {o}$%02X", GG_OPCode_Type_1b },
    { "{n}RMB {o}6,$%02X", GG_OPCode_Type_1b },
    { "{n}PLA", GG_OPCode_Type_Implied },
    { "{n}ADC {o}#$%02X", GG_OPCode_Type_1b },
    { "{n}ROR {o}A", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}JMP {o}($%04X)", GG_OPCode_Type_2b },
    { "{n}ADC {o}$%04X", GG_OPCode_Type_2b },
    { "{n}ROR {o}$%04X", GG_OPCode_Type_2b },
    { "{n}BBR {o}6,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative },

    { "{n}BVS {o}$%04X  {e}[%+d]", GG_OPCode_Type_1b_Relative },
    { "{n}ADC {o}($%02X),Y", GG_OPCode_Type_1b },
    { "{n}ADC {o}($%02X)", GG_OPCode_Type_1b },
    { "{n}TII {o}$%04X,$%04X,#$%04X", GG_OPCode_Type_2b_2b_2b },
    { "{n}STZ {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}ADC {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}ROR {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}RMB {o}7,$%02X", GG_OPCode_Type_1b },
    { "{n}SEI", GG_OPCode_Type_Implied },
    { "{n}ADC {o}$%04X,Y", GG_OPCode_Type_2b },
    { "{n}PLY", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}JMP {o}$(%04X,X)", GG_OPCode_Type_2b },
    { "{n}ADC {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}ROR {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}BBR {o}7,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative },

    { "{n}BRA {o}$%04X  {e}[%+d]", GG_OPCode_Type_1b_Relative },
    { "{n}STA {o}$(%02X,X)", GG_OPCode_Type_1b },
    { "{n}CLX", GG_OPCode_Type_Implied },
    { "{n}TST {o}#$%02X,$%02X", GG_OPCode_Type_1b_1b },
    { "{n}STY {o}$%02X", GG_OPCode_Type_1b },
    { "{n}STA {o}$%02X", GG_OPCode_Type_1b },
    { "{n}STX {o}$%02X", GG_OPCode_Type_1b },
    { "{n}SMB {o}0,$%02X", GG_OPCode_Type_1b },
    { "{n}DEY", GG_OPCode_Type_Implied },
    { "{n}BIT {o}#$%02X", GG_OPCode_Type_1b },
    { "{n}TXA", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}STY {o}$%04X", GG_OPCode_Type_2b },
    { "{n}STA {o}$%04X", GG_OPCode_Type_2b },
    { "{n}STX {o}$%04X", GG_OPCode_Type_2b },
    { "{n}BBS {o}0,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative },

    { "{n}BCC {o}$%04X  {e}[%+d]", GG_OPCode_Type_1b_Relative },
    { "{n}STA {o}($%02X),Y", GG_OPCode_Type_1b },
    { "{n}STA {o}($%02X)", GG_OPCode_Type_1b },
    { "{n}TST {o}#$%02X,$%04X", GG_OPCode_Type_1b_2b },
    { "{n}STY {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}STA {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}STX {o}$%02X,Y", GG_OPCode_Type_1b },
    { "{n}SMB {o}1,$%02X", GG_OPCode_Type_1b },
    { "{n}TYA", GG_OPCode_Type_Implied },
    { "{n}STA {o}$%04X,Y", GG_OPCode_Type_2b },
    { "{n}TXS", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}STZ {o}$%04X", GG_OPCode_Type_2b },
    { "{n}STA {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}STZ {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}BBS {o}1,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative },

    { "{n}LDY {o}#$%02X", GG_OPCode_Type_1b },
    { "{n}LDA {o}$(%02X,X)", GG_OPCode_Type_1b },
    { "{n}LDX {o}#$%02X", GG_OPCode_Type_1b },
    { "{n}TST {o}#$%02X,$%02X,X", GG_OPCode_Type_1b_1b },
    { "{n}LDY {o}$%02X", GG_OPCode_Type_1b },
    { "{n}LDA {o}$%02X", GG_OPCode_Type_1b },
    { "{n}LDX {o}$%02X", GG_OPCode_Type_1b },
    { "{n}SMB {o}2,$%02X", GG_OPCode_Type_1b },
    { "{n}TAY", GG_OPCode_Type_Implied },
    { "{n}LDA {o}#$%02X", GG_OPCode_Type_1b },
    { "{n}TAX", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}LDY {o}$%04X", GG_OPCode_Type_2b },
    { "{n}LDA {o}$%04X", GG_OPCode_Type_2b },
    { "{n}LDX {o}$%04X", GG_OPCode_Type_2b },
    { "{n}BBS {o}2,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative },

    { "{n}BCS {o}$%04X  {e}[%+d]", GG_OPCode_Type_1b_Relative },
    { "{n}LDA {o}($%02X),Y", GG_OPCode_Type_1b },
    { "{n}LDA {o}($%02X)", GG_OPCode_Type_1b },
    { "{n}TST {o}#$%02X,$%04X,X", GG_OPCode_Type_1b_2b },
    { "{n}LDY {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}LDA {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}LDX {o}$%02X,Y", GG_OPCode_Type_1b },
    { "{n}SMB {o}3,$%02X", GG_OPCode_Type_1b },
    { "{n}CLV", GG_OPCode_Type_Implied },
    { "{n}LDA {o}$%04X,Y", GG_OPCode_Type_2b },
    { "{n}TSX", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}LDY {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}LDA {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}LDX {o}$%04X,Y", GG_OPCode_Type_2b },
    { "{n}BBS {o}3,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative },

    { "{n}CPY {o}#$%02X", GG_OPCode_Type_1b },
    { "{n}CMP {o}$(%02X,X)", GG_OPCode_Type_1b },
    { "{n}CLY", GG_OPCode_Type_Implied },
    { "{n}TDD {o}$%04X,$%04X,#$%04X", GG_OPCode_Type_2b_2b_2b },
    { "{n}CPY {o}$%02X", GG_OPCode_Type_1b },
    { "{n}CMP {o}$%02X", GG_OPCode_Type_1b },
    { "{n}DEC {o}$%02X", GG_OPCode_Type_1b },
    { "{n}SMB {o}4,$%02X", GG_OPCode_Type_1b },
    { "{n}INY", GG_OPCode_Type_Implied },
    { "{n}CMP {o}#$%02X", GG_OPCode_Type_1b },
    { "{n}DEX", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}CPY {o}$%04X", GG_OPCode_Type_2b },
    { "{n}CMP {o}$%04X", GG_OPCode_Type_2b },
    { "{n}DEC {o}$%04X", GG_OPCode_Type_2b },
    { "{n}BBS {o}4,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative },

    { "{n}BNE {o}$%04X  {e}[%+d]", GG_OPCode_Type_1b_Relative },
    { "{n}CMP {o}($%02X),Y", GG_OPCode_Type_1b },
    { "{n}CMP {o}($%02X)", GG_OPCode_Type_1b },
    { "{n}TIN {o}$%04X,$%04X,#$%04X", GG_OPCode_Type_2b_2b_2b },
    { "{n}CSH", GG_OPCode_Type_Implied },
    { "{n}CMP {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}DEC {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}SMB {o}5,$%02X", GG_OPCode_Type_1b },
    { "{n}CLD", GG_OPCode_Type_Implied },
    { "{n}CMP {o}$%04X,Y", GG_OPCode_Type_2b },
    { "{n}PHX", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}CMP {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}DEC {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}BBS {o}5,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative },

    { "{n}CPX {o}#$%02X", GG_OPCode_Type_1b },
    { "{n}SBC {o}$(%02X,X)", GG_OPCode_Type_1b },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}TIA {o}$%04X,$%04X,#$%04X", GG_OPCode_Type_2b_2b_2b },
    { "{n}CPX {o}$%02X", GG_OPCode_Type_1b },
    { "{n}SBC {o}$%02X", GG_OPCode_Type_1b },
    { "{n}INC {o}$%02X", GG_OPCode_Type_1b },
    { "{n}SMB {o}6,$%02X", GG_OPCode_Type_1b },
    { "{n}INX", GG_OPCode_Type_Implied },
    { "{n}SBC {o}#$%02X", GG_OPCode_Type_1b },
    { "{n}NOP", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}CPX {o}$%04X", GG_OPCode_Type_2b },
    { "{n}SBC {o}$%04X", GG_OPCode_Type_2b },
    { "{n}INC {o}$%04X", GG_OPCode_Type_2b },
    { "{n}BBS {o}6,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative },

    { "{n}BEQ {o}$%04X  {e}[%+d]", GG_OPCode_Type_1b_Relative },
    { "{n}SBC {o}($%02X),Y", GG_OPCode_Type_1b },
    { "{n}SBC {o}($%02X)", GG_OPCode_Type_1b },
    { "{n}TAI {o}$%04X,$%04X,#$%04X", GG_OPCode_Type_2b_2b_2b },
    { "{n}SET", GG_OPCode_Type_Implied },
    { "{n}SBC {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}INC {o}$%02X,X", GG_OPCode_Type_1b },
    { "{n}SMB {o}7,$%02X", GG_OPCode_Type_1b },
    { "{n}SED", GG_OPCode_Type_Implied },
    { "{n}SBC {o}$%04X,Y", GG_OPCode_Type_2b },
    { "{n}PLX", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}NOP  {e}[UNOFFICIAL]", GG_OPCode_Type_Implied },
    { "{n}SBC {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}INC {o}$%04X,X", GG_OPCode_Type_2b },
    { "{n}BBS {o}7,$%02X,$%04X  {e}[%+d]", GG_OPCode_Type_1b_1b_Relative }
};

#endif /* HUC6280_NAME_H */