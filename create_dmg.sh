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
APP_PATH="$BUILD_DIR/Development/$APP_NAME.app"
DMG_PATH="$RELEASE_DIR/$DMG_NAME.dmg"
TEMP_DMG_PATH="$RELEASE_DIR/temp.dmg"
MOUNT_DIR="/Volumes/$VOLUME_NAME"

# Ensure directories exist
mkdir -p "$RELEASE_DIR"

# Remove old DMG if exists
rm -f "$DMG_PATH"
rm -f "$TEMP_DMG_PATH"

echo "Creating DMG for $APP_NAME..."

# Create a temporary DMG
echo "Step 1: Creating temporary DMG..."
hdiutil create -size 200m -fs HFS+ -volname "$VOLUME_NAME" "$TEMP_DMG_PATH"

# Mount the temporary DMG
echo "Step 2: Mounting temporary DMG..."
hdiutil attach "$TEMP_DMG_PATH"

# Copy the app to the DMG
echo "Step 3: Copying $APP_NAME to DMG..."
cp -R "$APP_PATH" "$MOUNT_DIR/"

# Create a symbolic link to Applications
echo "Step 4: Creating Applications symlink..."
ln -s /Applications "$MOUNT_DIR/Applications"

# Create a background image directory (optional)
mkdir "$MOUNT_DIR/.background"

# Copy release notes if they exist
if [ -f "$SCRIPT_DIR/RELEASE_NOTES_7.0.0_BETA1.md" ]; then
    cp "$SCRIPT_DIR/RELEASE_NOTES_7.0.0_BETA1.md" "$MOUNT_DIR/Release Notes.md"
fi

# Set custom icon positions (optional)
echo "Step 5: Setting DMG window properties..."
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
           set position of item "'$APP_NAME'.app" of container window to {100, 170}
           set position of item "Applications" of container window to {375, 170}
           if exists item "Release Notes.md" then
              set position of item "Release Notes.md" of container window to {240, 280}
           end if
           close
           open
           update without registering applications
           delay 2
     end tell
   end tell
' | osascript

# Unmount the DMG
echo "Step 6: Unmounting temporary DMG..."
hdiutil detach "$MOUNT_DIR"

# Convert to compressed DMG
echo "Step 7: Creating final compressed DMG..."
hdiutil convert "$TEMP_DMG_PATH" -format UDZO -o "$DMG_PATH"

# Clean up
echo "Step 8: Cleaning up..."
rm -f "$TEMP_DMG_PATH"

echo "✓ DMG created successfully: $DMG_PATH"