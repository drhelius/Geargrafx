# Geargrafx

[![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/drhelius/Geargrafx/geargrafx.yml)](https://github.com/drhelius/Geargrafx/actions/workflows/geargrafx.yml)
[![GitHub Releases)](https://img.shields.io/github/v/tag/drhelius/Geargrafx?label=version)](https://github.com/drhelius/Geargrafx/releases)
[![commits)](https://img.shields.io/github/commit-activity/t/drhelius/Geargrafx)](https://github.com/drhelius/Geargrafx/commits/main)
[![GitHub contributors](https://img.shields.io/github/contributors/drhelius/Geargrafx)](https://github.com/drhelius/Geargrafx/graphs/contributors)
[![GitHub Sponsors](https://img.shields.io/github/sponsors/drhelius)](https://github.com/sponsors/drhelius)
[![License](https://img.shields.io/github/license/drhelius/Geargrafx)](https://github.com/drhelius/Geargrafx/blob/main/LICENSE)
[![Twitter Follow](https://img.shields.io/twitter/follow/drhelius)](https://x.com/drhelius)

Geargrafx is a cross-platform TurboGrafx-16 / PC Engine emulator written in C++ that runs on Windows, macOS, Linux, BSD and RetroArch.

This is an open source project with its ongoing development made possible thanks to the support by these awesome [backers](backers.md). If you find it useful, please consider [sponsoring](https://github.com/sponsors/drhelius).

Don't hesitate to report bugs or ask for new features by [opening an issue](https://github.com/drhelius/Geargrafx/issues).

<img src="http://www.geardome.com/files/geargrafx/geargrafx_debug_02.png">

## Downloads

- **Windows**:
  - [Geargrafx-1.0.0-windows-x64.zip](https://github.com/drhelius/Geargrafx/releases/download/1.0.0/Geargrafx-1.0.0-windows-x64.zip)
  - [Geargrafx-1.0.0-windows-arm64.zip](https://github.com/drhelius/Geargrafx/releases/download/1.0.0/Geargrafx-1.0.0-windows-arm64.zip)
  - NOTE: If you have errors you may need to install:
    - [Microsoft Visual C++ Redistributable](https://go.microsoft.com/fwlink/?LinkId=746572)
    - [OpenGL Compatibility Pack](https://apps.microsoft.com/detail/9nqpsl29bfff)
- **macOS**:
  - [Geargrafx-1.0.0-macos-arm.zip](https://github.com/drhelius/Geargrafx/releases/download/1.0.0/Geargrafx-1.0.0-macos-arm.zip)
  - [Geargrafx-1.0.0-macos-intel.zip](https://github.com/drhelius/Geargrafx/releases/download/1.0.0/Geargrafx-1.0.0-macos-intel.zip)
- **Linux**:
  - [Geargrafx-1.0.0-ubuntu-24.04.zip](https://github.com/drhelius/Geargrafx/releases/download/1.0.0/Geargrafx-1.0.0-ubuntu-24.04.zip)
  - [Geargrafx-1.0.0-ubuntu-22.04.zip](https://github.com/drhelius/Geargrafx/releases/download/1.0.0/Geargrafx-1.0.0-ubuntu-22.04.zip)
  - [Geargrafx-1.0.0-ubuntu-20.04.zip](https://github.com/drhelius/Geargrafx/releases/download/1.0.0/Geargrafx-1.0.0-ubuntu-20.04.zip) 
  - NOTE: You may need to install `libsdl2` and `libglew`
- **RetroArch**: [Libretro core documentation](https://docs.libretro.com/library/geargrafx/).

## Features

- Save states with preview.
- Compressed rom support (ZIP).
- Supported platforms (standalone): Windows, Linux, BSD and macOS.
- Supported platforms (libretro): Windows, Linux, macOS, Raspberry Pi, Android, iOS, tvOS, PlayStation Vita, PlayStation 3, Nintendo 3DS, Nintendo GameCube, Nintendo Wii, Nintendo WiiU, Nintendo Switch, Emscripten, Classic Mini systems (NES, SNES, C64, ...), OpenDingux, RetroFW and QNX.
- Full debugger with just-in-time disassembler, CPU breakpoints, memory access breakpoints, code navigation (goto address, JP JR and CALL double clicking), debug symbols, automatic labels, memory editor, PSG inspector and video viewer including registries, tiles, sprites and backgrounds.
- Windows and Linux *Portable Mode*.
- ROM loading from the command line by adding the ROM path as an argument.
- Support for modern game controllers through [gamecontrollerdb.txt](https://github.com/mdqinc/SDL_GameControllerDB) file located in the same directory as the application binary.

## Tips

### Basic Usage
- **Mouse Cursor**: Automatically hides when hovering over the main output window or when Main Menu is disabled.
- **Portable Mode**: Create an empty file named `portable.ini` in the same directory as the application binary to enable portable mode.

### Debugging Features
- **Docking Windows**: In debug mode, you can dock windows together by pressing SHIFT and dragging a window onto another.
- **Multi-viewport**: In Windows or macOS, you can enable "multi-viewport" in the debug menu. You must restart the emulator for the change to take effect. Once enabled, you can drag debugger windows outside the main window.
- **Debug Symbols**: The emulator automatically tries to load a symbol file when loading a ROM. For example, for ```path_to_rom_file.rom``` it tries to load ```path_to_rom_file.sym```. You can also load a symbol file using the GUI or the CLI. It supports PCEAS, wla-dx and vasm file formats.

### Command Line Usage
```
geargrafx [rom_file] [symbol_file]
```

## Build Instructions

### Windows

- Install Microsoft Visual Studio Community 2022 or later.
- Open the Geargrafx Visual Studio solution `platforms/windows/Geargrafx.sln` and build.

### macOS

- Install Xcode and run `xcode-select --install` in the terminal for the compiler to be available on the command line.
- Run these commands to generate a Mac *app* bundle:

``` shell
brew install sdl2
cd platforms/macos
make dist
```

### Linux

- Ubuntu / Debian / Raspberry Pi (Raspbian):

``` shell
sudo apt install build-essential libsdl2-dev libglew-dev libgtk-3-dev
cd platforms/linux
make
```

- Fedora:

``` shell
sudo dnf install @development-tools gcc-c++ SDL2-devel glew-devel gtk3-devel
cd platforms/linux
make
```

- Arch Linux:

``` shell
sudo pacman -S base-devel sdl2 glew gtk3
cd platforms/linux
make
```

### BSD

- FreeBSD:

``` shell
su root -c "pkg install -y git gmake pkgconf SDL2 glew lang/gcc gtk3"
cd platforms/bsd
gmake
```

- NetBSD:

``` shell
su root -c "pkgin install gmake pkgconf SDL2 glew lang/gcc gtk3"
cd platforms/bsd
gmake
```

### Libretro

- Ubuntu / Debian / Raspberry Pi (Raspbian):

``` shell
sudo apt install build-essential
cd platforms/libretro
make
```

- Fedora:

``` shell
sudo dnf install @development-tools gcc-c++
cd platforms/libretro
make
```

## Contributors

Thank you to all the people who have already contributed to Geargrafx!

[![Contributors](https://contrib.rocks/image?repo=drhelius/geargrafx)](https://github.com/drhelius/geargrafx/graphs/contributors)

## License

Geargrafx is licensed under the GNU General Public License v3.0 License, see [LICENSE](LICENSE) for more information.
