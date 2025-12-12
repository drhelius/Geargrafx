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
- **GUI Integration**: MCP server runs alongside the emulator GUI, sharing the same state

## Available MCP Tools

The server exposes 50 tools organized in the following categories:

### Execution Control
- `debug_pause` - Pause emulation
- `debug_continue` - Resume emulation  
- `debug_step_into` - Step one instruction
- `debug_step_over` - Step over subroutine calls
- `debug_step_out` - Step out of current subroutine
- `debug_step_frame` - Step one frame
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
- `debug_get_disassembly` - Get disassembly around PC or specified address
- `debug_run_to_cursor` - Continue execution until reaching specified address
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
- `get_media_info` - Get loaded ROM/CD info
- `get_huc6270_status` - Get VDC status (position, state, control, interrupts)
- `get_huc6270_registers` - Get all 32 VDC registers
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

The HTTP transport mode runs the emulator with an embedded web server on `localhost:7777`. The emulator stays running independently while the AI client connects via HTTP.

**Advantages:**
- Persistent emulator instance
- Can connect/disconnect AI clients without restarting
- Manual emulator control while debugging with AI

**Use cases:**
- Advanced debugging workflows
- Multiple AI client connections
- Manual emulator testing with AI assistance

## Quick Start

### STDIO Mode with VS Code (Recommended)

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

3. **Restart Claude Desktop**

4. **Start debugging**: The emulator will launch when Claude needs to access debugging tools

### HTTP Mode (Advanced)

1. **Start the emulator manually** with HTTP transport:
   ```bash
   ./geargrafx --mcp-http
   # Server will start on http://localhost:7777
   
   # Or specify a custom port:
   ./geargrafx --mcp-http --mcp-http-port 3000
   ```

   You can optionally start the server using the "MCP" menu in the GUI.

2. **Configure VS Code** `.vscode/mcp.json`:
   ```json
   {
     "servers": {
       "geargrafx": {
         "type": "http",
         "url": "http://localhost:7777",
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
         "url": "http://localhost:7777"
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
         "url": "http://localhost:7777"
       }
     }
   }
   ```

4. **Restart your AI client** and start debugging

> **Note:** The MCP HTTP Server must be running standalone before connecting the AI client. Default port is 7777 (configurable with `--mcp-http-port`). In HTTP mode, the emulator stays running even when the AI client disconnects. You can manually load games and interact with the GUI while the AI assistant connects as needed.

## Usage Examples

Once configured, you can ask your AI assistant:

- "What game is currently loaded?"
- "Is the emulator running or paused?"
- "Show me the current CPU registers"
- "What's the value of PC?"
- "Read 16 bytes from RAM starting at 0x2000"
- "List all available memory areas"
- "Show me the disassembly around the current PC"
- "Set a breakpoint at address 0x8000"
- "Pause execution and show me all sprites"
- "What's in the call stack?"
- "Step through the next 5 instructions"
- "Show me the VDC registers"
- "Capture a screenshot of the current frame"

## How It Works

- The MCP server runs **alongside** the GUI in a background thread
- The emulator GUI remains fully functional (you can play/debug normally)
- Commands from the AI are queued and executed on the main thread
- Both GUI and MCP share the same emulator state
- Changes made through MCP are instantly reflected in the GUI and vice versa

## Architecture

### STDIO Transport (Default)
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

### HTTP Transport (Advanced)
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
