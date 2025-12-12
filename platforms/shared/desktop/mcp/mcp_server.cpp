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

#include "mcp_server.h"
#include <sstream>
#include <iomanip>

static void* ReaderThreadFunc(void* arg)
{
    McpServer* server = (McpServer*)arg;
    server->ReaderLoop();
    return NULL;
}

void McpServer::ReaderLoop()
{
    while (m_running.load())
    {
        std::string line;
        if (m_transport->recv(line))
        {
            if (!line.empty())
            {
                HandleLine(line);
            }
        }
        else
        {
            // EOF or error
            Stop();
            break;
        }
    }
}

void McpServer::Run()
{
    // Start reader thread for stdin
    std::thread reader_thread(ReaderThreadFunc, this);
    reader_thread.detach();

    // Main loop: wait for responses and send them
    while (m_running.load())
    {
        DebugResponse* resp = m_responseQueue.WaitAndPop();
        if (resp == NULL)
            break;

        if (resp->isError)
        {
            SendError(resp->requestId, resp->errorCode, resp->errorMessage);
        }
        else
        {
            // Wrap result in MCP response format
            json mcpResult;
            mcpResult["content"] = json::array();

            // Check if this is image data (special marker from GetScreenshot)
            if (resp->result.contains("__mcp_image") && resp->result["__mcp_image"] == true)
            {
                // Image content type
                mcpResult["content"].push_back({
                    {"type", "image"},
                    {"data", resp->result["data"]},
                    {"mimeType", resp->result["mimeType"]}
                });
            }
            else
            {
                // Text content type (default)
                std::ostringstream result_ss;
                result_ss << resp->result.dump(2);

                mcpResult["content"].push_back({
                    {"type", "text"},
                    {"text", result_ss.str()}
                });
            }

            json response;
            response["jsonrpc"] = "2.0";
            response["id"] = resp->requestId;
            response["result"] = mcpResult;

            SendResponse(response);
        }

        SafeDelete(resp);
    }
}

void McpServer::HandleLine(const std::string& line)
{
    json request;

    // Try to parse JSON
    if (!json::accept(line))
    {
        SendError(0, -32700, "Parse error: Invalid JSON");
        return;
    }

    request = json::parse(line);

    // Validate JSON-RPC structure
    if (!request.contains("jsonrpc") || request["jsonrpc"] != "2.0")
    {
        SendError(0, -32600, "Invalid Request: missing or invalid jsonrpc version");
        return;
    }

    if (!request.contains("method") || !request["method"].is_string())
    {
        SendError(0, -32600, "Invalid Request: missing method");
        return;
    }

    std::string method = request["method"];

    // Handle different methods
    if (method == "initialize")
    {
        HandleInitialize(request);
    }
    else if (method == "notifications/initialized")
    {
        // No response required for notifications
    }
    else if (method == "tools/list")
    {
        HandleToolsList(request);
    }
    else if (method == "tools/call")
    {
        HandleToolsCall(request);
    }
    else
    {
        int64_t id = request.contains("id") ? request["id"].get<int64_t>() : 0;
        SendError(id, -32601, "Method not found: " + method);
    }
}

void McpServer::HandleInitialize(const json& request)
{
    if (!request.contains("id"))
    {
        SendError(0, -32600, "Invalid Request: missing id");
        return;
    }

    int64_t id = request["id"];

    // Get protocol version from request
    std::string protocolVersion = "2024-11-05";
    if (request.contains("params") && request["params"].contains("protocolVersion"))
    {
        protocolVersion = request["params"]["protocolVersion"];
    }

    json response;
    response["jsonrpc"] = "2.0";
    response["id"] = id;
    response["result"] = {
        {"protocolVersion", protocolVersion},
        {"capabilities", {
            {"tools", json::object()}
        }},
        {"serverInfo", {
            {"name", "geargrafx-mcp-server"},
            {"version", "1.0.0"}
        }}
    };

    m_initialized = true;
    SendResponse(response);
}

void McpServer::HandleToolsList(const json& request)
{
    if (!request.contains("id"))
    {
        SendError(0, -32600, "Invalid Request: missing id");
        return;
    }

    int64_t id = request["id"];

    json tools = json::array();

    // Execution control tools
    tools.push_back({
        {"name", "debug_pause"},
        {"description", "Pause Geargrafx PC Engine / TurboGrafx-16 emulator execution (break at current instruction)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "debug_continue"},
        {"description", "Resume Geargrafx emulator execution from current breakpoint"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "debug_step_into"},
        {"description", "Step into next HuC6280 instruction (enters subroutines)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "debug_step_over"},
        {"description", "Step over next HuC6280 instruction (skips subroutines like JSR)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "debug_step_out"},
        {"description", "Step out of current subroutine (continues until RTS/RTI)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "debug_step_frame"},
        {"description", "Step one video frame (executes until next VBLANK on PC Engine)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "debug_reset"},
        {"description", "Reset the PC Engine / TurboGrafx-16 emulated system"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "debug_get_status"},
        {"description", "Get current debugger status (paused: true/false, at_breakpoint: true/false, pc: address if at breakpoint)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    // Breakpoint tools
    tools.push_back({
        {"name", "set_breakpoint"},
        {"description", "Set a breakpoint at specified address in PC Engine memory (ROM/RAM, VRAM, Palette, or hardware registers)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
        {"address", {
            {"type", "string"},
            {"description", "Hex address (e.g., '8000', '0x8000', '$8000')"}
        }},
        {"memory_area", {
            {"type", "string"},
            {"description", "Memory area: rom_ram (default), vram, palette, huc6270_reg, huc6260_reg"},
            {"enum", json::array({"rom_ram", "vram", "palette", "huc6270_reg", "huc6260_reg"})}
        }},
        {"type", {
            {"type", "string"},
            {"description", "Breakpoint type: exec (default), read, write"},
            {"enum", json::array({"exec", "read", "write"})}
        }}
            }},
            {"required", json::array({"address"})}
        }}
    });

    tools.push_back({
        {"name", "set_breakpoint_range"},
        {"description", "Set a breakpoint for an address range"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
        {"start_address", {
            {"type", "string"},
            {"description", "Start hex address (e.g., '8000')"}
        }},
        {"end_address", {
            {"type", "string"},
            {"description", "End hex address (e.g., '8FFF')"}
        }},
        {"memory_area", {
            {"type", "string"},
            {"description", "Memory area: rom_ram, vram, palette, huc6270_reg, huc6260_reg"},
            {"enum", json::array({"rom_ram", "vram", "palette", "huc6270_reg", "huc6260_reg"})}
        }},
        {"type", {
            {"type", "string"},
            {"description", "Breakpoint type: exec, read, write"},
            {"enum", json::array({"exec", "read", "write"})}
        }}
            }},
            {"required", json::array({"start_address", "end_address"})}
        }}
    });

    tools.push_back({
        {"name", "remove_breakpoint"},
        {"description", "Clear a breakpoint. Single address breakpoints: provide 'address' only. Range breakpoints: provide both 'address' and 'end_address' matching the exact range"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
        {"address", {
            {"type", "string"},
            {"description", "Hex address (e.g., '8000'). For ranges: the start address"}
        }},
        {"end_address", {
            {"type", "string"},
            {"description", "Hex end address (e.g., '8FFF'). Required only for range breakpoints. Must match the end address used when creating the range"}
        }},
        {"memory_area", {
            {"type", "string"},
            {"description", "Memory area: rom_ram, vram, palette, huc6270_reg, huc6260_reg"},
            {"enum", json::array({"rom_ram", "vram", "palette", "huc6270_reg", "huc6260_reg"})}
        }},
        {"type", {
            {"type", "string"},
            {"description", "Breakpoint type: exec, read, write"},
            {"enum", json::array({"exec", "read", "write"})}
        }}
            }},
            {"required", json::array({"address"})}
        }}
    });

    tools.push_back({
        {"name", "list_breakpoints"},
        {"description", "List all breakpoints"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    // Memory tools
    tools.push_back({
        {"name", "list_memory_areas"},
        {"description", "List all available memory areas (RAM, ROM, VRAM, etc.)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "read_memory"},
        {"description", "Read memory from a specific memory area"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
        {"area", {
            {"type", "integer"},
            {"description", "Memory area ID (use list_memory_areas to get IDs)"}
        }},
        {"offset", {
            {"type", "string"},
            {"description", "Hex offset within the area (e.g., '0100')"}
        }},
        {"size", {
            {"type", "integer"},
            {"description", "Number of bytes to read"}
        }}
            }},
            {"required", json::array({"area", "offset", "size"})}
        }}
    });

    tools.push_back({
        {"name", "write_memory"},
        {"description", "Write memory to a specific memory area"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
        {"area", {
            {"type", "integer"},
            {"description", "Memory area ID (use list_memory_areas to get IDs)"}
        }},
        {"offset", {
            {"type", "string"},
            {"description", "Hex offset within the area (e.g., '0100')"}
        }},
        {"bytes", {
            {"type", "string"},
            {"description", "Hex bytes separated by spaces (e.g., 'A9 00 85 10')"}
        }}
            }},
            {"required", json::array({"area", "offset", "bytes"})}
        }}
    });

    // Register tools
    tools.push_back({
        {"name", "write_huc6280_register"},
        {"description", "Write to a HuC6280 CPU register"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
        {"name", {
            {"type", "string"},
            {"description", "Register name (PC, A, X, Y, S, P)"}
        }},
        {"value", {
            {"type", "string"},
            {"description", "Hex value"}
        }}
            }},
            {"required", json::array({"name", "value"})}
        }}
    });

    // Disassembly tool
    tools.push_back({
        {"name", "debug_get_disassembly"},
        {"description", "Get disassembled HuC6280 assembly code from PC Engine memory. Returns address, bank, segment, instruction, and raw bytes."},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
        {"start", {
            {"type", "string"},
            {"description", "Start hex address (optional, defaults to PC). Accepts formats: 'E177', '0xE177', '$E177'"}
        }},
        {"offset", {
            {"type", "integer"},
            {"description", "Number of instruction lines to disassemble (default 15)"}
        }}
            }}
        }}
    });

    // Media info tool
    tools.push_back({
        {"name", "get_media_info"},
        {"description", "Get information about the loaded PC Engine ROM or CD-ROM (file path, type, size, console type, mapper, BIOS paths, etc.)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    // Chip status tools
    tools.push_back({
        {"name", "get_huc6280_status"},
        {"description", "Get HuC6280 CPU status (registers, MPR, timer, interrupts, I/O, speed)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "get_huc6270_registers"},
        {"description", "Get all 32 HuC6270 VDC registers. Use vdc parameter (1 or 2) for SuperGrafx"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
        {"vdc", {
            {"type", "integer"},
            {"description", "VDC number (1 or 2 for SuperGrafx, default 1)"}
        }}
            }}
        }}
    });

    tools.push_back({
        {"name", "get_huc6270_status"},
        {"description", "Get HuC6270 VDC status (position, state, control, interrupts). Use vdc parameter (1 or 2) for SuperGrafx"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
        {"vdc", {
            {"type", "integer"},
            {"description", "VDC number (1 or 2 for SuperGrafx, default 1)"}
        }}
            }}
        }}
    });

    tools.push_back({
        {"name", "get_huc6260_status"},
        {"description", "Get HuC6260 VCE status (position, sync signals, control register, CTA, blur, B&W)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "get_huc6202_status"},
        {"description", "Get HuC6202 VPC status (only for SuperGrafx games - window priority, selected VDC, IRQ status)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "get_psg_status"},
        {"description", "Get PSG (Programmable Sound Generator) status for all 6 channels (frequency, amplitude, waveform, noise, DDA)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "get_cdrom_status"},
        {"description", "Get CD-ROM drive status (only for CD-ROM games)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "get_arcade_card_status"},
        {"description", "Get Arcade Card status (only for Arcade Card games)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "get_cdrom_audio_status"},
        {"description", "Get CD-ROM audio playback status (only for CD-ROM games)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "get_adpcm_status"},
        {"description", "Get ADPCM audio status (only for CD-ROM games)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "get_screenshot"},
        {"description", "Capture current PC Engine / TurboGrafx-16 screen frame as base64-encoded PNG image"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "list_sprites"},
        {"description", "List information for all 64 hardware sprites (position, size, pattern index, palette, flags). Use vdc parameter (1 or 2) for SuperGrafx dual VDC"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
        {"vdc", {
            {"type", "integer"},
            {"description", "VDC number (1 or 2 for SuperGrafx, default 1)"}
        }}
            }}
        }}
    });

    tools.push_back({
        {"name", "get_sprite_image"},
        {"description", "Get the image of a specific sprite as base64-encoded PNG. Use vdc parameter (1 or 2) for SuperGrafx"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
        {"sprite_index", {
            {"type", "integer"},
            {"description", "Sprite index (0-63)"}
        }},
        {"vdc", {
            {"type", "integer"},
            {"description", "VDC number (1 or 2 for SuperGrafx, default 1)"}
        }}
            }},
            {"required", json::array({"sprite_index"})}
        }}
    });

    // Disassembler tools
    tools.push_back({
        {"name", "debug_run_to_cursor"},
        {"description", "Continue execution until reaching specified address"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"address", {
                    {"type", "string"},
                    {"description", "Hex address (e.g., 'E177')"}
                }}
            }},
            {"required", json::array({"address"})}
        }}
    });

    tools.push_back({
        {"name", "add_disassembler_bookmark"},
        {"description", "Add a bookmark in the disassembler window at specified address"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"address", {
                    {"type", "string"},
                    {"description", "Hex address (e.g., 'E177')"}
                }},
                {"name", {
                    {"type", "string"},
                    {"description", "Bookmark name (optional, auto-generated if not provided)"}
                }}
            }},
            {"required", json::array({"address"})}
        }}
    });

    tools.push_back({
        {"name", "remove_disassembler_bookmark"},
        {"description", "Remove a bookmark from the disassembler window at specified address"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"address", {
                    {"type", "string"},
                    {"description", "Hex address (e.g., 'E177')"}
                }}
            }},
            {"required", json::array({"address"})}
        }}
    });

    tools.push_back({
        {"name", "add_symbol"},
        {"description", "Add a symbol (label) at specified address with bank"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"bank", {
                    {"type", "string"},
                    {"description", "Bank number in hex (e.g., '00')"}
                }},
                {"address", {
                    {"type", "string"},
                    {"description", "Address in hex (e.g., 'E177')"}
                }},
                {"name", {
                    {"type", "string"},
                    {"description", "Symbol name"}
                }}
            }},
            {"required", json::array({"bank", "address", "name"})}
        }}
    });

    tools.push_back({
        {"name", "remove_symbol"},
        {"description", "Remove a symbol from specified address and bank"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"bank", {
                    {"type", "string"},
                    {"description", "Bank number in hex (e.g., '00')"}
                }},
                {"address", {
                    {"type", "string"},
                    {"description", "Address in hex (e.g., 'E177')"}
                }}
            }},
            {"required", json::array({"bank", "address"})}
        }}
    });

    // Memory editor tools
    tools.push_back({
        {"name", "select_memory_range"},
        {"description", "Select a range of memory addresses in a memory area"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"area", {
                    {"type", "integer"},
                    {"description", "Memory area ID (use list_memory_areas to get available areas and their IDs)"}
                }},
                {"start_address", {
                    {"type", "string"},
                    {"description", "Start address in hex (e.g., '2000')"}
                }},
                {"end_address", {
                    {"type", "string"},
                    {"description", "End address in hex (e.g., '20FF')"}
                }}
            }},
            {"required", json::array({"area", "start_address", "end_address"})}
        }}
    });

    tools.push_back({
        {"name", "set_memory_selection_value"},
        {"description", "Set all bytes in current memory selection to specified value"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"area", {
                    {"type", "integer"},
                    {"description", "Memory area ID (use list_memory_areas to get available areas and their IDs)"}
                }},
                {"value", {
                    {"type", "string"},
                    {"description", "Byte value in hex (e.g., 'FF' or '00')"}
                }}
            }},
            {"required", json::array({"area", "value"})}
        }}
    });

    tools.push_back({
        {"name", "add_memory_bookmark"},
        {"description", "Add a bookmark in a memory area at specified address"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"area", {
                    {"type", "integer"},
                    {"description", "Memory area ID (use list_memory_areas to get available areas and their IDs)"}
                }},
                {"address", {
                    {"type", "string"},
                    {"description", "Address in hex (e.g., '2000')"}
                }},
                {"name", {
                    {"type", "string"},
                    {"description", "Bookmark name (optional)"}
                }}
            }},
            {"required", json::array({"area", "address"})}
        }}
    });

    tools.push_back({
        {"name", "remove_memory_bookmark"},
        {"description", "Remove a bookmark from a memory area at specified address"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"area", {
                    {"type", "integer"},
                    {"description", "Memory area ID (use list_memory_areas to get available areas and their IDs)"}
                }},
                {"address", {
                    {"type", "string"},
                    {"description", "Address in hex (e.g., '2000')"}
                }}
            }},
            {"required", json::array({"area", "address"})}
        }}
    });

    tools.push_back({
        {"name", "add_memory_watch"},
        {"description", "Add a watch (tracked memory location) in a memory area"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"area", {
                    {"type", "integer"},
                    {"description", "Memory area ID (use list_memory_areas to get available areas and their IDs)"}
                }},
                {"address", {
                    {"type", "string"},
                    {"description", "Address in hex (e.g., '2000')"}
                }},
                {"notes", {
                    {"type", "string"},
                    {"description", "Watch notes (optional)"}
                }}
            }},
            {"required", json::array({"area", "address"})}
        }}
    });

    tools.push_back({
        {"name", "remove_memory_watch"},
        {"description", "Remove a watch from a memory area at specified address"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"area", {
                    {"type", "integer"},
                    {"description", "Memory area ID (use list_memory_areas to get available areas and their IDs)"}
                }},
                {"address", {
                    {"type", "string"},
                    {"description", "Address in hex (e.g., '2000')"}
                }}
            }},
            {"required", json::array({"area", "address"})}
        }}
    });

    tools.push_back({
        {"name", "list_disassembler_bookmarks"},
        {"description", "List all bookmarks in the disassembler"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "list_symbols"},
        {"description", "List all symbols (labels) defined in the disassembler"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "get_call_stack"},
        {"description", "List the current call stack (function calls hierarchy)"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", json::object()}
        }}
    });

    tools.push_back({
        {"name", "list_memory_bookmarks"},
        {"description", "List all bookmarks in a specific memory area"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"area", {
                    {"type", "integer"},
                    {"description", "Memory area ID (use list_memory_areas to get available areas and their IDs)"}
                }}
            }},
            {"required", json::array({"area"})}
        }}
    });

    tools.push_back({
        {"name", "list_memory_watches"},
        {"description", "List all watches in a specific memory area"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"area", {
                    {"type", "integer"},
                    {"description", "Memory area ID (use list_memory_areas to get available areas and their IDs)"}
                }}
            }},
            {"required", json::array({"area"})}
        }}
    });

    tools.push_back({
        {"name", "get_memory_selection"},
        {"description", "Get the current memory selection range for a specific memory area"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"area", {
                    {"type", "integer"},
                    {"description", "Memory area ID (use list_memory_areas to get available areas and their IDs)"}
                }}
            }},
            {"required", json::array({"area"})}
        }}
    });

    tools.push_back({
        {"name", "memory_search_capture"},
        {"description", "Capture a snapshot of memory for comparison in searches"},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"area", {
                    {"type", "integer"},
                    {"description", "Memory area ID (use list_memory_areas to get available areas and their IDs)"}
                }}
            }},
            {"required", json::array({"area"})}
        }}
    });

    tools.push_back({
        {"name", "memory_search"},
        {"description", "Search memory for values matching criteria. Returns addresses and values found."},
        {"inputSchema", {
            {"type", "object"},
            {"properties", {
                {"area", {
                    {"type", "integer"},
                    {"description", "Memory area ID (use list_memory_areas to get available areas and their IDs)"}
                }},
                {"operator", {
                    {"type", "string"},
                    {"description", "Comparison operator"},
                    {"enum", json::array({"<", ">", "==", "!=", "<=", ">="})}
                }},
                {"compare_type", {
                    {"type", "string"},
                    {"description", "What to compare against: 'previous' (snapshot), 'value' (specific value), or 'address' (value at specific address)"},
                    {"enum", json::array({"previous", "value", "address"})}
                }},
                {"compare_value", {
                    {"type", "integer"},
                    {"description", "Value to compare (for compare_type='value') or address to compare (for compare_type='address')"}
                }},
                {"data_type", {
                    {"type", "string"},
                    {"description", "Data type: 'unsigned' (default), 'signed', 'hex'"},
                    {"enum", json::array({"unsigned", "signed", "hex"})}
                }}
            }},
            {"required", json::array({"area", "operator", "compare_type"})}
        }}
    });

    json response;
    response["jsonrpc"] = "2.0";
    response["id"] = id;
    response["result"] = {
        {"tools", tools}
    };

    SendResponse(response);
}

void McpServer::HandleToolsCall(const json& request)
{
    if (!request.contains("id"))
    {
        SendError(0, -32600, "Invalid Request: missing id");
        return;
    }

    int64_t id = request["id"];

    if (!request.contains("params") || !request["params"].contains("name"))
    {
        SendError(id, -32602, "Invalid params: missing tool name");
        return;
    }

    std::string toolName = request["params"]["name"];
    json arguments = request["params"].contains("arguments") ? request["params"]["arguments"] : json::object();

    // Enqueue command for main thread to execute
    DebugCommand* cmd = new DebugCommand();
    cmd->requestId = id;
    cmd->toolName = toolName;
    cmd->arguments = arguments;
    m_commandQueue.Push(cmd);
}

static int GetBreakpointTypeFromString(const std::string& memory_area)
{
    if (memory_area == "rom_ram") return HuC6280::HuC6280_BREAKPOINT_TYPE_ROMRAM;
    if (memory_area == "vram") return HuC6280::HuC6280_BREAKPOINT_TYPE_VRAM;
    if (memory_area == "palette") return HuC6280::HuC6280_BREAKPOINT_TYPE_PALETTE_RAM;
    if (memory_area == "huc6270_reg") return HuC6280::HuC6280_BREAKPOINT_TYPE_HUC6270_REGISTER;
    if (memory_area == "huc6260_reg") return HuC6280::HuC6280_BREAKPOINT_TYPE_HUC6260_REGISTER;
    return HuC6280::HuC6280_BREAKPOINT_TYPE_ROMRAM; // default
}

json McpServer::ExecuteCommand(const std::string& toolName, const json& arguments)
{
    // Normalize tool name: VS Code converts underscores to dots
    std::string normalizedTool = toolName;
    size_t pos = 0;
    while ((pos = normalizedTool.find('.', pos)) != std::string::npos) {
        normalizedTool[pos] = '_';
        pos++;
    }

    // Execution control
    if (normalizedTool == "debug_pause")
    {
        m_debugAdapter.Pause();
        return {{"success", true}};
    }
    else if (normalizedTool == "debug_continue")
    {
        m_debugAdapter.Resume();
        return {{"success", true}};
    }
    else if (normalizedTool == "debug_step_into")
    {
        m_debugAdapter.StepInto();
        return {{"success", true}};
    }
    else if (normalizedTool == "debug_step_over")
    {
        m_debugAdapter.StepOver();
        return {{"success", true}};
    }
    else if (normalizedTool == "debug_step_out")
    {
        m_debugAdapter.StepOut();
        return {{"success", true}};
    }
    else if (normalizedTool == "debug_step_frame")
    {
        m_debugAdapter.StepFrame();
        return {{"success", true}};
    }
    else if (normalizedTool == "debug_reset")
    {
        m_debugAdapter.Reset();
        return {{"success", true}};
    }
    else if (normalizedTool == "debug_get_status")
    {
        return m_debugAdapter.GetDebugStatus();
    }
    // Breakpoints
    else if (normalizedTool == "set_breakpoint")
    {
        std::string addrStr = arguments["address"];
        u16 address;
        if (!parse_hex_with_prefix(addrStr, &address))
            return {{"error", "Invalid address format"}};

        std::string memory_area = arguments.value("memory_area", "rom_ram");
        int breakpoint_type = GetBreakpointTypeFromString(memory_area);

        std::string type = arguments.value("type", "exec");
        bool read = (type == "read");
        bool write = (type == "write");
        bool execute = (type == "exec");

        m_debugAdapter.SetBreakpoint(address, breakpoint_type, read, write, execute);
        return {{"success", true}, {"address", addrStr}, {"memory_area", memory_area}};
    }
    else if (normalizedTool == "set_breakpoint_range")
    {
        std::string startAddrStr = arguments["start_address"];
        std::string endAddrStr = arguments["end_address"];
        u16 start_address, end_address;

        if (!parse_hex_with_prefix(startAddrStr, &start_address))
            return {{"error", "Invalid start_address format"}};
        if (!parse_hex_with_prefix(endAddrStr, &end_address))
            return {{"error", "Invalid end_address format"}};
        if (start_address > end_address)
            return {{"error", "start_address must be <= end_address"}};

        std::string memory_area = arguments.value("memory_area", "rom_ram");
        int breakpoint_type = GetBreakpointTypeFromString(memory_area);

        std::string type = arguments.value("type", "exec");
        bool read = (type == "read");
        bool write = (type == "write");
        bool execute = (type == "exec");

        m_debugAdapter.SetBreakpointRange(start_address, end_address, breakpoint_type,
                                         read, write, execute);
        return {{"success", true}, {"start_address", startAddrStr}, {"end_address", endAddrStr}, {"memory_area", memory_area}};
    }
    else if (normalizedTool == "remove_breakpoint")
    {
        std::string addrStr = arguments["address"];
        u16 address;
        if (!parse_hex_with_prefix(addrStr, &address))
            return {{"error", "Invalid address format"}};

        std::string memory_area = arguments.value("memory_area", "rom_ram");
        int breakpoint_type = GetBreakpointTypeFromString(memory_area);

        // Check if end_address is provided for range breakpoints
        u16 end_address = 0;
        if (arguments.contains("end_address"))
        {
            std::string endAddrStr = arguments["end_address"];
            if (!parse_hex_with_prefix(endAddrStr, &end_address))
                return {{"error", "Invalid end_address format"}};
        }

        m_debugAdapter.ClearBreakpointByAddress(address, breakpoint_type, end_address);
        return {{"success", true}, {"address", addrStr}, {"memory_area", memory_area}};
    }
    else if (normalizedTool == "list_breakpoints")
    {
        std::vector<BreakpointInfo> breakpoints = m_debugAdapter.ListBreakpoints();
        json bpArray = json::array();
        for (const BreakpointInfo& bp : breakpoints)
        {
            json bpObj;
            bpObj["enabled"] = bp.enabled;
            bpObj["type"] = bp.type_name;

            std::ostringstream addr_ss;
            addr_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << bp.address1;
            bpObj["address"] = addr_ss.str();

            if (bp.range)
            {
                std::ostringstream addr2_ss;
                addr2_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << bp.address2;
                bpObj["address2"] = addr2_ss.str();
            }

            bpObj["read"] = bp.read;
            bpObj["write"] = bp.write;
            bpObj["execute"] = bp.execute;
            bpArray.push_back(bpObj);
        }
        return {{"breakpoints", bpArray}};
    }
    // Memory
    else if (normalizedTool == "list_memory_areas")
    {
        std::vector<MemoryAreaInfo> areas = m_debugAdapter.ListMemoryAreas();
        json areaArray = json::array();
        for (const MemoryAreaInfo& area : areas)
        {
            json areaObj;
            areaObj["id"] = area.id;
            areaObj["name"] = area.name;
            areaObj["size"] = area.size;
            areaArray.push_back(areaObj);
        }
        return {{"areas", areaArray}};
    }
    else if (normalizedTool == "read_memory")
    {
        int area = arguments["area"];
        std::string offsetStr = arguments["offset"];
        u32 offset;
        if (!parse_hex_with_prefix(offsetStr, &offset))
            return {{"error", "Invalid offset format"}};

        size_t size = arguments["size"];
        std::vector<u8> data = m_debugAdapter.ReadMemoryArea(area, offset, size);

        std::ostringstream hex_ss;
        for (size_t i = 0; i < data.size(); i++)
        {
            hex_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)data[i];
            if (i < data.size() - 1)
                hex_ss << " ";
        }

        return {{"area", area}, {"offset", offsetStr}, {"data", hex_ss.str()}};
    }
    else if (normalizedTool == "write_memory")
    {
        int area = arguments["area"];
        std::string offsetStr = arguments["offset"];
        u32 offset;
        if (!parse_hex_with_prefix(offsetStr, &offset))
            return {{"error", "Invalid offset format"}};

        std::string bytesStr = arguments["bytes"];
        std::vector<u8> data;

        // Parse hex bytes
        std::istringstream iss(bytesStr);
        std::string byteStr;
        while (iss >> byteStr)
        {
            u8 byte;
            if (!parse_hex_with_prefix(byteStr, &byte))
                return {{"error", "Invalid byte format"}};
            data.push_back(byte);
        }

        m_debugAdapter.WriteMemoryArea(area, offset, data);
        return {{"success", true}, {"area", area}, {"offset", offsetStr}, {"bytes_written", data.size()}};
    }
    // Registers
    else if (normalizedTool == "write_huc6280_register")
    {
        std::string name = arguments["name"];
        std::string valueStr = arguments["value"];
        u32 value;
        if (!parse_hex_with_prefix(valueStr, &value))
            return {{"error", "Invalid value format"}};

        m_debugAdapter.SetRegister(name, value);
        return {{"success", true}, {"register", name}, {"value", valueStr}};
    }
    // Disassembly
    else if (normalizedTool == "debug_get_disassembly")
    {
        size_t offset = arguments.value("offset", 15);

        std::vector<DisasmLine> lines;

        if (arguments.contains("start"))
        {
            std::string addrStr = arguments["start"];
            u16 address;
            if (!parse_hex_with_prefix(addrStr, &address))
                return {{"error", "Invalid start address format"}};

            lines = m_debugAdapter.GetDisassemblyRange(address, offset);
        }
        else
        {
            lines = m_debugAdapter.GetDisassemblyAroundPc(0, offset);
        }

        std::ostringstream disasm_ss;
        for (const DisasmLine& line : lines)
        {
            disasm_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(6)
                     << line.address << "-" << std::setw(2) << (int)line.bank << ": "
                     << line.segment << "  " << line.name << "  ; " << line.bytes;
            if (line.jump)
            {
                disasm_ss << "  [jump to " << std::hex << std::setw(4) << line.jump_address << "]";
            }
            disasm_ss << "\n";
        }

        return {{"disassembly", disasm_ss.str()}};
    }
    // Media info
    else if (normalizedTool == "get_media_info")
    {
        return m_debugAdapter.GetMediaInfo();
    }
    // Chip status
    else if (normalizedTool == "get_huc6280_status")
    {
        return m_debugAdapter.GetHuC6280Status();
    }
    else if (normalizedTool == "get_huc6270_registers")
    {
        int vdc = arguments.value("vdc", 1);
        return m_debugAdapter.GetHuC6270Registers(vdc);
    }
    else if (normalizedTool == "get_huc6270_status")
    {
        int vdc = arguments.value("vdc", 1);
        return m_debugAdapter.GetHuC6270Status(vdc);
    }
    else if (normalizedTool == "get_huc6260_status")
    {
        return m_debugAdapter.GetHuC6260Status();
    }
    else if (normalizedTool == "get_huc6202_status")
    {
        return m_debugAdapter.GetHuC6202Status();
    }
    else if (normalizedTool == "get_psg_status")
    {
        return m_debugAdapter.GetPSGStatus();
    }
    else if (normalizedTool == "get_cdrom_status")
    {
        return m_debugAdapter.GetCDROMStatus();
    }
    else if (normalizedTool == "get_arcade_card_status")
    {
        return m_debugAdapter.GetArcadeCardStatus();
    }
    else if (normalizedTool == "get_cdrom_audio_status")
    {
        return m_debugAdapter.GetCDROMAudioStatus();
    }
    else if (normalizedTool == "get_adpcm_status")
    {
        return m_debugAdapter.GetADPCMStatus();
    }
    else if (normalizedTool == "get_screenshot")
    {
        return m_debugAdapter.GetScreenshot();
    }
    else if (normalizedTool == "list_sprites")
    {
        int vdc = arguments.value("vdc", 1);
        return m_debugAdapter.ListSprites(vdc);
    }
    else if (normalizedTool == "get_sprite_image")
    {
        int sprite_index = arguments.value("sprite_index", 0);
        int vdc = arguments.value("vdc", 1);
        return m_debugAdapter.GetSpriteImage(sprite_index, vdc);
    }
    // Disassembler operations
    else if (normalizedTool == "debug_run_to_cursor")
    {
        std::string addrStr = arguments["address"];
        u16 address;
        if (!parse_hex_with_prefix(addrStr, &address))
            return {{"error", "Invalid address format"}};
        return m_debugAdapter.RunToAddress(address);
    }
    else if (normalizedTool == "add_disassembler_bookmark")
    {
        std::string addrStr = arguments["address"];
        u16 address;
        if (!parse_hex_with_prefix(addrStr, &address))
            return {{"error", "Invalid address format"}};
        std::string name = arguments.value("name", "");
        return m_debugAdapter.AddDisassemblerBookmark(address, name);
    }
    else if (normalizedTool == "remove_disassembler_bookmark")
    {
        std::string addrStr = arguments["address"];
        u16 address;
        if (!parse_hex_with_prefix(addrStr, &address))
            return {{"error", "Invalid address format"}};
        return m_debugAdapter.RemoveDisassemblerBookmark(address);
    }
    else if (normalizedTool == "add_symbol")
    {
        std::string bankStr = arguments["bank"];
        std::string addrStr = arguments["address"];
        std::string name = arguments["name"];
        u8 bank;
        u16 address;
        if (!parse_hex_with_prefix(bankStr, &bank))
            return {{"error", "Invalid bank format"}};
        if (!parse_hex_with_prefix(addrStr, &address))
            return {{"error", "Invalid address format"}};
        return m_debugAdapter.AddSymbol(bank, address, name);
    }
    else if (normalizedTool == "remove_symbol")
    {
        std::string bankStr = arguments["bank"];
        std::string addrStr = arguments["address"];
        u8 bank;
        u16 address;
        if (!parse_hex_with_prefix(bankStr, &bank))
            return {{"error", "Invalid bank format"}};
        if (!parse_hex_with_prefix(addrStr, &address))
            return {{"error", "Invalid address format"}};
        return m_debugAdapter.RemoveSymbol(bank, address);
    }
    // Memory editor operations
    else if (normalizedTool == "select_memory_range")
    {
        int editor = arguments["area"];
        std::string startStr = arguments["start_address"];
        std::string endStr = arguments["end_address"];
        u32 start_address, end_address;
        if (!parse_hex_with_prefix(startStr, &start_address))
            return {{"error", "Invalid start_address format"}};
        if (!parse_hex_with_prefix(endStr, &end_address))
            return {{"error", "Invalid end_address format"}};
        return m_debugAdapter.SelectMemoryRange(editor, start_address, end_address);
    }
    else if (normalizedTool == "set_memory_selection_value")
    {
        int editor = arguments["area"];
        std::string valueStr = arguments["value"];
        u8 value;
        if (!parse_hex_with_prefix(valueStr, &value))
            return {{"error", "Invalid value format"}};
        return m_debugAdapter.SetMemorySelectionValue(editor, value);
    }
    else if (normalizedTool == "add_memory_bookmark")
    {
        int editor = arguments["area"];
        std::string addrStr = arguments["address"];
        std::string name = arguments.value("name", "");
        u32 address;
        if (!parse_hex_with_prefix(addrStr, &address))
            return {{"error", "Invalid address format"}};
        return m_debugAdapter.AddMemoryBookmark(editor, address, name);
    }
    else if (normalizedTool == "remove_memory_bookmark")
    {
        int editor = arguments["area"];
        std::string addrStr = arguments["address"];
        u32 address;
        if (!parse_hex_with_prefix(addrStr, &address))
            return {{"error", "Invalid address format"}};
        return m_debugAdapter.RemoveMemoryBookmark(editor, address);
    }
    else if (normalizedTool == "add_memory_watch")
    {
        int editor = arguments["area"];
        std::string addrStr = arguments["address"];
        std::string notes = arguments.value("notes", "");
        u32 address;
        if (!parse_hex_with_prefix(addrStr, &address))
            return {{"error", "Invalid address format"}};
        return m_debugAdapter.AddMemoryWatch(editor, address, notes);
    }
    else if (normalizedTool == "remove_memory_watch")
    {
        int editor = arguments["area"];
        std::string addrStr = arguments["address"];
        u32 address;
        if (!parse_hex_with_prefix(addrStr, &address))
            return {{"error", "Invalid address format"}};
        return m_debugAdapter.RemoveMemoryWatch(editor, address);
    }
    else if (normalizedTool == "list_disassembler_bookmarks")
    {
        return m_debugAdapter.ListDisassemblerBookmarks();
    }
    else if (normalizedTool == "list_symbols")
    {
        return m_debugAdapter.ListSymbols();
    }
    else if (normalizedTool == "get_call_stack")
    {
        return m_debugAdapter.ListCallStack();
    }
    else if (normalizedTool == "list_memory_bookmarks")
    {
        int area = arguments["area"];
        return m_debugAdapter.ListMemoryBookmarks(area);
    }
    else if (normalizedTool == "list_memory_watches")
    {
        int area = arguments["area"];
        return m_debugAdapter.ListMemoryWatches(area);
    }
    else if (normalizedTool == "get_memory_selection")
    {
        int area = arguments["area"];
        return m_debugAdapter.GetMemorySelection(area);
    }
    else if (normalizedTool == "memory_search_capture")
    {
        int area = arguments["area"];
        return m_debugAdapter.MemorySearchCapture(area);
    }
    else if (normalizedTool == "memory_search")
    {
        int area = arguments["area"];
        std::string op = arguments["operator"];
        std::string compare_type = arguments["compare_type"];
        int compare_value = arguments.value("compare_value", 0);
        std::string data_type = arguments.value("data_type", "unsigned");
        return m_debugAdapter.MemorySearch(area, op, compare_type, compare_value, data_type);
    }
    else
    {
        return {{"error", "Unknown tool: " + toolName}};
    }
}

void McpServer::SendResponse(const json& response)
{
    std::string line = response.dump();
    m_transport->send(line);
}

void McpServer::SendError(int64_t id, int code, const std::string& message, const json& data)
{
    json error;
    error["jsonrpc"] = "2.0";
    error["id"] = id;
    error["error"] = {
        {"code", code},
        {"message", message}
    };

    if (!data.empty() && !data.is_null())
    {
        error["error"]["data"] = data;
    }

    SendResponse(error);
}


