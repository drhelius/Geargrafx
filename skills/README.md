# Geargrafx Agent Skills

[Agent Skills](https://agentskills.io/) for the Geargrafx TurboGrafx-16 / PC Engine / SuperGrafx emulator MCP server. These skills teach AI agents how to effectively use Geargrafx's MCP tools for debugging and ROM hacking tasks.

## Prerequisites

All skills require the **Geargrafx emulator** running as an MCP server. The emulator must be configured in your AI client (VS Code, Claude Desktop, Claude Code, etc.) so the agent can access the MCP tools.

See [MCP_README.md](../MCP_README.md) for complete setup instructions (STDIO, HTTP, VS Code, Claude Desktop, Claude Code).

## Installation

The recommended way to install the skills is using the [`skills`](https://skills.sh/docs) CLI, which requires no prior installation:

```bash
npx skills add drhelius/geargrafx
```

Or install a specific skill:

```bash
npx skills add drhelius/geargrafx --skill geargrafx-debugging
npx skills add drhelius/geargrafx --skill geargrafx-romhacking
```

This downloads and configures the skills for use with your AI agent. See the [skills CLI reference](https://skills.sh/docs/cli) for more details.

## Available Skills

### geargrafx-debugging

**Purpose**: Game development, debugging, and tracing of TurboGrafx-16 / PC Engine / SuperGrafx games.

**What it covers**:
- Loading ROMs, CD-ROM images, and debug symbols
- HuC6280 CPU register, flag, and MPR inspection
- Setting execution, read, write, and range breakpoints across 5 memory areas
- Stepping through code (into, over, out, frame, run-to)
- Execution tracing with interleaved hardware events (VDC, VCE, PSG, timer, CD-ROM, SCSI, ADPCM, input)
- Hardware inspection: HuC6270 VDC (sprites, backgrounds), HuC6260 VCE (palette, color), HuC6202 VPC (SuperGrafx), PSG, CD-ROM subsystems
- Sprite viewer with image capture
- Screenshot capture
- Call stack analysis
- Organizing debug sessions with symbols, bookmarks, and watches

**Key MCP tools used**: `debug_pause`, `debug_step_into`, `debug_step_over`, `debug_step_out`, `set_breakpoint`, `get_huc6280_status`, `get_disassembly`, `get_call_stack`, `set_trace_log`, `get_trace_log`, `get_huc6270_registers`, `get_huc6260_status`, `get_psg_status`, `list_sprites`, `add_symbol`, `get_screenshot`

**Example prompts**:
- "Find the VBlank interrupt handler and analyze what it does"
- "Set a breakpoint at $E000 and step through the code"
- "The game has corrupted graphics — diagnose the VDC registers"
- "Trace the sprite rendering routine and explain the algorithm"
- "Debug the CD-ROM loading sequence and trace SCSI commands"

### geargrafx-romhacking

**Purpose**: Creating modifications, cheats, translations, and ROM hacks for TurboGrafx-16 / PC Engine / SuperGrafx games.

**What it covers**:
- Memory search workflows (capture → change → compare cycle)
- Finding game variables (lives, health, score, position)
- Creating cheats (infinite lives, score modification, etc.)
- Text and string discovery for translations
- Sprite and graphics data location via sprite viewer and VRAM inspection
- Data table and structure reverse engineering
- Save state management for safe experimentation
- Fast forwarding to reach specific game states

**Key MCP tools used**: `memory_search_capture`, `memory_search`, `memory_find_bytes`, `read_memory`, `write_memory`, `set_breakpoint` (write type), `add_memory_watch`, `add_memory_bookmark`, `save_state`, `load_state`, `toggle_fast_forward`, `get_screenshot`, `list_sprites`, `get_sprite_image`, `controller_button`

**Example prompts**:
- "Find the lives counter and give me infinite lives"
- "Search for the score variable in memory"
- "Find all text strings in the ROM for translation"
- "Locate the sprite data for the player character"
- "Create a cheat for maximum health in this CD-ROM game"
