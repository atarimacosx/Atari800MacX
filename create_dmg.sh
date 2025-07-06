#!/bin/bash

# DMG Creation Script for Atari800MacX

set -e

# Configuration
APP_NAME="Atari800MacX"
VERSION="7.0.0"
DMG_NAME="$APP_NAME-$VERSION-beta1"
VOLUME_NAME="Atari800MacX 7.0.0 Beta 1"

# Paths
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build"
RELEASE_DIR="$SCRIPT_DIR/release"
# Use the app path passed as environment variable from build script, or find it
if [ -n "$BUILT_APP_PATH" ]; then
    APP_PATH="$BUILT_APP_PATH"
else
    # Find the actual build path from DerivedData
    DERIVED_DATA_PATH="$HOME/Library/Developer/Xcode/DerivedData"
    APP_PATH=$(find "$DERIVED_DATA_PATH" -name "$APP_NAME.app" -path "*/Build/Products/Development/*" | head -1)
fi
DMG_PATH="$RELEASE_DIR/$DMG_NAME.dmg"
TEMP_DMG_PATH="$RELEASE_DIR/temp.dmg"
MOUNT_DIR="/Volumes/$VOLUME_NAME"

# Ensure directories exist
mkdir -p "$RELEASE_DIR"

# Check if app exists
if [ ! -d "$APP_PATH" ]; then
    echo "Error: App not found at $APP_PATH"
    echo "Please build the app first or check the path"
    exit 1
fi

echo "Using app at: $APP_PATH"

# Remove old DMG if exists
rm -f "$DMG_PATH"
rm -f "$TEMP_DMG_PATH"

# Unmount any existing volumes with this name
hdiutil detach "$MOUNT_DIR" 2>/dev/null || true

echo "Creating DMG for $APP_NAME..."

# Create a temporary DMG
echo "Step 1: Creating temporary DMG..."
hdiutil create -size 200m -fs HFS+ -volname "$VOLUME_NAME" "$TEMP_DMG_PATH"

# Mount the temporary DMG
echo "Step 2: Mounting temporary DMG..."
hdiutil attach "$TEMP_DMG_PATH"

# Create the main Atari800MacX folder structure
echo "Step 3: Creating folder structure..."
mkdir -p "$MOUNT_DIR/Atari800MacX"

# Copy the app to the Atari800MacX folder
echo "Step 4: Copying $APP_NAME to DMG..."
cp -R "$APP_PATH" "$MOUNT_DIR/Atari800MacX/"

# Create the directory structure that users expect
echo "Step 5: Creating directory structure..."
mkdir -p "$MOUNT_DIR/Atari800MacX/AtariExeFiles"
mkdir -p "$MOUNT_DIR/Atari800MacX/Carts"
mkdir -p "$MOUNT_DIR/Atari800MacX/Disks/Sets"
mkdir -p "$MOUNT_DIR/Atari800MacX/HardDrive1"
mkdir -p "$MOUNT_DIR/Atari800MacX/HardDrive2"
mkdir -p "$MOUNT_DIR/Atari800MacX/HardDrive3"
mkdir -p "$MOUNT_DIR/Atari800MacX/HardDrive4"
mkdir -p "$MOUNT_DIR/Atari800MacX/OSRoms"
mkdir -p "$MOUNT_DIR/Atari800MacX/Palettes"
mkdir -p "$MOUNT_DIR/Atari800MacX/SavedState"

# Copy palette files
echo "Step 6: Copying palette files..."
if [ -d "$SCRIPT_DIR/atari800-MacOSX/act" ]; then
    cp "$SCRIPT_DIR/atari800-MacOSX/act"/*.act "$MOUNT_DIR/Atari800MacX/Palettes/" 2>/dev/null || true
fi

# Create custom folder icons (using Icon\r files like the original)
echo "Step 7: Adding folder icons..."
# Create invisible Icon files for folder customization
touch "$MOUNT_DIR/Atari800MacX/Icon"$'\r'
touch "$MOUNT_DIR/Atari800MacX/AtariExeFiles/Icon"$'\r'
touch "$MOUNT_DIR/Atari800MacX/Carts/Icon"$'\r'
touch "$MOUNT_DIR/Atari800MacX/Disks/Icon"$'\r'
touch "$MOUNT_DIR/Atari800MacX/Disks/Sets/Icon"$'\r'
touch "$MOUNT_DIR/Atari800MacX/HardDrive1/Icon"$'\r'
touch "$MOUNT_DIR/Atari800MacX/HardDrive2/Icon"$'\r'
touch "$MOUNT_DIR/Atari800MacX/HardDrive3/Icon"$'\r'
touch "$MOUNT_DIR/Atari800MacX/HardDrive4/Icon"$'\r'
touch "$MOUNT_DIR/Atari800MacX/OSRoms/Icon"$'\r'
touch "$MOUNT_DIR/Atari800MacX/Palettes/Icon"$'\r'
touch "$MOUNT_DIR/Atari800MacX/SavedState/Icon"$'\r'

# Make the Icon files invisible
chflags hidden "$MOUNT_DIR/Atari800MacX/Icon"$'\r' 2>/dev/null || true
chflags hidden "$MOUNT_DIR/Atari800MacX/AtariExeFiles/Icon"$'\r' 2>/dev/null || true
chflags hidden "$MOUNT_DIR/Atari800MacX/Carts/Icon"$'\r' 2>/dev/null || true
chflags hidden "$MOUNT_DIR/Atari800MacX/Disks/Icon"$'\r' 2>/dev/null || true
chflags hidden "$MOUNT_DIR/Atari800MacX/Disks/Sets/Icon"$'\r' 2>/dev/null || true
chflags hidden "$MOUNT_DIR/Atari800MacX/HardDrive1/Icon"$'\r' 2>/dev/null || true
chflags hidden "$MOUNT_DIR/Atari800MacX/HardDrive2/Icon"$'\r' 2>/dev/null || true
chflags hidden "$MOUNT_DIR/Atari800MacX/HardDrive3/Icon"$'\r' 2>/dev/null || true
chflags hidden "$MOUNT_DIR/Atari800MacX/HardDrive4/Icon"$'\r' 2>/dev/null || true
chflags hidden "$MOUNT_DIR/Atari800MacX/OSRoms/Icon"$'\r' 2>/dev/null || true
chflags hidden "$MOUNT_DIR/Atari800MacX/Palettes/Icon"$'\r' 2>/dev/null || true
chflags hidden "$MOUNT_DIR/Atari800MacX/SavedState/Icon"$'\r' 2>/dev/null || true

# Create README file
echo "Step 8: Creating README..."
cat > "$MOUNT_DIR/README.rtf" << 'EOF'
{\rtf1\ansi\ansicpg1252\cocoartf2709
{\fonttbl\f0\fswiss\fcharset0 Helvetica;}
{\colortbl;\red255\green255\blue255;}
\margl1440\margr1440\vieww10220\viewh10840\viewkind0
\pard\tx1440\tx2880\tx4320\tx5760\tx7200\ql\qnatural

\f0\b\fs24 \cf0 Atari800MacX 7.0.0 Beta 1
\b0

This is the macOS port of the Atari800 emulator with a native Cocoa interface, including preferences, menus, file associations, help and more.

\b \ul Installation:\

\b0 \ulnone \
Just drag the Atari800MacX folder to your Applications folder or anywhere on your hard drive.\

\b \ul New in Version 7.0.0:\

\b0 \ulnone \
- FujiNet Network Device Support (NetSIO)\
- Automatic fallback for ROM loading issues\
- Updated build system and distribution\
- Enhanced compatibility and stability\

\b \ul Instructions:\

\b0 \ulnone \
The online manual found under Help explains the menus and use of the emulator. Visit http://www.atarimac.com for more information.\

\b \ul Bug Reports:\

\b0 \ulnone \
Please report bugs or feature requests at: https://github.com/atarimacosx/Atari800MacX\
}
EOF

# Copy release notes if they exist
#if [ -f "$SCRIPT_DIR/RELEASE_NOTES_7.0.0_BETA1.md" ]; then
#    cp "$SCRIPT_DIR/RELEASE_NOTES_7.0.0_BETA1.md" "$MOUNT_DIR/Release Notes.md"
#fi

# Create a background image directory (optional)
mkdir "$MOUNT_DIR/.background"

# Set custom icon positions (optional)
echo "Step 9: Setting DMG window properties..."
echo '
   tell application "Finder"
     tell disk "'$VOLUME_NAME'"
           open
           set current view of container window to icon view
           set toolbar visible of container window to false
           set statusbar visible of container window to false
           set the bounds of container window to {400, 100, 920, 440}
           set viewOptions to the icon view options of container window
           set arrangement of viewOptions to not arranged
           set icon size of viewOptions to 72
           set position of item "Atari800MacX" of container window to {180, 170}
           if exists item "README.rtf" then
              set position of item "README.rtf" of container window to {350, 170}
           end if
           if exists item "Release Notes.md" then
              set position of item "Release Notes.md" of container window to {265, 280}
           end if
           close
           open
           update without registering applications
           delay 2
     end tell
   end tell
' | osascript

# Unmount the DMG
echo "Step 10: Unmounting temporary DMG..."
hdiutil detach "$MOUNT_DIR"

# Convert to compressed DMG
echo "Step 11: Creating final compressed DMG..."
hdiutil convert "$TEMP_DMG_PATH" -format UDZO -o "$DMG_PATH"

# Clean up
echo "Step 12: Cleaning up..."
rm -f "$TEMP_DMG_PATH"

echo "✓ DMG created successfully: $DMG_PATH"
