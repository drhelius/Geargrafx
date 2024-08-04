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

#include <vector>
#include <algorithm>
#include "../src/geargrafx.h"
#include "RSJparser.tcc"

bool run_file(const char* filename);
bool run_test(RSJresource& test);

GeargrafxCore* core = nullptr;
HuC6280* cpu = nullptr;
Memory* memory = nullptr;

int main(int argc, char* argv[])
{
    int ret = 0;

    core = new GeargrafxCore();
    core->Init();
    cpu = core->GetHuC6280();
    memory = core->GetMemory();

    char file_number[4];
    char file_name[10];

    for (int i = 0x00; i < 0x100; i++)
    {
        snprintf(file_number, 4, "%02x", i);
        snprintf(file_name, 10, "%s.json", file_number);

        if (!run_file(file_name))
        {
            ret = 1;
            break;
        }
    }

    SafeDelete(core);

    return ret;
}

bool run_file(const char* filename)
{
    std::ifstream json_fstream (filename);

    if (!json_fstream.is_open())
    {
        Log("%s not found", filename);
        return true;
    }

    RSJresource json_file(json_fstream);

    int i = 0;
    bool failed = false;

    for (auto it=json_file.as_array().begin(); it!=json_file.as_array().end(); ++it)
    {
        RSJresource test = (*it);

        if (!run_test(test))
        {
            Log("%s: test %d failed - %s", filename, i, test["name"].as_str().c_str());
            std::cout << test.as_str() << std::endl;
            failed = true;
            break;
        }

        i++;
    }

    if (!failed)
        Log("%s: %d tests passed", filename, i);

    return !failed;
}

bool run_test(RSJresource& test)
{
    bool failed = false;

    cpu->GetState()->PC->SetValue(test["initial"]["pc"].as<int>());
    cpu->GetState()->S->SetValue(test["initial"]["s"].as<int>());
    cpu->GetState()->A->SetValue(test["initial"]["a"].as<int>());
    cpu->GetState()->X->SetValue(test["initial"]["x"].as<int>());
    cpu->GetState()->Y->SetValue(test["initial"]["y"].as<int>());
    cpu->GetState()->P->SetValue(test["initial"]["p"].as<int>());

    for (auto it=test["initial"]["ram"].as_array().begin(); it!=test["initial"]["ram"].as_array().end(); ++it)
    {
        memory->Write((*it)[0].as<int>(), (*it)[1].as<int>());
    }

    unsigned int cycles = cpu->Tick();

#if 0
    unsigned int expected_cycles = test["cycles"].as_array().size();

    if (cycles != expected_cycles)
    {
        Log("Cycles failed, expected: %d got: %d", expected_cycles, cycles);
        failed = true;
    }
#endif

    u16 pc = cpu->GetState()->PC->GetValue();
    u8 s = cpu->GetState()->S->GetValue();
    u8 a = cpu->GetState()->A->GetValue();
    u8 x = cpu->GetState()->X->GetValue();
    u8 y = cpu->GetState()->Y->GetValue();
    u8 p = cpu->GetState()->P->GetValue();

    if (pc != test["final"]["pc"].as<int>())
    {
        Log("PC failed, expected: %04X got: %04X", test["final"]["pc"].as<int>(), pc);
        failed = true;
    }

    if (s != test["final"]["s"].as<int>())
    {
        Log("S failed, expected: %02X got: %02X", test["final"]["s"].as<int>(), s);
        failed = true;
    }

    if (a != test["final"]["a"].as<int>())
    {
        Log("A failed, expected: %02X got: %02X", test["final"]["a"].as<int>(), a);
        failed = true;
    }

    if (x != test["final"]["x"].as<int>())
    {
        Log("X failed, expected: %02X got: %02X", test["final"]["x"].as<int>(), x);
        failed = true;
    }

    if (y != test["final"]["y"].as<int>())
    {
        Log("Y failed, expected: %02X got: %02X", test["final"]["y"].as<int>(), y);
        failed = true;
    }

    if (p != test["final"]["p"].as<int>())
    {
        Log("P failed, expected: %02X got: %02X", test["final"]["p"].as<int>(), p);
        failed = true;
    }

    for (auto it=test["final"]["ram"].as_array().begin(); it!=test["final"]["ram"].as_array().end(); ++it)
    {
        u16 address = (*it)[0].as<int>();
        u8 value = memory->Read(address);
        u8 expected = (*it)[1].as<int>();

        if (value != expected)
        {
            Log("RAM failed at %04X, expected: %02X got: %02X", address, expected, value);
            failed = true;
        }
    }

    return !failed;
}