# Geargrafx

[![GitHub Workflow Status (with event)](https://img.shields.io/github/actions/workflow/status/drhelius/Geargrafx/geargrafx.yml)](https://github.com/drhelius/Geargrafx/actions/workflows/geargrafx.yml)
[![GitHub tag (with filter)](https://img.shields.io/github/v/tag/drhelius/Geargrafx?label=version)](https://github.com/drhelius/Geargrafx/releases)
[![commits)](https://img.shields.io/github/commit-activity/t/drhelius/Geargrafx)](https://github.com/drhelius/Geargrafx/commits/main)
[![GitHub contributors](https://img.shields.io/github/contributors/drhelius/Geargrafx)](https://github.com/drhelius/Geargrafx/graphs/contributors)
[![GitHub Sponsors](https://img.shields.io/github/sponsors/drhelius)](https://github.com/sponsors/drhelius)
[![GitHub](https://img.shields.io/github/license/drhelius/Geargrafx)](https://github.com/drhelius/Geargrafx/blob/main/LICENSE)
[![Twitter Follow](https://img.shields.io/twitter/follow/drhelius)](https://twitter.com/drhelius)

> IN DEVELOPMENT: Not intended to be used now!

This is an open source project with its ongoing development made possible thanks to the support by these awesome [backers](backers.md). If you find it useful, please, consider [sponsoring](https://github.com/sponsors/drhelius).

----------

![Geargrafx](https://github.com/user-attachments/assets/7db9eab6-e9c4-422d-a75f-3fde73d3d7f2)

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

[![Contributors](https://contrib.rocks/image?repo=drhelius/geargrafx)]("https://github.com/drhelius/geargrafx/graphs/contributors)

## License

Geargrafx is licensed under the GNU General Public License v3.0 License, see [LICENSE](LICENSE) for more information.
