set echo on

echo "Installing Dependiences..."
if command -v pacman ; then
    echo "Installing pacman version..."
    sudo pacman -S libX11 && sudo pacman -S libxkbcommon
    sudo pacman -S vulkan-icd-loader vulkan-tools vulkan-extra-layers vulkan-validation-layers vulkan-headers clang
    vkcube
    echo "All Dependiences Installed Successfully."
elif command -v apt ; then
    echo "Installing apt version..."
    sudo apt-get update -y && sudo apt-get install libx11-dev -y && sudo apt-get install libxkbcommon-x11-dev -y
    sudo apt install vulkan-tools vulkan-validationlayers-dev libvulkan-dev clang-y
    vkcube
    echo "All Dependiences Installed Successfully."
elif command -v dnf ; then
    echo "Installing dnf version..."
    sudo dnf update -y && sudo dnf install libX11-devel -y && sudo dnf install libxkbcommon-x11-devel -y
    sudo dnf install vulkan vulkan-tools vulkan-loader vulkan-validation-layers vulkan-headers clang -y
    vkcube
    echo "All Dependiences Installed Successfully."
else
    echo "Couldn't find known package manager..."
fi

