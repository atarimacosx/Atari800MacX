#!/bin/bash

# Simple test build script

set -e

APP_NAME="Atari800MacX"
BUILD_CONFIG="Development"
VERSION="7.0.0"

# Paths
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$SCRIPT_DIR/atari800-MacOSX/src/Atari800MacX"
BUILD_DIR="$SCRIPT_DIR/build"
APP_PATH="$BUILD_DIR/$BUILD_CONFIG/$APP_NAME.app"

echo "=== Test Build for $APP_NAME $VERSION ==="

# Clean and build
echo "Step 1: Building app..."
cd "$PROJECT_DIR"
rm -rf "$BUILD_DIR"

xcodebuild -project "$APP_NAME.xcodeproj" \
    -scheme "$APP_NAME" \
    -configuration "$BUILD_CONFIG" \
    -derivedDataPath "$BUILD_DIR" \
    clean build

echo "Step 2: Verifying code signing..."
if [ -d "$APP_PATH" ]; then
    codesign --verify --deep --strict --verbose=2 "$APP_PATH"
    echo "✓ App built and signed successfully!"
    echo "Location: $APP_PATH"
    echo ""
    echo "You can test it with: open '$APP_PATH'"
else
    echo "❌ App not found at: $APP_PATH"
    exit 1
fi