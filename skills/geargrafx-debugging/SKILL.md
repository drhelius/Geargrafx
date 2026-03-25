---
name: geargrafx-debugging
description: >-
  Debug and trace TurboGrafx-16 / PC Engine / SuperGrafx games using the
  Geargrafx emulator MCP server. Provides workflows for HuC6280 CPU debugging,
  breakpoint management, hardware inspection, disassembly analysis, and
  execution tracing. Use when the user wants to debug a PC Engine game, trace
  code execution, inspect CPU registers or hardware state, set breakpoints,
  analyze interrupts, step through HuC6280 instructions, reverse engineer game
  code, examine VDC/VCE/PSG registers, view the call stack, or diagnose
  rendering, audio, or timing issues. Also use when the user mentions PC Engine
  development, TurboGrafx-16 homebrew testing, SuperGrafx debugging, CD-ROM
  game debugging, or HuC6280 debugging with Geargrafx.
compatibility: >-
  Requires the Geargrafx emulator running as an MCP server (stdio or HTTP
  transport). Configure your AI client to connect to Geargrafx via MCP before
  using this skill.
metadata:
  author: drhelius
  version: "1.0"
---

# TurboGrafx-16 / PC Engine Game Debugging with Geargrafx

## Overview

Debug TurboGrafx-16, PC Engine, and SuperGrafx games using the Geargrafx emulator as an MCP server. Control execution (pause, step, breakpoints), inspect the HuC6280 CPU and hardware (HuC6270 VDC, HuC6260 VCE, HuC6202 VPC, PSG), read/write memory, disassemble code, trace instructions, and capture screenshots — all through MCP tool calls. The emulator also serves hardware documentation as MCP resources.

## MCP Server Requirement

This skill requires the **Geargrafx MCP server** to be installed and connected. All debugging operations are performed through Geargrafx MCP tools. Verify connectivity by calling `debug_get_status` — if it returns a response, the server is active.

### Installing Geargrafx

Run the bundled install script (macOS/Linux):

```bash
bash scripts/install.sh
```

This installs Geargrafx via Homebrew on macOS or downloads the latest release on Linux. It prints the binary path on completion. You can also set `GEARGRAFX_INSTALL_DIR` to control where the binary goes (default: `~/.local/bin`).

Alternatively, download from [GitHub Releases](https://github.com/drhelius/Geargrafx/releases/latest) or install with `brew install --cask drhelius/geardome/geargrafx` on macOS.

### Connecting as MCP Server

Geargrafx runs as an MCP server using STDIO transport (recommended). Configure your AI client:

**VS Code** — create `.vscode/mcp.json`:
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

**Claude Code:**
```bash
claude mcp add --transport stdio geargrafx -- /path/to/geargrafx --mcp-stdio
```

**Claude Desktop** — edit config (`~/Library/Application Support/Claude/claude_desktop_config.json` on macOS):
```json
{
  "mcpServers": {
    "geargrafx": {
      "command": "/path/to/geargrafx",
      "args": ["--mcp-stdio"]
    }
  }
}
```

Replace `/path/to/geargrafx` with the actual binary path from the install script.

### Hardware Documentation (MCP Resources)

The Geargrafx MCP server provides built-in PC Engine hardware documentation as MCP resources. Load them into your context when investigating specific hardware.

| Resource | URI | Load when... |
|---|---|---|
| HuC6280 CPU | `geargrafx://hardware/huc6280_cpu` | CPU registers, MPR mapping, timer, interrupts, I/O, speed modes |
| Instruction Set | `geargrafx://hardware/huc6280_instructions` | Opcode reference, addressing modes, cycle counts |
| PSG | `geargrafx://hardware/huc6280_psg` | 6-channel sound: waveform, noise, LFO, volume, DDA |
| HuC6270 VDC | `geargrafx://hardware/huc6270_vdc` | Video Display Controller: BAT, sprites, scroll, DMA, interrupts |
| HuC6260 VCE | `geargrafx://hardware/huc6260_vce` | Video Color Encoder: palette, dot clock, color format |
| HuC6202 VPC | `geargrafx://hardware/huc6202_vpc` | Video Priority Controller (SuperGrafx): window, priority |
| Memory Map | `geargrafx://hardware/memory_map` | Full memory map: MPR pages, I/O, WRAM, VRAM, ROM banking |

---

## Debugging Workflow

### 1. Load and Orient

```
load_media → get_media_info → get_huc6280_status → get_screenshot
```

Start every session by loading the ROM, confirming it loaded correctly, then checking CPU state and taking a screenshot to understand the current game state. If a `.sym`, `.lbl`, or `.noi` file exists alongside the ROM, symbols are loaded automatically.

Load additional symbols with `load_symbols` or add individual labels with `add_symbol`.

### 2. Pause and Inspect

Always call `debug_pause` before inspecting state. While paused:

- **CPU state**: `get_huc6280_status` — registers A, X, Y, S, P (flags), PC, MPR mapping, timer, interrupts, I/O port, speed mode
- **Disassembly**: `get_disassembly` with a start/end address range — only shows executed code paths
- **Call stack**: `get_call_stack` — current subroutine hierarchy
- **Memory**: `read_memory` with a memory area tab ID (use `list_memory_areas` to discover available areas and their IDs)

### 3. Set Breakpoints

Use breakpoints to stop execution at points of interest:

| Breakpoint Type | Tool | Use Case |
|---|---|---|
| Execution | `set_breakpoint` (type: exec) | Stop when PC reaches address |
| Read | `set_breakpoint` (type: read) | Stop when memory address is read |
| Write | `set_breakpoint` (type: write) | Stop when memory address is written |
| Range | `set_breakpoint_range` | Cover an address range (exec/read/write) |

Breakpoints support 5 memory areas: `rom_ram` (default), `vram`, `palette`, `huc6270_reg`, `huc6260_reg`.

**Important**: Read/write breakpoints stop with PC at the instruction *after* the memory access.

Manage breakpoints with `list_breakpoints`, `remove_breakpoint`.

### 4. Step Through Code

After hitting a breakpoint or pausing:

| Action | Tool | Behavior |
|---|---|---|
| Step Into | `debug_step_into` | Execute one instruction, enter subroutines |
| Step Over | `debug_step_over` | Execute one instruction, skip JSR calls |
| Step Out | `debug_step_out` | Run until RTS/RTI returns from current subroutine |
| Step Frame | `debug_step_frame` | Execute until next VBlank |
| Run To | `debug_run_to_cursor` | Continue until PC reaches target address |
| Continue | `debug_continue` | Resume normal execution |

After each step, call `get_huc6280_status` and `get_disassembly` to see where you are.

### 5. Trace Execution

The trace logger records CPU instructions interleaved with hardware events (VDC, VCE, PSG, timer, CD-ROM, SCSI, ADPCM, input).

1. `set_trace_log` with `enabled: true` to start recording (optionally filter event types)
2. Let the game run or step through code
3. `set_trace_log` with `enabled: false` to stop (entries are preserved)
4. `get_trace_log` to read recorded entries

Available trace event filters: `cpu_irq`, `vdc`, `vce`, `psg`, `timer`, `input`, `cdrom`, `adpcm`, `scsi`. CPU tracing is always on.

Tracing is essential for understanding timing-sensitive code, interrupt handlers, and hardware interaction sequences.

---

## Hardware Inspection

### HuC6270 VDC (Video Display Controller)

- `get_huc6270_status` — VDC state: position, control, interrupt flags
- `get_huc6270_registers` — all 20 VDC registers (0x00-0x13), Address Register (AR), Status Register (SR)
- `write_huc6270_register` — write to a VDC register (0-19) or AR (20). Use vdc parameter (1 or 2) for SuperGrafx
- `list_sprites` — all 64 sprites with position, size, pattern, palette
- `get_sprite_image` — get a sprite as a PNG image

### HuC6260 VCE (Video Color Encoder)

- `get_huc6260_status` — VCE state: position, sync signals, dot clock, control

### HuC6202 VPC (SuperGrafx only)

- `get_huc6202_status` — VPC state: window settings, priority configuration

### PSG (Programmable Sound Generator)

- `get_psg_status` — all 6 PSG channels: waveform, frequency, volume, noise, LFO, DDA

### CD-ROM Subsystems (CD games only)

- `get_cdrom_status` — CD-ROM drive status
- `get_cdrom_audio_status` — CD audio playback status
- `get_adpcm_status` — ADPCM audio status
- `get_arcade_card_status` — Arcade Card status

### CPU

- `get_huc6280_status` — full CPU state: registers, MPR, timer, interrupts, I/O, speed
- `write_huc6280_register` — modify a register live

### Screenshots

- `get_screenshot` — current rendered frame as PNG

Use screenshots after stepping or continuing to see the visual impact of changes.

---

## Common Debugging Scenarios

### Finding an Interrupt Handler

1. Set an execution breakpoint at the IRQ vector address (read the Memory Map resource for the vector table)
2. `debug_continue` to run until the IRQ fires
3. `get_huc6280_status` + `get_disassembly` to see the handler code
4. `get_call_stack` to see how deep you are
5. `add_symbol` to label the handler address and any subroutines it calls

### Diagnosing Graphics Corruption

1. `debug_pause` → `get_huc6270_registers` — check BAT, sprite attributes, scroll, DMA settings
2. `get_huc6260_status` — verify dot clock, color mode
3. `get_screenshot` — capture the current visual state
4. `read_memory` on VRAM and SAT areas to inspect tile/sprite data
5. Set read/write breakpoints (memory_area: `vram` or `huc6270_reg`) on display buffer addresses to catch corruption source

### Analyzing a Subroutine

1. `set_breakpoint` at the subroutine entry point
2. `debug_continue` → when hit, `get_huc6280_status`
3. Step through with `debug_step_into` / `debug_step_over`
4. After each step: check registers, read relevant memory
5. `add_symbol` for the routine and any called subroutines
6. `add_disassembler_bookmark` to mark interesting locations

### Tracking a Variable

1. `add_memory_watch` on the variable's address — watches are visible in the emulator GUI
2. Set a write breakpoint with `set_breakpoint` (type: write) on that address
3. When hit, `get_disassembly` reveals what code is modifying it
4. `get_call_stack` shows the call chain leading to the write

### Timing Analysis

1. `set_trace_log` with `enabled: true` to start recording timer and VDC events
2. Let the game run through the section of interest
3. `get_trace_log` to see the interleaved CPU + hardware events
4. Check VDC interrupt timing via `get_huc6270_status`
5. Correlate timer fires with code execution in the trace

### CD-ROM Game Debugging

1. `load_media` with a `.cue` file to load a CD-ROM image
2. `get_cdrom_status` to verify drive state
3. `set_trace_log` with `cdrom: true` and `scsi: true` to trace CD access patterns
4. `get_adpcm_status` to inspect ADPCM audio state
5. Set breakpoints on CD-ROM RAM areas to catch data loading

---

## Memory Areas

Use `list_memory_areas` to get the full list with IDs and sizes. Common areas:

| Area | Description | Typical Size |
|---|---|---|
| WRAM | Working RAM | 8KB (32KB for SuperGrafx) |
| ZP | Zero Page (fast variables) | 256 bytes |
| ROM | Game ROM | Varies |
| CARD RAM | HuCard RAM | Varies |
| BRAM | Backup RAM (save data) | 2KB |
| PALETTES | VCE color table | 512 bytes |
| VRAM / VRAM 1 | VDC video RAM | 64KB |
| VRAM 2 | VDC 2 video RAM (SuperGrafx) | 64KB |
| SAT / SAT 1 | Sprite Attribute Table | 512 bytes |
| SAT 2 | SAT for VDC 2 (SuperGrafx) | 512 bytes |
| CDROM RAM | CD-ROM working RAM | Varies |
| ADPCM | ADPCM sample RAM | 64KB |
| ARCADE | Arcade Card RAM | Varies |
| MB128 | Memory Base 128 | 128KB |

---

## Organizing Your Debug Session

- **Symbols**: Use `add_symbol` liberally to label addresses you've identified — makes disassembly readable
- **Bookmarks**: Use `add_disassembler_bookmark` for code locations and `add_memory_bookmark` for data regions
- **Watches**: Use `add_memory_watch` for variables you're tracking across steps
- **Save states**: Use `save_state` / `load_state` to snapshot and restore emulator state at interesting points
- **Screenshots**: Capture visual state with `get_screenshot` after significant changes
