#!/usr/bin/env bash
base_dir=$(dirname "$0")
root_dir=$base_dir/..
git submodule update "$root_dir/assets"
git submodule update "$root_dir/third_party/stb"
git submodule update "$root_dir/third_party/vulkan/Vulkan-Headers"

if [ -x "$(command -v apt)" ]; then
  sudo apt install clang python-is-python3 ninja-build libx11-xcb-dev libxcb-keysyms1-dev libxkbcommon-x11-dev
fi
