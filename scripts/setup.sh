#!/usr/bin/env bash
base_dir=$(dirname "$0")
root_dir=$base_dir/..
git submodule update --init "$root_dir/assets"
git submodule update --init "$root_dir/third_party/stb"
git submodule update --init "$root_dir/third_party/vulkan/Vulkan-Headers"

if [ -x "$(command -v apt)" ]; then
  sudo apt install clang python-is-python3 ninja-build libx11-xcb-dev libxcb-keysyms1-dev libxkbcommon-x11-dev
elif [ -x "$(command -v dnf)" ]; then
  sudo dnf install clang ninja-build libX11-devel xcb-util-keysyms-devel libxkbcommon-x11-devel
fi
