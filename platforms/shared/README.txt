   ____                                 __      
  / ___| ___  __ _ _ __ __ _ _ __ __ _ / _|_  __
 | |  _ / _ \/ _` | '__/ _` | '__/ _` | |_\ \/ /
 | |_| |  __/ (_| | | | (_| | | | (_| |  _|>  < 
  \____|\___|\__,_|_|  \__, |_|  \__,_|_| /_/\_\
                       |___/                    

A very accurate cross-platform TurboGrafx-16 / PC Engine / SuperGrafx / PCE CD-ROM² emulator
-----------------------------------------------------
Instructions and tips at:
https://github.com/drhelius/Geargrafx
-----------------------------------------------------
This is an open source project with its ongoing development made possible thanks to the support by awesome backers.
Please, consider sponsoring: https://github.com/sponsors/drhelius
Follow me on Twitter for updates: http://twitter.com/drhelius
Don't hesitate to report bugs or ask for new features by opening an issue: https://github.com/drhelius/Geargrafx/issues
-----------------------------------------------------
Features:

- Accurate emulation supporting the entire HuCard PCE / SGX catalog.
- Support for CD-ROM², Super CD-ROM² and Arcade CD-ROM² systems.
- Save states with preview.
- Backup RAM and Memory Base 128 support.
- Multi Tap (up to 5 players).
- Standard Gamepad (2 buttons), Avenue Pad 3 (3 buttons, auto-configured based on game), Avenue Pad 6 (6 buttons).
- Adjustable scanline count (224p, 240p or manual).
- RGB or Composite color output.
- Compressed rom and CD images support (pce, sgx, cue, zip and chd).
- Music rom support: HES.
- VGM recorder.
- Internal database for automatic rom detection and hardware selection if Auto options are selected.
- Full debugger with just-in-time disassembler, CPU breakpoints, memory access breakpoints, code navigation, debug symbols, automatic labels, memory editor, PSG inspector and video viewer.
- MCP server for AI-assisted debugging with GitHub Copilot, Claude, ChatGPT and similar, exposing tools for execution control, memory inspection, hardware status, and more.
- Windows and Linux Portable Mode.
- ROM loading from the command line by adding the ROM path as an argument.
- ROM loading using drag & drop.
- Support for modern game controllers through gamecontrollerdb.txt file located in the same directory as the application binary.

-----------------------------------------------------
Tips:

Basic Usage:
- BIOS: Geargrafx requires a BIOS to run CD-ROM games. It is possible to load any BIOS but the System Card 3.0 with md5 38179df8f4ac870017db21ebcbf53114 is recommended.
- CD-ROM Images: Geargrafx supports chd, zipped and unzipped cue/bin, cue/img and cue/iso images. cue/iso + wav is also supported when audio track format is 44100Hz, 16 bit, stereo. It does not support MP3 or OGG audio tracks.
- Mouse Cursor: Automatically hides when hovering over the main output window or when Main Menu is disabled.
- Portable Mode: Create an empty file named portable.ini in the same directory as the application binary to enable portable mode.

Debugging Features:
- Docking Windows: In debug mode, you can dock windows together by pressing SHIFT and dragging a window onto another.
- Multi-viewport: In Windows or macOS, you can enable multi-viewport in the debug menu. You must restart the emulator for the change to take effect. Once enabled, you can drag debugger windows outside the main window.
- Debug Symbols: The emulator automatically tries to load a symbol file when loading a ROM. For example, for path_to_rom_file.rom it tries to load path_to_rom_file.sym. You can also load symbol files using the GUI or the CLI. It supports PCEAS (old and new format), wla-dx and vasm file formats.

Command Line Usage:
geargrafx [options] [game_file] [symbol_file]

Arguments:
  [game_file]              Game file: accepts ROMs (.pce, .sgx, .hes), CUE (.cue) or ZIP (.zip)
  [symbol_file]            Optional symbol file for debugging

Options:
  -f, --fullscreen         Start in fullscreen mode
  -w, --windowed           Start in windowed mode with menu visible
      --mcp-stdio          Auto-start MCP server with stdio transport
      --mcp-http           Auto-start MCP server with HTTP transport
      --mcp-http-port N    HTTP port for MCP server (default: 7777)
  -v, --version            Display version information
  -h, --help               Display this help message

MCP Server:
Geargrafx includes a Model Context Protocol (MCP) server that enables AI-assisted debugging through AI agents like GitHub Copilot, Claude, ChatGPT and similar. The server provides tools for execution control, memory inspection, breakpoints, disassembly, hardware status, sprite viewing, and more.
For complete setup instructions and tool documentation, see MCP_README.md at https://github.com/drhelius/Geargrafx

-----------------------------------------------------
Geargrafx is licensed under the GNU General Public License v3.0 License:

Copyright (C) 2024 Ignacio Sanchez
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program. If not, see http://www.gnu.org/licenses/
