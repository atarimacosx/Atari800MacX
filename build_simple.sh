#!/bin/bash

# Simple build script without notarization for testing

set -e

APP_NAME="Atari800MacX"
VERSION="7.0.0"
BUILD_CONFIG="Deployment"

# Paths
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$SCRIPT_DIR/atari800-MacOSX/src/Atari800MacX"
BUILD_DIR="$SCRIPT_DIR/build"
APP_PATH="$BUILD_DIR/Build/Products/$BUILD_CONFIG/$APP_NAME.app"

echo "=== Simple Build for Testing ==="
echo "Version: $VERSION Beta 1"

# Clean and build
echo "Step 1: Cleaning previous builds..."
rm -rf "$BUILD_DIR"

echo "Step 2: Building app without signing..."
cd "$PROJECT_DIR"

# Build without code signing first
xcodebuild -project "$APP_NAME.xcodeproj" \
    -scheme "$APP_NAME" \
    -configuration "$BUILD_CONFIG" \
    -derivedDataPath "$BUILD_DIR" \
    CODE_SIGN_IDENTITY="" \
    CODE_SIGNING_REQUIRED=NO \
    clean build

echo "✓ Build complete!"
echo "App location: $APP_PATH"

# Test the app
if [ -d "$APP_PATH" ]; then
    echo ""
    echo "App built successfully. You can now:"
    echo "1. Test run: open '$APP_PATH'"
    echo "2. Sign manually if needed"
    echo "3. Create DMG if everything works"
else
    echo "❌ App not found at expected location"
    exit 1
fi