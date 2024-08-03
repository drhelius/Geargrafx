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

#include "../src/geargrafx.h"
#include "RSJparser.tcc"

int main(int argc, char* argv[])
{
    int ret = 0;
    bool failed = false;

    GeargrafxCore* core = new GeargrafxCore();
    core->Init();
    HuC6280* cpu = core->GetHuC6280();
    Memory* memory = core->GetMemory();

    // std::string str = "{ 'name': 'ca 11 df', 'initial': { 'pc': 25402, 's': 171, 'a': 34, 'x': 222, 'y': 136, 'p': 234, 'ram': [ [25402, 202], [25403, 17], [25404, 223]]}, 'final': { 'pc': 25403, 's': 171, 'a': 34, 'x': 221, 'y': 136, 'p': 232, 'ram': [ [25402, 202], [25403, 17], [25404, 223]]}, 'cycles': [ [25402, 202, 'read'], [25403, 17, 'read']] }";

    std::string str = "{ 'name': 'cf 1', 'initial': {'pc': 31248, 's': 32, 'a': 103, 'x': 237, 'y': 178, 'p': 184, 'ram': [[31250, 246], [230, 197], [31249, 230], [31248, 207]]}, 'final': {'pc': 31251, 's': 32, 'a': 103, 'x': 237, 'y': 178, 'p': 184, 'ram': [[31250, 246], [230, 197], [31249, 230], [31248, 207]]}, 'cycles': [[31248, 207, 'read'],[31249, 230, 'read'],[230, 197, 'read'],[230, 197, 'read'],[31250, 246, 'read']]}";

    RSJresource my_resource (str);

    cpu->GetState()->PC->SetValue(my_resource["initial"]["pc"].as<int>());
    cpu->GetState()->S->SetValue(my_resource["initial"]["s"].as<int>());
    cpu->GetState()->A->SetValue(my_resource["initial"]["a"].as<int>());
    cpu->GetState()->X->SetValue(my_resource["initial"]["x"].as<int>());
    cpu->GetState()->Y->SetValue(my_resource["initial"]["y"].as<int>());
    cpu->GetState()->P->SetValue(my_resource["initial"]["p"].as<int>());

    for (auto it=my_resource["initial"]["ram"].as_array().begin(); it!=my_resource["initial"]["ram"].as_array().end(); ++it)
    {
        memory->Write((*it)[0].as<int>(), (*it)[1].as<int>());
    }

    unsigned int cycles = cpu->Tick();

#if 0
    unsigned int expected_cycles = my_resource["cycles"].as_array().size();

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

    if (pc != my_resource["final"]["pc"].as<int>())
    {
        Log("PC failed, expected: %04X got: %04X", my_resource["final"]["pc"].as<int>(), pc);
        failed = true;
    }

    if (s != my_resource["final"]["s"].as<int>())
    {
        Log("S failed, expected: %02X got: %02X", my_resource["final"]["s"].as<int>(), s);
        failed = true;
    }

    if (a != my_resource["final"]["a"].as<int>())
    {
        Log("A failed, expected: %02X got: %02X", my_resource["final"]["a"].as<int>(), a);
        failed = true;
    }

    if (x != my_resource["final"]["x"].as<int>())
    {
        Log("X failed, expected: %02X got: %02X", my_resource["final"]["x"].as<int>(), x);
        failed = true;
    }

    if (y != my_resource["final"]["y"].as<int>())
    {
        Log("Y failed, expected: %02X got: %02X", my_resource["final"]["y"].as<int>(), y);
        failed = true;
    }

    if (p != my_resource["final"]["p"].as<int>())
    {
        Log("P failed, expected: %02X got: %02X", my_resource["final"]["p"].as<int>(), p);
        failed = true;
    }

    for (auto it=my_resource["final"]["ram"].as_array().begin(); it!=my_resource["final"]["ram"].as_array().end(); ++it)
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

    if (failed)
    {
        ret = 1;
        Log("Test failed: %s", str.c_str());
    }
    else
    {
        Log("Test passed");
    }

    SafeDelete(core);

    return ret;
}