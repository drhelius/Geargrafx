# Geargrafx MCP Server

A [Model Context Protocol](https://modelcontextprotocol.io/introduction) (MCP) server for the Geargrafx emulator, enabling AI-assisted debugging and development of TurboGrafx-16 / PC Engine / SuperGrafx games.

This server provides comprehensive tools for game development, rom hacking, reverse engineering, and debugging through standardized MCP protocols compatible with AI agents like GitHub Copilot, Claude Desktop, ChatGPT and others.

## Features

- **Full Debugger Access**: CPU registers, memory inspection, breakpoints, and execution control
- **Multiple Memory Areas**: Access RAM, VRAM, ROM, CD-ROM RAM, Arcade Card RAM, and more
- **Disassembly**: View disassembled code around PC or any address
- **Hardware Inspection**: HuC6280 CPU, HuC6270 VDC, HuC6260 VCE, HuC6202 VPC, PSG, CD-ROM subsystems
- **Sprite Viewer**: List and inspect all 64 sprites with images
- **Symbol Support**: Add, remove, and list debug symbols
- **Bookmarks**: Memory and disassembler bookmarks for navigation
- **Call Stack**: View function call hierarchy
- **Screenshot Capture**: Get current frame as PNG image
- **Documentation Resources**: Built-in hardware and programming documentation for AI context
- **GUI Integration**: MCP server runs alongside the emulator GUI, sharing the same state

## Available MCP Tools

The server exposes tools organized in the following categories:

### Execution Control
- `debug_pause` - Pause emulation
- `debug_continue` - Resume emulation  
- `debug_step_into` - Step one instruction
- `debug_step_over` - Step over subroutine calls
- `debug_step_out` - Step out of current subroutine
- `debug_step_frame` - Step one frame
- `debug_run_to_cursor` - Continue execution until reaching specified address
- `debug_reset` - Reset emulation
- `debug_get_status` - Get debug status (paused, at_breakpoint, pc address)

### CPU & Registers
- `write_huc6280_register` - Set register value
- `get_huc6280_status` - Get complete HuC6280 CPU status (registers, MPR, timer, interrupts, I/O, speed)

### Memory Operations
- `list_memory_areas` - List all available memory areas
- `read_memory` - Read from specific memory area
- `write_memory` - Write to specific memory area
- `get_memory_selection` - Get current memory selection range
- `select_memory_range` - Select a range of memory addresses
- `set_memory_selection_value` - Set all bytes in selection to specified value
- `add_memory_bookmark` - Add bookmark in memory area
- `remove_memory_bookmark` - Remove memory bookmark
- `list_memory_bookmarks` - List all bookmarks in memory area
- `add_memory_watch` - Add watch (tracked memory location)
- `remove_memory_watch` - Remove memory watch
- `list_memory_watches` - List all watches in memory area
- `memory_search_capture` - Capture memory snapshot for search comparison
- `memory_search` - Search memory with operators (<, >, ==, !=, <=, >=), compare types (previous, value, address), and data types (hex, signed, unsigned)

### Disassembly & Debugging
- `get_disassembly` - Get disassembly around PC or specified address
- `add_symbol` - Add symbol (label) at specified address
- `remove_symbol` - Remove symbol
- `list_symbols` - List all defined symbols
- `add_disassembler_bookmark` - Add bookmark in disassembler
- `remove_disassembler_bookmark` - Remove disassembler bookmark
- `list_disassembler_bookmarks` - List all disassembler bookmarks
- `get_call_stack` - View function call hierarchy

### Breakpoints
- `set_breakpoint` - Set execution, read, or write breakpoint (supports 5 memory areas: rom_ram, vram, palette, huc6270_reg, huc6260_reg)
- `set_breakpoint_range` - Set breakpoint for an address range (supports 5 memory areas)
- `remove_breakpoint` - Remove breakpoint
- `list_breakpoints` - List all breakpoints

### Hardware Status
- `get_huc6270_status` - Get VDC status (position, state, control, interrupts)
- `get_huc6270_registers` - Get all 20 VDC registers (0x00-0x13), Address Register (AR), and Status Register (SR)
- `write_huc6270_register` - Write to a VDC register (0-19) or Address Register (20=AR). Status Register is read-only. Use vdc parameter (1 or 2) for SuperGrafx
- `get_huc6260_status` - Get VCE status (position, sync signals, control)
- `get_huc6202_status` - Get VPC status (SuperGrafx only)
- `get_psg_status` - Get PSG status for all 6 channels
- `get_cdrom_status` - Get CD-ROM drive status (CD games only)
- `get_cdrom_audio_status` - Get CD-ROM audio playback status
- `get_adpcm_status` - Get ADPCM audio status
- `get_arcade_card_status` - Get Arcade Card status

### Sprites
- `list_sprites` - List all 64 sprites with position, size, pattern, palette
- `get_sprite_image` - Get sprite image as base64 PNG

### Screen Capture
- `get_screenshot` - Capture current screen frame as base64 PNG

### Media & State Management
- `get_media_info` - Get loaded ROM/CD info
- `load_media` - Load ROM file or CD-ROM image (.pce, .sgx, .hes, .cue, .zip). Automatically loads .sym symbol file if present
- `load_symbols` - Load debug symbols from file (.sym format with 'BANK:ADDRESS LABEL' entries)
- `list_save_state_slots` - List all 5 save state slots with information (rom name, timestamp, validity)
- `select_save_state_slot` - Select active save state slot (1-5) for save/load operations
- `save_state` - Save emulator state to currently selected slot
- `load_state` - Load emulator state from currently selected slot
- `set_fast_forward_speed` - Set fast forward speed multiplier (0: 1.5x, 1: 2x, 2: 2.5x, 3: 3x, 4: Unlimited)
- `toggle_fast_forward` - Toggle fast forward mode on/off

### Controller Input
- `controller_press_button` - Press a button on a controller (player 1-5). Buttons: up, down, left, right, select, run, i, ii, iii, iv, v, vi
- `controller_release_button` - Release a button on a controller (player 1-5)
- `controller_set_type` - Set controller type for a player: standard (2 buttons), avenue_pad_3 (3 buttons), avenue_pad_6 (6 buttons)
- `controller_get_type` - Get the current controller type for a player (returns: standard, avenue_pad_3, or avenue_pad_6)
- `controller_set_turbo_tap` - Enable or disable Turbo Tap (multitap) for 5-player support

## Transport Modes

The Geargrafx MCP server supports two transport modes:

### STDIO Transport (Recommended)

The default mode uses standard input/output for communication. The emulator is launched by the AI client and communicates through stdin/stdout pipes.

**Advantages:**
- Automatic lifecycle management (client starts/stops the emulator)
- No network configuration needed
- More secure (no open ports)

**Use cases:**
- VS Code GitHub Copilot
- Claude Desktop
- Any MCP client with process management

### HTTP Transport

The HTTP transport mode runs the emulator with an embedded web server on `localhost:7777/mcp`. The emulator stays running independently while the AI client connects via HTTP.

**Advantages:**
- Persistent emulator instance
- Can connect/disconnect AI clients without restarting
- Manual emulator control while debugging with AI

**Use cases:**
- Advanced debugging workflows
- Multiple AI client connections
- Manual emulator testing with AI assistance

## Quick Start

### STDIO Mode with VS Code

1. **Install [GitHub Copilot extension](https://code.visualstudio.com/docs/copilot/overview)** in VS Code

2. **Configure VS Code settings**:

   Add to your workspace folder a file named `.vscode/mcp.json` with:

   ```json
   {
     "servers": {
       "geargrafx": {
         "command": "/path/to/geargrafx",
         "args": ["--mcp-stdio"]
       }
     }
   }
   ```

   **Important:** Update the `command` path to match your build location:
   - **macOS:** `/path/to/geargrafx`
   - **Linux:** `/path/to/geargrafx`
   - **Windows:** `C:/path/to/geargrafx.exe`

3. **Restart VS Code** for settings to take effect

4. **Open GitHub Copilot Chat** and start debugging:
   - The emulator will auto-start with MCP server enabled
   - Load a game ROM
   - Start chatting with Copilot about the game state

### STDIO Mode with Claude Desktop

#### Option 1: Desktop Extension (Recommended)

The easiest way to install Geargrafx MCP server on Claude Desktop is using the MCPB package:

1. **Download the latest MCPB package** for your platform from the [releases page](https://github.com/drhelius/geargrafx/releases):
   - **macOS Intel:** `geargrafx-macos-x64.mcpb`
   - **macOS Apple Silicon:** `geargrafx-macos-arm64.mcpb`
   - **Windows Intel/AMD:** `geargrafx-windows-x64.mcpb`
   - **Windows ARM:** `geargrafx-windows-arm64.mcpb`
   - **Linux Intel/AMD:** `geargrafx-linux-x64.mcpb`

2. **Install the extension**:
   - Open Claude Desktop
   - Navigate to **Settings > Extensions**
   - Click **Advanced settings**
   - In the Extension Developer section, click **Install Extension…**
   - Select the downloaded `.mcpb` file

3. **Start debugging**: The extension is now available in your conversations. The emulator will automatically launch when Claude needs to access debugging tools.

#### Option 2: Manual Configuration

If you prefer to build from source or configure manually:

1. **Edit Claude Desktop config file**:

   Follow [these instructions](https://modelcontextprotocol.io/quickstart/user#for-claude-desktop-users) to access Claude's config file, then edit it to include:

   ```json
   {
     "mcpServers": {
       "geargrafx": {
         "command": "/path/to/geargrafx/platforms/macos/geargrafx",
         "args": ["--mcp-stdio"]
       }
     }
   }
   ```

   **Config file locations:**
   - **macOS:** `~/Library/Application Support/Claude/claude_desktop_config.json`
   - **Windows:** `%APPDATA%\Claude\claude_desktop_config.json`
   - **Linux:** `~/.config/Claude/claude_desktop_config.json`

   **Important:** Update the `command` path to match your build location.

2. **Restart Claude Desktop**

3. **Start debugging**: The emulator will launch when Claude needs to access debugging tools

### HTTP Mode

1. **Start the emulator manually** with HTTP transport:
   ```bash
   ./geargrafx --mcp-http
   # Server will start on http://localhost:7777/mcp
   
   # Or specify a custom port:
   ./geargrafx --mcp-http --mcp-http-port 3000
   # Server will start on http://localhost:3000/mcp
   ```

   You can optionally start the server using the "MCP" menu in the GUI.

2. **Configure VS Code** `.vscode/mcp.json`:
   ```json
   {
     "servers": {
       "geargrafx": {
         "type": "http",
         "url": "http://localhost:7777/mcp",
         "headers": {}
       }
     }
   }
   ```

   Or User Settings JSON:
   ```json
   {
     "github.copilot.chat.mcp.servers": {
       "geargrafx": {
         "type": "http",
         "url": "http://localhost:7777/mcp"
       }
     }
   }
   ```

3. **Or configure Claude Desktop**:
   ```json
   {
     "mcpServers": {
       "geargrafx": {
         "type": "http",
         "url": "http://localhost:7777/mcp"
       }
     }
   }
   ```

4. **Restart your AI client** and start debugging

> **Note:** The MCP HTTP Server must be running standalone before connecting the AI client. Default port is 7777 (configurable with `--mcp-http-port`). In HTTP mode, the emulator stays running even when the AI client disconnects. You can manually load games and interact with the GUI while the AI assistant connects as needed.

## Usage Examples

Once configured, you can ask your AI assistant:

### Basic Commands

- "What game is currently loaded?"
- "Load the ROM at /path/to/game.pce"
- "Show me the current CPU registers"
- "Read 16 bytes from RAM starting at 0x2000"
- "Set a breakpoint at address 0x8000"
- "Pause execution and show me all sprites"
- "Step through the next 5 instructions"
- "Capture a screenshot of the current frame"
- "Press the up button on player 1 controller"
- "Set player 1 controller to avenue pad 6 type"

### Advanced Debugging Workflows

- "Find the VBlank interrupt handler, analyze what it does, and add symbols for all the subroutines it calls"

- "Locate the sprite update routine. Study how this game manages its sprite system, explain the algorithm, and add bookmarks to key sections. Also add watches for any sprite-related variables you find"

- "There's a data decompression routine around address 0xC000. Step through it instruction by instruction, reverse engineer the compression algorithm, and explain how it works with examples"

- "Find where the game stores its level data in ROM. Analyze the data structure format, create a memory map showing each section, and add symbols for the data tables"

- "The game is rendering corrupted graphics. Examine the VDC registers, check the VRAM contents, inspect the sprite attribute table, and diagnose what's causing the corruption. Set up watches on relevant memory addresses"

## Available Resources

In addition to tools, the MCP server provides documentation resources that AI assistants can access to better understand the PC Engine / TurboGrafx-16 hardware and programming.

Resources are organized into categories and are automatically loaded when the server starts. They provide context and reference material for AI-assisted debugging and development.

### Hardware Documentation Resources

Complete technical reference documentation for all PC Engine / TurboGrafx-16 hardware components:

- **HuC6280 CPU — 8-bit CMOS Microprocessor** (`geargrafx://hardware/huc6280_cpu`)
  - 65C02-compatible core architecture
  - CPU registers (A/X/Y/S/P/PC) and status flags
  - Memory Management Unit (MMU) with 8×8KB page mapping
  - Interrupt controller, 7-bit timer, I/O ports
  - Clock speeds (1.79MHz/7.16MHz switchable)

- **HuC6280 Instruction Set Reference** (`geargrafx://hardware/huc6280_instructions`)
  - Complete instruction set with all 65C02 base opcodes
  - HuC6280 extensions (BBR/BBS/RMB/SMB/TST/TAM/TMA/SAX/SAY/SXY/ST0/ST1/ST2/CSL/CSH/CLA/CLX/CLY/TAI/TDD/TIA/TII/TIN)
  - Addressing modes, cycle counts, flags affected

- **HuC6280 PSG — Programmable Sound Generator** (`geargrafx://hardware/huc6280_psg`)
  - 6 waveform channels with 32-sample waveform memory
  - Dual noise generators (channels 5 and 6)
  - Low Frequency Oscillator (LFO)
  - Stereo balance control and Direct D/A mode (DDA)

- **HuC6270 VDC — Video Display Controller** (`geargrafx://hardware/huc6270_vdc`)
  - 64KB VRAM with internal registers (R00-R13)
  - Display timing control and sprite rendering
  - Sprite Attribute Table (SAT) and DMA operations
  - Dual-VDC SuperGrafx support

- **HuC6260 VCE — Video Color Encoder** (`geargrafx://hardware/huc6260_vce`)
  - 512×9-bit color RAM (3-bit R/G/B per entry)
  - Palette organization (16 palettes × 16 colors)
  - Color table access and analog video output

- **HuC6202 VPC — Video Priority Controller** (`geargrafx://hardware/huc6202_vpc`)
  - SuperGrafx dual-VDC (VDC#1/VDC#2) arbitration
  - Priority control registers and window-based layer visibility
  - Per-pixel priority resolution

- **PC Engine Memory Map** (`geargrafx://hardware/memory_map`)
  - 64KB logical / 2MB physical address space
  - MMU 8KB page translation (MPR0-MPR7)
  - Hardware register layout (VDC/$0000, VCE/$0400, PSG/$0800, Timer/$0C00, Input/$1000, IRQ/$1400)
  - RAM configuration, ROM banking, CD-ROM/Arcade Card extensions

### How Resources Work

Resources are read-only documentation that AI assistants can reference during conversations:

1. **Automatic Loading**: Resources are loaded when the MCP server starts
2. **Context Enhancement**: AI assistants can retrieve resources to understand hardware specifics
3. **Format**: Resources are provided in Markdown format for easy reading
4. **URIs**: Each resource has a unique URI like `geargrafx://category/resource_id`

### Accessing Resources

Resources are accessed through the MCP protocol's resource capabilities:

- `resources/list` - List all available resources
- `resources/read` - Read the content of a specific resource

AI clients automatically handle resource retrieval, so you typically don't need to manually request them. The AI assistant will fetch relevant resources when needed to answer your questions or provide better debugging assistance.

## How It Works

- The MCP server runs **alongside** the GUI in a background thread
- The emulator GUI remains fully functional (you can play/debug normally)
- Commands from the AI are queued and executed on the main thread
- Both GUI and MCP share the same emulator state
- Changes made through MCP are instantly reflected in the GUI and vice versa

## Architecture

### STDIO Transport
```
┌─────────────────┐                    ┌──────────────────┐
│   VS Code /     │       stdio        │    Geargrafx     │
│ Claude Desktop  │◄──────────────────►│   MCP Server     │
│   (AI Client)   │       pipes        │  (Background)    │
└─────────────────┘                    └──────────────────┘
        │                                       │
        └───► Launches ►────────────────────────┘
                                                │
                                                │ Shared State
                                                ▼
                                       ┌──────────────────┐
                                       │   Emulator Core  │
                                       │   + GUI Window   │
                                       └──────────────────┘
```

### HTTP Transport
```
┌─────────────────┐                    ┌──────────────────┐
│   VS Code /     │  HTTP (port 7777)  │    Geargrafx     │
│ Claude Desktop  │◄──────────────────►│ MCP HTTP Server  │
│   (AI Client)   │                    │   (listener)     │
└─────────────────┘                    └──────────────────┘
                                                │
                                                │ Shared State
                                                ▼
                                       ┌──────────────────┐
                                       │   Emulator Core  │
                                       │   + GUI Window   │
                                       └──────────────────┘
```
