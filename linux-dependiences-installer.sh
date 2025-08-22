#!/bin/bash
#Build script for rebuilding everything
set echo on

echo "Installing Dependiences..."
sudo dnf update
sudo dnf install libX11-devel -y
sudo dnf install libxkbcommon-x11-devel -y
sudo apt-get update
sudo apt-get install libx11-dev -y
sudo apt-get install libxkbcommon-x11-dev -y
sudo pacman -S libX11
sudo pacman -S libxkbcommon

echo "All Dependiences Installed Successfully."
echo "Please bear in mind that this automated installer didn't download vulkan sdk."
echo "You need to do it manually."