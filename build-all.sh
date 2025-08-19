#!/bin/bash
#Build script for rebuilding everything
set echo on

echo "Installing Dependiences..."
sudo dnf install libX11-devel -y
sudo dnf install libxkbcommon-x11-devel -y
sudo apt-get install libx11-dev -y
sudo apt-get install libxkbcommon-x11-dev -y

echo "Building everything..."

pushd engine
source build.sh
popd

ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit 
fi 

pushd testbed
source build.sh
popd
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi

echo "All assemblies built successfully."