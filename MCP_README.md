# Geargrafx MCP Server

A [Model Context Protocol](https://modelcontextprotocol.io/introduction) server for the Geargrafx emulator, enabling AI-assisted debugging and development of TurboGrafx-16 / PC Engine / SuperGrafx games.

This server provides tools for game development, rom hacking, reverse engineering, and debugging through standardized MCP protocols compatible with AI agents like GitHub Copilot, Claude, Codex and others.

## Downloads

<table>
  <thead>
    <tr>
      <th>Platform</th>
      <th>Architecture</th>
      <th>Download Link</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="2"><strong>Windows</strong></td>
      <td>x64</td>
      <td><a href="https://github.com/drhelius/Geargrafx/releases/download/1.7.19/Geargrafx-1.7.19-mcpb-windows-x64.mcpb">Geargrafx-1.7.19-mcpb-windows-x64.mcpb</a></td>
    </tr>
    <tr>
      <td>ARM64</td>
      <td><a href="https://github.com/drhelius/Geargrafx/releases/download/1.7.19/Geargrafx-1.7.19-mcpb-windows-arm64.mcpb">Geargrafx-1.7.19-mcpb-windows-arm64.mcpb</a></td>
    </tr>
    <tr>
      <td rowspan="2"><strong>macOS</strong></td>
      <td>x64</td>
      <td><a href="https://github.com/drhelius/Geargrafx/releases/download/1.7.19/Geargrafx-1.7.19-mcpb-macos-x64.mcpb">Geargrafx-1.7.19-mcpb-macos-x64.mcpb</a></td>
    </tr>
    <tr>
      <td>ARM64</td>
      <td><a href="https://github.com/drhelius/Geargrafx/releases/download/1.7.19/Geargrafx-1.7.19-mcpb-macos-arm64.mcpb">Geargrafx-1.7.19-mcpb-macos-arm64.mcpb</a></td>
    </tr>
    <tr>
      <td rowspan="2"><strong>Linux</strong></td>
      <td>x64</td>
      <td><a href="https://github.com/drhelius/Geargrafx/releases/download/1.7.19/Geargrafx-1.7.19-mcpb-linux-x64.mcpb">Geargrafx-1.7.19-mcpb-linux-x64.mcpb</a></td>
    </tr>
    <tr>
      <td>ARM64</td>
      <td><a href="https://github.com/drhelius/Geargrafx/releases/download/1.7.19/Geargrafx-1.7.19-mcpb-linux-arm64.mcpb">Geargrafx-1.7.19-mcpb-linux-arm64.mcpb</a></td>
    </tr>
  </tbody>
</table>

## Features

- **Full Debugger Access**: CPU registers, memory inspection, breakpoints, and execution control
- **Multiple Memory Areas**: Access RAM, VRAM, ROM, CD-ROM RAM, Arcade Card RAM, and more
- **Disassembly**: View disassembled code around PC or any address
- **Hardware Inspection**: HuC6280 CPU, HuC6270 VDC, HuC6260 VCE, HuC6202 VPC, PSG, CD-ROM subsystems, TurboLink
- **Sprite Viewer**: List and inspect all 64 sprites with images
- **Symbol Support**: Add, remove, list, and look up debug symbols
- **Input State**: Inspect effective pressed buttons and pending tap releases
- **Bookmarks**: Memory and disassembler bookmarks for navigation
- **Call Stack**: View function call hierarchy
- **Trace Logger**: CPU instruction trace with interleaved hardware events (VDC, VCE, PSG, timer, CD-ROM, SCSI, ADPCM, input)
- **Screenshot Capture**: Get current frame as PNG image
- **Rewind**: Time-travel debugging with snapshot status and seek tools
- **Documentation Resources**: Built-in hardware and programming documentation for AI context
- **GUI Integration**: MCP server runs alongside the emulator GUI, sharing the same state

## Transport Modes

The Geargrafx MCP server supports two transport modes:

### STDIO Transport (Recommended)

The default mode uses standard input/output for communication. The emulator is launched by the AI client and communicates through stdin/stdout pipes.

### HTTP Transport

The HTTP transport mode runs the emulator with an embedded web server on `127.0.0.1:7777/mcp` by default. The emulator stays running independently while the AI client connects via HTTP. Each request's `Host` and browser `Origin` must match the address on which its connection reached the server. Loopback mode can run without authentication; wildcard and other non-loopback bind addresses require `GEARGRAFX_MCP_HTTP_TOKEN`, and the server refuses to start without it.

### Headless Mode

Add `--headless` to run without a GUI window. This is useful for servers, CLI agents, or any machine without a display. All MCP tools work identically in headless mode. Requires `--mcp-stdio` or `--mcp-http`.

### Concurrent Clients

The HTTP server accepts repeated valid MCP initialization requests. All connected clients control the same Geargrafx instance. Individual HTTP requests are serialized, but multi-request debugging workflows are not atomic. Concurrent agents can interfere with each other through pauses, resets, breakpoints, memory writes, media loads, and save states.

For independent agent tasks, run one Geargrafx instance per agent on a unique HTTP port. Use `--headless` and give each instance its own portable application directory so its configuration and runtime files are isolated:

```bash
./geargrafx --mcp-http --headless --portable --mcp-http-port 7778
```

The `--portable` option stores configuration and user data beside the application. Alternatively, create an empty `portable.ini` beside the executable in each application directory. On macOS, place it next to each `.app` bundle.

## MCP Tool Router

By default, Geargrafx exposes every MCP tool directly. This avoids nested tool discovery in clients that already defer MCP schemas, including Claude Code.

Add `--mcp-router` to expose a compact set of high-frequency tools directly and route advanced debugger tools through lightweight discovery tools. This reduces MCP context while preserving access to the full debugger surface.

Direct tools in routed mode: `load_media`, `get_media_info`, `debug_pause`, `debug_continue`, `debug_step_into`, `get_huc6280_status`, `read_memory`, `write_memory`, `get_disassembly`, `set_breakpoint`, `get_screenshot`, and `controller_button`.

Router tools:

- `list_tool_categories` lists routed tool categories with descriptions and tool counts.
- `get_category_tools` lists routed tools in a category with compact descriptions.
- `search_tools` searches direct and routed tools and returns compact category/tool/description matches.
- `get_tool_info` returns one tool's real input schema and metadata.
- `execute_tool` executes a routed tool by name. First use `search_tools` or `get_category_tools` to discover the tool, then call `get_tool_info` to obtain its exact input schema.

Example routed call:

```json
{
  "name": "get_huc6270_status",
  "arguments": {}
}
```

Without `--mcp-router`, call every MCP tool directly.

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

3. **Restart VS Code** may be necessary for settings to take effect

4. **Open GitHub Copilot Chat** and start debugging:
   - The emulator will auto-start with MCP server enabled
   - Load a game ROM
   - Start chatting with Copilot about the game state
   - You can add context from "MCP Resources" if needed

### STDIO Mode with Claude Desktop

#### Option 1: Desktop Extension (Recommended)

The easiest way to install Geargrafx MCP server on Claude Desktop is using the MCPB package:

1. **Download the latest MCPB package** for your platform from the [releases page](https://github.com/drhelius/geargrafx/releases).

2. **Install the extension**:
   - Open Claude Desktop
   - Navigate to **Settings > Extensions**
   - Click **Advanced settings**
   - In the Extension Developer section, click **Install Extension…**
   - Select the downloaded `.mcpb` file

3. **Start debugging**: The extension is now available in your conversations. The emulator will automatically launch when the tool is enabled.

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

### STDIO Mode with Claude Code

1. **Add the Geargrafx MCP server** using the CLI:
   ```bash
   claude mcp add --transport stdio geargrafx -- /path/to/geargrafx --mcp-stdio
   ```

   **Important:** Update the path to match your build location.

2. **Verify the server was added**:
   ```bash
   claude mcp list
   ```

3. **Start debugging**: Open Claude Code and start chatting about the game state. The emulator will auto-start when tools are invoked.

### HTTP Mode

1. **Start the emulator manually** with HTTP transport:

   ```bash
   ./geargrafx --mcp-http
   ```

   The default endpoint is `http://127.0.0.1:7777/mcp`.

   To use a custom port:

   ```bash
   ./geargrafx --mcp-http --mcp-http-port 3000
   ```

  To bind to a custom address, set a bearer token first:

   ```bash
  GEARGRAFX_MCP_HTTP_TOKEN="change-this-token" ./geargrafx --mcp-http --mcp-http-address 0.0.0.0 --mcp-http-port 3000
   ```

  Clients must connect using the server's actual interface address, such as `http://192.168.1.50:3000/mcp`, not `0.0.0.0` or a spoofed loopback address.

   You can also start the server using the "MCP" menu in the GUI.

2. **Configure bearer-token authentication**:

  Set `GEARGRAFX_MCP_HTTP_TOKEN` before starting HTTP mode. Authentication is optional for loopback binds and required for wildcard or other non-loopback binds.

   macOS and Linux:

   ```bash
   GEARGRAFX_MCP_HTTP_TOKEN="change-this-token" ./geargrafx --mcp-http
   ```

   Windows PowerShell:

   ```powershell
   $env:GEARGRAFX_MCP_HTTP_TOKEN = "change-this-token"
   .\geargrafx.exe --mcp-http
   ```

   Windows Command Prompt:

   ```cmd
   set GEARGRAFX_MCP_HTTP_TOKEN=change-this-token
   geargrafx.exe --mcp-http
   ```

3. **Configure VS Code** `.vscode/mcp.json`:

   ```json
   {
     "servers": {
       "geargrafx": {
         "type": "http",
         "url": "http://127.0.0.1:7777/mcp",
         "headers": {
           "Authorization": "Bearer change-this-token"
         }
       }
     }
   }
   ```

4. **Or configure Claude Desktop**:

   ```json
   {
     "mcpServers": {
       "geargrafx": {
         "type": "http",
         "url": "http://127.0.0.1:7777/mcp",
         "headers": {
           "Authorization": "Bearer change-this-token"
         }
       }
     }
   }
   ```

5. **Or configure Claude Code**:

   ```bash
   claude mcp add --transport http geargrafx http://127.0.0.1:7777/mcp
   ```

6. **Restart your AI client** and start debugging

> **Note:** The MCP HTTP Server must be running standalone before connecting the AI client.
> **Security:** Without `GEARGRAFX_MCP_HTTP_TOKEN`, HTTP mode starts only on a loopback address. Wildcard and other non-loopback binds are refused. `Host` and browser `Origin` values are matched to the connection's actual destination address to prevent DNS rebinding and address spoofing.

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
- "Tap the up button on player 1 controller"
- "Set player 1 controller to avenue pad 6 type"

### Advanced Debugging Workflows

- "Find the VBlank interrupt handler, analyze what it does, and add symbols for all the subroutines it calls"
- "Locate the sprite update routine. Study how this game manages its sprite system, explain the algorithm, and add bookmarks to key sections. Also add watches for any sprite-related variables you find"
- "There's a data decompression routine around address 0xC000. Step through it instruction by instruction, reverse engineer the compression algorithm, and explain how it works with examples"
- "Find where the game stores its level data in ROM. Analyze the data structure format, create a memory map showing each section, and add symbols for the data tables"
- "The game is rendering corrupted graphics. Examine the VDC registers, check the VRAM contents, inspect the sprite attribute table, and diagnose what's causing the corruption. Set up watches on relevant memory addresses"

## Available MCP Tools

This is the full tool catalog. All tools are exposed directly by default. With `--mcp-router`, discover advanced tools through `search_tools` or `get_category_tools`, inspect their schemas with `get_tool_info`, then invoke them with `execute_tool`.

The server exposes tools organized in the following categories:

### Execution Control
- `debug_pause` - Pause emulation
- `debug_continue` - Resume emulation  
- `debug_step_into` - Step one instruction
- `debug_step_over` - Step over subroutine calls
- `debug_step_out` - Step out of current subroutine
- `debug_step_frame` - Step one or more frames. Optional `frames` is 1-1000 (default 1). Optional `mode` is `async` (default, returns after scheduling) or `sync` (returns after all requested frames complete at VBlank). Use `mode: "sync"` when issuing dependent tool calls.
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
- `memory_find` - Find hex byte sequences (`hex_bytes`) or text (`text`, optional `case_sensitive`) in memory

### Disassembly & Debugging
- `get_disassembly` - Get disassembly for specified address range
- `add_symbol` - Add symbol (label) at specified address
- `remove_symbol` - Remove symbol
- `list_symbols` - List all defined symbols
- `lookup_symbol_by_name` - Find all exact-name symbol matches
- `lookup_symbol_at_address` - Find symbol at bank/address
- `add_disassembler_bookmark` - Add bookmark in disassembler
- `remove_disassembler_bookmark` - Remove disassembler bookmark
- `list_disassembler_bookmarks` - List all disassembler bookmarks
- `get_call_stack` - View function call hierarchy
- `get_trace_log` - Read trace logger entries (CPU + hardware events) using absolute sequence pagination. Use `set_trace_log` to start or stop the logger
- `set_trace_log` - Start or stop trace logging and configure exact event filters and memory or disk storage

#### Trace pagination

`get_trace_log` returns:

- `total_entries`: entries currently retained in the memory ring.
- `total_logged`: the next absolute sequence number. It is monotonic for the process lifetime and is not reset by clear, resize, media changes, or save-state loads.
- `oldest_sequence`: absolute sequence of the oldest retained entry (`total_logged - total_entries`).
- `start`: actual absolute sequence used for this page.
- `next_sequence`: absolute sequence to pass as the next `start`.
- `count`: number of returned entries.
- `overrun`: `true` when a requested `start` had expired and was clamped to `oldest_sequence`.
- `lines`: formatted trace entries in sequence order.

When `start` is omitted, the latest 100 retained entries are returned. A negative `start` requests that many entries from the retained tail. Positive starts are absolute sequences. `count` defaults to 100 and is capped at 1000. A requested sequence older than `oldest_sequence` starts at the oldest retained entry and sets `overrun` to `true`. A sequence at or beyond `total_logged` returns an empty page with `start` and `next_sequence` equal to the requested value and `overrun` set to `false`.

Memory-mode counters shown in the GUI and written by manual export use the same absolute sequence values as MCP. Disk trace files use a separate zero-based entry counter local to each file.

#### Trace filters

Omitting `filters` enables the safe default of CPU instructions and IRQs. When supplied, `filters` must be non-empty, unique, and contain only these exact values:

```text
cpu.instructions
cpu.irqs
vdc.registers
vdc.irqs
vdc.dma
vce.registers
vce.timing
input.reads
input.writes
input.turbolink
input.turbolink.writes
input.turbolink.drive
input.turbolink.samples
input.turbolink.cable
timer.irqs
timer.registers
cdrom.irqs
cdrom.control
cdrom.audio
psg.global_lfo
psg.frequency
psg.channel
psg.wave_dda
psg.noise
adpcm.registers
adpcm.dma
adpcm.playback
adpcm.transfers
adpcm.irqs
scsi.commands
scsi.phases
scsi.responses
scsi.response_bytes
scsi.transfers
scsi.problems
system.mpr
system.mapper
system.interrupts
```

`input.turbolink` enables all four TurboLink streams. `writes` records every `$1000` O-port access, `drive` records only changes to the BU5782K pull-low outputs, `samples` records physical LINK1/LINK2 levels and the actual K result with D0-D3, and `cable` records activation/deactivation of the local emulated hardware endpoint. Shared-memory heartbeats and barriers are intentionally not emulation trace events.

The `system` streams cover TAM/MPR mappings, Street Fighter II mapper latch updates, and HuC6280 interrupt-controller mask/acknowledgement writes. ADPCM lines include playing, pending, half-IRQ, and end-IRQ state bits.

#### Trace storage

Starting a stopped logger without `output` selects `memory`. Memory capacities are `100K`, `500K`, `1M`, `2M`, and `5M` entries; the persisted default is `100K`. Disk limits are `10MB`, `50MB`, `100MB`, `250MB`, `500MB`, `1GB`, and `unbounded`; the persisted default is `100MB`.

Storage changes while tracing is active cleanly stop and restart the logger. Repeating the active storage configuration is idempotent and only updates filters. Stopping preserves retained memory entries; changing memory capacity or starting disk output resets the ring used by that recording. Disk output is flushed before stop, reset, media changes, physical CD-ROM eject/error, save-state loads, and shutdown. Failures are reported in the UI and application log but do not cancel unrelated emulator operations.

`output_path` is a directory only, not a filename. Geargrafx creates a unique timestamped UTF-compatible trace filename in that directory. When omitted, the configured default, media, or custom directory policy remains in effect.

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
- `list_cdrom_tracks` - List CD-ROM track types and LBA ranges
- `get_cdrom_audio_status` - Get CD-ROM audio playback status
- `get_adpcm_status` - Get ADPCM audio status
- `get_arcade_card_status` - Get Arcade Card status
- `get_turbolink_status` - Get BU5782K SEL/CLR and pull-low state, the last actual K/line sample with D0-D3 and event ticks, plus shared-memory membership, hardware readiness, pacing, progress, barrier, lease, generation, and recovery diagnostics
- `reset_turbolink_metrics` - Reset TurboLink activity, synchronization, wait, and recovery counters

### Sprites
- `list_sprites` - List all 64 sprites with position, size, pattern, palette
- `get_sprite_image` - Get sprite image as base64 PNG

### Screen Capture
- `get_screenshot` - Capture current screen frame as base64 PNG

### Media & State Management
- `get_media_info` - Get loaded ROM/CD info
- `list_recent_media` - List the 10 most recent ROM files or CD-ROM images opened by Geargrafx
- `load_media` - Load ROM file or CD-ROM image (.pce, .sgx, .hes, .cue, .zip). Automatically loads .sym symbol file if present
- `load_bios` - Load a BIOS file for CD-ROM emulation. Two types: 'syscard' (System Card, 256KB) and 'gameexpress' (Game Express, 32KB)
- `load_symbols` - Load debug symbols from file (.sym format with 'BANK:ADDRESS LABEL' entries)
- `list_save_state_slots` - List all 5 save state slots with information (rom name, timestamp, validity)
- `select_save_state_slot` - Select active save state slot (1-5) for save/load operations
- `save_state` - Save emulator state to currently selected slot
- `load_state` - Load emulator state from currently selected slot
- `save_state_file` - Save emulator state to an explicit file path
- `load_state_file` - Load emulator state from an explicit file path
- `set_fast_forward_speed` - Set fast forward speed multiplier (0: 1.5x, 1: 2x, 2: 2.5x, 3: 3x, 4: Unlimited)
- `toggle_fast_forward` - Toggle fast forward mode on/off
- `get_rewind_status` - Get rewind buffer status (enabled, snapshots, capacity, buffered seconds)
- `rewind_seek` - Seek to a specific rewind snapshot while paused

### Controller Input
- `controller_button` - Control a button on a controller (player 1-5). Use action 'press' to hold the button, 'release' to let it go, or 'press_and_release' to simulate a quick tap. Buttons: up, down, left, right, select, run, I, II, III, IV, V, VI
- `controller_macro` - Run an ordered input macro. Top-level `player` defaults to 1, and each command may override it. Supported commands are `tap`, `press`, `release`, and `wait`; timing is explicit through `wait` frame counts
- `get_input_state` - Get effective pressed buttons and current controller input state
- `controller_set_type` - Set controller type for a player: standard (2 buttons), avenue_pad_3 (3 buttons), avenue_pad_6 (6 buttons)
- `controller_get_type` - Get the current controller type for a player (returns: standard, avenue_pad_3, or avenue_pad_6)
- `controller_set_turbo_tap` - Enable or disable Turbo Tap (multitap) for 5-player support

## Available MCP Resources

In addition to tools, the MCP server provides documentation resources that AI assistants can access to better understand the PC Engine / TurboGrafx-16 hardware and programming.

MCP clients usually offer resources in the "Add context..." section of the chat interface. You may need to manually add them when you think they are relevant.

### Hardware Documentation Resources

Complete technical reference documentation for all PC Engine / TurboGrafx-16 hardware components:

- **HuC6280 CPU — 8-bit CMOS Microprocessor** (`geargrafx://hardware/huc6280_cpu`)
- **HuC6280 Instruction Set Reference** (`geargrafx://hardware/huc6280_instructions`)
- **HuC6280 PSG — Programmable Sound Generator** (`geargrafx://hardware/huc6280_psg`)
- **HuC6270 VDC — Video Display Controller** (`geargrafx://hardware/huc6270_vdc`)
- **HuC6260 VCE — Video Color Encoder** (`geargrafx://hardware/huc6260_vce`)
- **HuC6202 VPC — Video Priority Controller** (`geargrafx://hardware/huc6202_vpc`)
- **PC Engine Memory Map** (`geargrafx://hardware/memory_map`)

## How MCP Works in Geargrafx

- The MCP server runs **alongside** the GUI in a background thread
- The emulator GUI remains fully functional (you can play/debug normally while using MCP)
- Commands from the AI are queued and executed on the GUI thread
- Both GUI and MCP share the same emulator state
- Changes made through MCP are instantly reflected in the GUI and vice versa

## Architecture

### STDIO Transport
```
┌─────────────────┐                    ┌──────────────────┐
│   VS Code /     │       stdio        │    Geargrafx     │
│ Claude Desktop  │◄──────────────────►│    MCP Server    │
│   (AI Client)   │       pipes        │   (background)   │
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
│   (AI Client)   │                    │    (listener)    │
└─────────────────┘                    └──────────────────┘
                                                │
                                                │ Shared State
                                                ▼
                                       ┌──────────────────┐
                                       │   Emulator Core  │
                                       │   + GUI Window   │
                                       └──────────────────┘
```
