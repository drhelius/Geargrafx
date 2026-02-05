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

#ifndef HUC6280_DISPATCH_H
#define HUC6280_DISPATCH_H

#if defined(__GNUC__) || defined(__clang__)

#define HUC6280_DISPATCH(opcode) \
    do { \
        static const void* JUMPTABLE[256] = { \
            &&op00, &&op01, &&op02, &&op03, &&op04, &&op05, &&op06, &&op07, &&op08, &&op09, &&op0A, &&op0B, &&op0C, &&op0D, &&op0E, &&op0F, \
            &&op10, &&op11, &&op12, &&op13, &&op14, &&op15, &&op16, &&op17, &&op18, &&op19, &&op1A, &&op1B, &&op1C, &&op1D, &&op1E, &&op1F, \
            &&op20, &&op21, &&op22, &&op23, &&op24, &&op25, &&op26, &&op27, &&op28, &&op29, &&op2A, &&op2B, &&op2C, &&op2D, &&op2E, &&op2F, \
            &&op30, &&op31, &&op32, &&op33, &&op34, &&op35, &&op36, &&op37, &&op38, &&op39, &&op3A, &&op3B, &&op3C, &&op3D, &&op3E, &&op3F, \
            &&op40, &&op41, &&op42, &&op43, &&op44, &&op45, &&op46, &&op47, &&op48, &&op49, &&op4A, &&op4B, &&op4C, &&op4D, &&op4E, &&op4F, \
            &&op50, &&op51, &&op52, &&op53, &&op54, &&op55, &&op56, &&op57, &&op58, &&op59, &&op5A, &&op5B, &&op5C, &&op5D, &&op5E, &&op5F, \
            &&op60, &&op61, &&op62, &&op63, &&op64, &&op65, &&op66, &&op67, &&op68, &&op69, &&op6A, &&op6B, &&op6C, &&op6D, &&op6E, &&op6F, \
            &&op70, &&op71, &&op72, &&op73, &&op74, &&op75, &&op76, &&op77, &&op78, &&op79, &&op7A, &&op7B, &&op7C, &&op7D, &&op7E, &&op7F, \
            &&op80, &&op81, &&op82, &&op83, &&op84, &&op85, &&op86, &&op87, &&op88, &&op89, &&op8A, &&op8B, &&op8C, &&op8D, &&op8E, &&op8F, \
            &&op90, &&op91, &&op92, &&op93, &&op94, &&op95, &&op96, &&op97, &&op98, &&op99, &&op9A, &&op9B, &&op9C, &&op9D, &&op9E, &&op9F, \
            &&opA0, &&opA1, &&opA2, &&opA3, &&opA4, &&opA5, &&opA6, &&opA7, &&opA8, &&opA9, &&opAA, &&opAB, &&opAC, &&opAD, &&opAE, &&opAF, \
            &&opB0, &&opB1, &&opB2, &&opB3, &&opB4, &&opB5, &&opB6, &&opB7, &&opB8, &&opB9, &&opBA, &&opBB, &&opBC, &&opBD, &&opBE, &&opBF, \
            &&opC0, &&opC1, &&opC2, &&opC3, &&opC4, &&opC5, &&opC6, &&opC7, &&opC8, &&opC9, &&opCA, &&opCB, &&opCC, &&opCD, &&opCE, &&opCF, \
            &&opD0, &&opD1, &&opD2, &&opD3, &&opD4, &&opD5, &&opD6, &&opD7, &&opD8, &&opD9, &&opDA, &&opDB, &&opDC, &&opDD, &&opDE, &&opDF, \
            &&opE0, &&opE1, &&opE2, &&opE3, &&opE4, &&opE5, &&opE6, &&opE7, &&opE8, &&opE9, &&opEA, &&opEB, &&opEC, &&opED, &&opEE, &&opEF, \
            &&opF0, &&opF1, &&opF2, &&opF3, &&opF4, &&opF5, &&opF6, &&opF7, &&opF8, &&opF9, &&opFA, &&opFB, &&opFC, &&opFD, &&opFE, &&opFF  \
        }; \
        goto *JUMPTABLE[opcode]; \
        op00: OPCode0x00(); goto done; op01: OPCode0x01(); goto done; op02: OPCode0x02(); goto done; op03: OPCode0x03(); goto done; \
        op04: OPCode0x04(); goto done; op05: OPCode0x05(); goto done; op06: OPCode0x06(); goto done; op07: OPCode0x07(); goto done; \
        op08: OPCode0x08(); goto done; op09: OPCode0x09(); goto done; op0A: OPCode0x0A(); goto done; op0B: OPCode0x0B(); goto done; \
        op0C: OPCode0x0C(); goto done; op0D: OPCode0x0D(); goto done; op0E: OPCode0x0E(); goto done; op0F: OPCode0x0F(); goto done; \
        op10: OPCode0x10(); goto done; op11: OPCode0x11(); goto done; op12: OPCode0x12(); goto done; op13: OPCode0x13(); goto done; \
        op14: OPCode0x14(); goto done; op15: OPCode0x15(); goto done; op16: OPCode0x16(); goto done; op17: OPCode0x17(); goto done; \
        op18: OPCode0x18(); goto done; op19: OPCode0x19(); goto done; op1A: OPCode0x1A(); goto done; op1B: OPCode0x1B(); goto done; \
        op1C: OPCode0x1C(); goto done; op1D: OPCode0x1D(); goto done; op1E: OPCode0x1E(); goto done; op1F: OPCode0x1F(); goto done; \
        op20: OPCode0x20(); goto done; op21: OPCode0x21(); goto done; op22: OPCode0x22(); goto done; op23: OPCode0x23(); goto done; \
        op24: OPCode0x24(); goto done; op25: OPCode0x25(); goto done; op26: OPCode0x26(); goto done; op27: OPCode0x27(); goto done; \
        op28: OPCode0x28(); goto done; op29: OPCode0x29(); goto done; op2A: OPCode0x2A(); goto done; op2B: OPCode0x2B(); goto done; \
        op2C: OPCode0x2C(); goto done; op2D: OPCode0x2D(); goto done; op2E: OPCode0x2E(); goto done; op2F: OPCode0x2F(); goto done; \
        op30: OPCode0x30(); goto done; op31: OPCode0x31(); goto done; op32: OPCode0x32(); goto done; op33: OPCode0x33(); goto done; \
        op34: OPCode0x34(); goto done; op35: OPCode0x35(); goto done; op36: OPCode0x36(); goto done; op37: OPCode0x37(); goto done; \
        op38: OPCode0x38(); goto done; op39: OPCode0x39(); goto done; op3A: OPCode0x3A(); goto done; op3B: OPCode0x3B(); goto done; \
        op3C: OPCode0x3C(); goto done; op3D: OPCode0x3D(); goto done; op3E: OPCode0x3E(); goto done; op3F: OPCode0x3F(); goto done; \
        op40: OPCode0x40(); goto done; op41: OPCode0x41(); goto done; op42: OPCode0x42(); goto done; op43: OPCode0x43(); goto done; \
        op44: OPCode0x44(); goto done; op45: OPCode0x45(); goto done; op46: OPCode0x46(); goto done; op47: OPCode0x47(); goto done; \
        op48: OPCode0x48(); goto done; op49: OPCode0x49(); goto done; op4A: OPCode0x4A(); goto done; op4B: OPCode0x4B(); goto done; \
        op4C: OPCode0x4C(); goto done; op4D: OPCode0x4D(); goto done; op4E: OPCode0x4E(); goto done; op4F: OPCode0x4F(); goto done; \
        op50: OPCode0x50(); goto done; op51: OPCode0x51(); goto done; op52: OPCode0x52(); goto done; op53: OPCode0x53(); goto done; \
        op54: OPCode0x54(); goto done; op55: OPCode0x55(); goto done; op56: OPCode0x56(); goto done; op57: OPCode0x57(); goto done; \
        op58: OPCode0x58(); goto done; op59: OPCode0x59(); goto done; op5A: OPCode0x5A(); goto done; op5B: OPCode0x5B(); goto done; \
        op5C: OPCode0x5C(); goto done; op5D: OPCode0x5D(); goto done; op5E: OPCode0x5E(); goto done; op5F: OPCode0x5F(); goto done; \
        op60: OPCode0x60(); goto done; op61: OPCode0x61(); goto done; op62: OPCode0x62(); goto done; op63: OPCode0x63(); goto done; \
        op64: OPCode0x64(); goto done; op65: OPCode0x65(); goto done; op66: OPCode0x66(); goto done; op67: OPCode0x67(); goto done; \
        op68: OPCode0x68(); goto done; op69: OPCode0x69(); goto done; op6A: OPCode0x6A(); goto done; op6B: OPCode0x6B(); goto done; \
        op6C: OPCode0x6C(); goto done; op6D: OPCode0x6D(); goto done; op6E: OPCode0x6E(); goto done; op6F: OPCode0x6F(); goto done; \
        op70: OPCode0x70(); goto done; op71: OPCode0x71(); goto done; op72: OPCode0x72(); goto done; op73: OPCode0x73(); goto done; \
        op74: OPCode0x74(); goto done; op75: OPCode0x75(); goto done; op76: OPCode0x76(); goto done; op77: OPCode0x77(); goto done; \
        op78: OPCode0x78(); goto done; op79: OPCode0x79(); goto done; op7A: OPCode0x7A(); goto done; op7B: OPCode0x7B(); goto done; \
        op7C: OPCode0x7C(); goto done; op7D: OPCode0x7D(); goto done; op7E: OPCode0x7E(); goto done; op7F: OPCode0x7F(); goto done; \
        op80: OPCode0x80(); goto done; op81: OPCode0x81(); goto done; op82: OPCode0x82(); goto done; op83: OPCode0x83(); goto done; \
        op84: OPCode0x84(); goto done; op85: OPCode0x85(); goto done; op86: OPCode0x86(); goto done; op87: OPCode0x87(); goto done; \
        op88: OPCode0x88(); goto done; op89: OPCode0x89(); goto done; op8A: OPCode0x8A(); goto done; op8B: OPCode0x8B(); goto done; \
        op8C: OPCode0x8C(); goto done; op8D: OPCode0x8D(); goto done; op8E: OPCode0x8E(); goto done; op8F: OPCode0x8F(); goto done; \
        op90: OPCode0x90(); goto done; op91: OPCode0x91(); goto done; op92: OPCode0x92(); goto done; op93: OPCode0x93(); goto done; \
        op94: OPCode0x94(); goto done; op95: OPCode0x95(); goto done; op96: OPCode0x96(); goto done; op97: OPCode0x97(); goto done; \
        op98: OPCode0x98(); goto done; op99: OPCode0x99(); goto done; op9A: OPCode0x9A(); goto done; op9B: OPCode0x9B(); goto done; \
        op9C: OPCode0x9C(); goto done; op9D: OPCode0x9D(); goto done; op9E: OPCode0x9E(); goto done; op9F: OPCode0x9F(); goto done; \
        opA0: OPCode0xA0(); goto done; opA1: OPCode0xA1(); goto done; opA2: OPCode0xA2(); goto done; opA3: OPCode0xA3(); goto done; \
        opA4: OPCode0xA4(); goto done; opA5: OPCode0xA5(); goto done; opA6: OPCode0xA6(); goto done; opA7: OPCode0xA7(); goto done; \
        opA8: OPCode0xA8(); goto done; opA9: OPCode0xA9(); goto done; opAA: OPCode0xAA(); goto done; opAB: OPCode0xAB(); goto done; \
        opAC: OPCode0xAC(); goto done; opAD: OPCode0xAD(); goto done; opAE: OPCode0xAE(); goto done; opAF: OPCode0xAF(); goto done; \
        opB0: OPCode0xB0(); goto done; opB1: OPCode0xB1(); goto done; opB2: OPCode0xB2(); goto done; opB3: OPCode0xB3(); goto done; \
        opB4: OPCode0xB4(); goto done; opB5: OPCode0xB5(); goto done; opB6: OPCode0xB6(); goto done; opB7: OPCode0xB7(); goto done; \
        opB8: OPCode0xB8(); goto done; opB9: OPCode0xB9(); goto done; opBA: OPCode0xBA(); goto done; opBB: OPCode0xBB(); goto done; \
        opBC: OPCode0xBC(); goto done; opBD: OPCode0xBD(); goto done; opBE: OPCode0xBE(); goto done; opBF: OPCode0xBF(); goto done; \
        opC0: OPCode0xC0(); goto done; opC1: OPCode0xC1(); goto done; opC2: OPCode0xC2(); goto done; opC3: OPCode0xC3(); goto done; \
        opC4: OPCode0xC4(); goto done; opC5: OPCode0xC5(); goto done; opC6: OPCode0xC6(); goto done; opC7: OPCode0xC7(); goto done; \
        opC8: OPCode0xC8(); goto done; opC9: OPCode0xC9(); goto done; opCA: OPCode0xCA(); goto done; opCB: OPCode0xCB(); goto done; \
        opCC: OPCode0xCC(); goto done; opCD: OPCode0xCD(); goto done; opCE: OPCode0xCE(); goto done; opCF: OPCode0xCF(); goto done; \
        opD0: OPCode0xD0(); goto done; opD1: OPCode0xD1(); goto done; opD2: OPCode0xD2(); goto done; opD3: OPCode0xD3(); goto done; \
        opD4: OPCode0xD4(); goto done; opD5: OPCode0xD5(); goto done; opD6: OPCode0xD6(); goto done; opD7: OPCode0xD7(); goto done; \
        opD8: OPCode0xD8(); goto done; opD9: OPCode0xD9(); goto done; opDA: OPCode0xDA(); goto done; opDB: OPCode0xDB(); goto done; \
        opDC: OPCode0xDC(); goto done; opDD: OPCode0xDD(); goto done; opDE: OPCode0xDE(); goto done; opDF: OPCode0xDF(); goto done; \
        opE0: OPCode0xE0(); goto done; opE1: OPCode0xE1(); goto done; opE2: OPCode0xE2(); goto done; opE3: OPCode0xE3(); goto done; \
        opE4: OPCode0xE4(); goto done; opE5: OPCode0xE5(); goto done; opE6: OPCode0xE6(); goto done; opE7: OPCode0xE7(); goto done; \
        opE8: OPCode0xE8(); goto done; opE9: OPCode0xE9(); goto done; opEA: OPCode0xEA(); goto done; opEB: OPCode0xEB(); goto done; \
        opEC: OPCode0xEC(); goto done; opED: OPCode0xED(); goto done; opEE: OPCode0xEE(); goto done; opEF: OPCode0xEF(); goto done; \
        opF0: OPCode0xF0(); goto done; opF1: OPCode0xF1(); goto done; opF2: OPCode0xF2(); goto done; opF3: OPCode0xF3(); goto done; \
        opF4: OPCode0xF4(); goto done; opF5: OPCode0xF5(); goto done; opF6: OPCode0xF6(); goto done; opF7: OPCode0xF7(); goto done; \
        opF8: OPCode0xF8(); goto done; opF9: OPCode0xF9(); goto done; opFA: OPCode0xFA(); goto done; opFB: OPCode0xFB(); goto done; \
        opFC: OPCode0xFC(); goto done; opFD: OPCode0xFD(); goto done; opFE: OPCode0xFE(); goto done; opFF: OPCode0xFF(); goto done; \
        done:; \
    } while (0)

#else

#define HUC6280_DISPATCH(opcode) (this->*m_opcodes[opcode])()

#endif

#endif /* HUC6280_DISPATCH_H */
