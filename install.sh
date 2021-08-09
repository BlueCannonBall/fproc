#!/bin/bash
set -e
trap "echo 'Failed to install fproc :('" EXIT

echo "Installing fproc..."

cp ./daemon/fprocd /usr/local/bin
cp ./cli/target/release/fproc /usr/local/bin
cp ./gui/fproc-gui /usr/local/bin
mkdir -p /usr/local/share/applications
cp ./gui/fproc.desktop /usr/local/share/applications
mkdir -p /usr/local/share/icons/hicolor/48x48/apps
cp icon.png /usr/local/share/icons/hicolor/48x48/apps/fproc.png

echo "Successfully installed fproc!"
trap - EXIT
