#!/usr/bin/env bash
# Install Geargrafx emulator for use as an MCP server.
# Usage: bash scripts/install.sh
#
# This script installs Geargrafx and prints the binary path.
# It supports macOS (Homebrew) and Linux (GitHub releases).

set -euo pipefail

REPO="drhelius/Geargrafx"
INSTALL_DIR="${GEARGRAFX_INSTALL_DIR:-$HOME/.local/bin}"

check_existing() {
    if command -v geargrafx &>/dev/null; then
        echo "geargrafx is already installed: $(command -v geargrafx)"
        exit 0
    fi
    if [[ -x "$INSTALL_DIR/geargrafx" ]]; then
        echo "geargrafx is already installed: $INSTALL_DIR/geargrafx"
        exit 0
    fi
}

install_macos() {
    if command -v brew &>/dev/null; then
        echo "Installing Geargrafx via Homebrew..."
        brew install --cask drhelius/geardome/geargrafx
        local app_path="/Applications/geargrafx.app/Contents/MacOS/geargrafx"
        if [[ -x "$app_path" ]]; then
            echo "$app_path"
            return
        fi
        echo "Installed via Homebrew. Run: brew info drhelius/geardome/geargrafx to find the binary path."
        return
    fi

    echo "Homebrew not found, downloading from GitHub..."
    local arch
    arch=$(uname -m)
    local suffix
    case "$arch" in
        arm64) suffix="arm64" ;;
        x86_64) suffix="intel" ;;
        *)
            echo "Unsupported architecture: $arch"
            echo "Download manually from: https://github.com/$REPO/releases/latest"
            exit 1
            ;;
    esac

    echo "Fetching latest release info..."
    local tag
    tag=$(curl -fsSL "https://api.github.com/repos/$REPO/releases/latest" | grep '"tag_name"' | head -1 | sed 's/.*"tag_name": *"\([^"]*\)".*/\1/')

    if [[ -z "$tag" ]]; then
        echo "Failed to fetch latest release. Download manually from: https://github.com/$REPO/releases/latest"
        exit 1
    fi

    local asset="Geargrafx-${tag}-desktop-macos-${suffix}.zip"
    local url="https://github.com/$REPO/releases/download/${tag}/${asset}"

    echo "Downloading $asset..."
    local tmpdir
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT

    curl -fsSL -o "$tmpdir/geargrafx.zip" "$url"
    unzip -q "$tmpdir/geargrafx.zip" -d "$tmpdir/geargrafx"

    local app
    app=$(find "$tmpdir/geargrafx" -name "geargrafx.app" -type d | head -1)

    if [[ -n "$app" ]]; then
        cp -R "$app" /Applications/
        local bin="/Applications/geargrafx.app/Contents/MacOS/geargrafx"
        if [[ -x "$bin" ]]; then
            echo "$bin"
            return
        fi
    fi

    echo "Binary not found in archive. Check: https://github.com/$REPO/releases/latest"
    exit 1
}

install_linux() {
    # Try PPA first (Ubuntu/Debian)
    if command -v apt-get &>/dev/null; then
        echo "Installing Geargrafx via PPA..."
        sudo mkdir -p /etc/apt/keyrings
        curl -fsSL "https://raw.githubusercontent.com/drhelius/ppa-geardome/main/KEY.gpg" | sudo gpg --dearmor -o /etc/apt/keyrings/ppa-geardome.gpg 2>/dev/null || true
        echo "deb [signed-by=/etc/apt/keyrings/ppa-geardome.gpg] https://raw.githubusercontent.com/drhelius/ppa-geardome/main ./" | sudo tee /etc/apt/sources.list.d/ppa-geardome.list > /dev/null
        sudo apt-get update -qq
        if sudo apt-get install -y geargrafx; then
            echo "$(command -v geargrafx)"
            return
        fi
        echo "PPA install failed, falling back to GitHub release..."
    fi

    local arch
    arch=$(uname -m)
    local suffix
    case "$arch" in
        x86_64)  suffix="x64" ;;
        aarch64) suffix="arm64" ;;
        *)
            echo "Unsupported architecture: $arch"
            echo "Download manually from: https://github.com/$REPO/releases/latest"
            exit 1
            ;;
    esac

    # Detect Ubuntu version or default to 24.04
    local ubuntu_ver="24.04"
    if [[ -f /etc/os-release ]]; then
        local ver
        ver=$(grep VERSION_ID /etc/os-release | cut -d'"' -f2 2>/dev/null || echo "")
        case "$ver" in
            22.04) ubuntu_ver="22.04" ;;
            24.04) ubuntu_ver="24.04" ;;
        esac
    fi

    echo "Fetching latest release info..."
    local tag
    tag=$(curl -fsSL "https://api.github.com/repos/$REPO/releases/latest" | grep '"tag_name"' | head -1 | sed 's/.*"tag_name": *"\([^"]*\)".*/\1/')

    if [[ -z "$tag" ]]; then
        echo "Failed to fetch latest release. Download manually from: https://github.com/$REPO/releases/latest"
        exit 1
    fi

    local asset="Geargrafx-${tag}-desktop-ubuntu${ubuntu_ver}-${suffix}.zip"
    local url="https://github.com/$REPO/releases/download/${tag}/${asset}"

    echo "Downloading $asset..."
    local tmpdir
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT

    curl -fsSL -o "$tmpdir/geargrafx.zip" "$url"
    unzip -q "$tmpdir/geargrafx.zip" -d "$tmpdir/geargrafx"

    mkdir -p "$INSTALL_DIR"
    local bin
    bin=$(find "$tmpdir/geargrafx" -name "geargrafx" -type f | head -1)
    if [[ -z "$bin" ]]; then
        echo "Binary not found in archive. Check: https://github.com/$REPO/releases/latest"
        exit 1
    fi

    cp "$bin" "$INSTALL_DIR/geargrafx"
    chmod +x "$INSTALL_DIR/geargrafx"
    echo "$INSTALL_DIR/geargrafx"
}

main() {
    check_existing

    local os
    os=$(uname -s)
    case "$os" in
        Darwin) install_macos ;;
        Linux)  install_linux ;;
        *)
            echo "Unsupported OS: $os"
            echo "Download manually from: https://github.com/$REPO/releases/latest"
            exit 1
            ;;
    esac
}

main
