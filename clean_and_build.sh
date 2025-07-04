#!/bin/bash

# Clean and Build Script - Handles resource fork issues

set -e

APP_NAME="Atari800MacX"
BUILD_CONFIG="Deployment"
VERSION="7.0.0"

# Paths
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$SCRIPT_DIR/atari800-MacOSX/src/Atari800MacX"
BUILD_DIR="$SCRIPT_DIR/build"
APP_PATH="$BUILD_DIR/Build/Products/$BUILD_CONFIG/$APP_NAME.app"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}=== Clean Build for $APP_NAME $VERSION ===${NC}"

# Step 1: Deep clean the source directory
echo -e "${YELLOW}Step 1: Deep cleaning source directory...${NC}"
cd "$PROJECT_DIR"

# Remove all extended attributes recursively
find . -type f -exec xattr -c {} \; 2>/dev/null || true

# Remove resource forks and other macOS metadata
find . \( -name "._*" -o -name ".DS_Store" -o -name "__MACOSX" \) -delete 2>/dev/null || true

# Clean frameworks specifically
if [ -d "SDL2.framework" ]; then
    echo "Cleaning SDL2.framework..."
    xattr -cr SDL2.framework || true
    find SDL2.framework -name "._*" -delete 2>/dev/null || true
fi

# Step 2: Clean build directories
echo -e "${YELLOW}Step 2: Cleaning previous builds...${NC}"
rm -rf "$BUILD_DIR"
rm -rf build  # Local build dir in project

# Step 3: Build without signing first
echo -e "${YELLOW}Step 3: Building app without code signing...${NC}"
xcodebuild -project "$APP_NAME.xcodeproj" \
    -scheme "$APP_NAME" \
    -configuration "$BUILD_CONFIG" \
    -derivedDataPath "$BUILD_DIR" \
    CODE_SIGN_IDENTITY="" \
    CODE_SIGNING_REQUIRED=NO \
    clean build

# Step 4: Clean the built app
echo -e "${YELLOW}Step 4: Cleaning built app bundle...${NC}"
if [ -d "$APP_PATH" ]; then
    echo "Cleaning app bundle: $APP_PATH"
    
    # Remove extended attributes from entire app bundle
    xattr -cr "$APP_PATH"
    
    # Remove resource forks and metadata files
    find "$APP_PATH" \( -name "._*" -o -name ".DS_Store" -o -name "__MACOSX" \) -delete 2>/dev/null || true
    
    # Clean frameworks in the app bundle
    if [ -d "$APP_PATH/Contents/Frameworks" ]; then
        find "$APP_PATH/Contents/Frameworks" -type f -exec xattr -c {} \; 2>/dev/null || true
        find "$APP_PATH/Contents/Frameworks" -name "._*" -delete 2>/dev/null || true
    fi
    
    echo -e "${GREEN}✓ App bundle cleaned${NC}"
else
    echo -e "${RED}❌ App not found at: $APP_PATH${NC}"
    exit 1
fi

# Step 5: Manual code signing
echo -e "${YELLOW}Step 5: Manually signing app bundle...${NC}"

# Sign frameworks first
if [ -d "$APP_PATH/Contents/Frameworks" ]; then
    echo "Signing frameworks..."
    find "$APP_PATH/Contents/Frameworks" -name "*.framework" -type d | while read framework; do
        echo "Signing: $framework"
        codesign --force --sign "Apple Development: Paulo Garcia (952JHD69PH)" --timestamp=none "$framework" || true
    done
fi

# Sign the main app
echo "Signing main app..."
codesign --force --sign "Apple Development: Paulo Garcia (952JHD69PH)" \
    --timestamp=none \
    --options runtime \
    "$APP_PATH"

# Step 6: Verify signature
echo -e "${YELLOW}Step 6: Verifying signature...${NC}"
codesign --verify --deep --strict --verbose=2 "$APP_PATH"

echo -e "${GREEN}✓ Build and signing complete!${NC}"
echo "App location: $APP_PATH"
echo ""
echo "Next steps:"
echo "1. Test the app: open '$APP_PATH'"
echo "2. If it works, run ./build_release.sh for full DMG creation"