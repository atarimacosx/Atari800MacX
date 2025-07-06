#!/bin/bash

# Atari800MacX Build and Release Script
# This script builds, signs, and notarizes the app for distribution

set -e  # Exit on error

# Configuration
APP_NAME="Atari800MacX"
BUNDLE_ID="com.atarimac.atari800macx"
DEVELOPER_ID="Apple Development: Paulo Garcia (952JHD69PH)"
TEAM_ID="ZC4PGBX6D6"
VERSION="7.0.0"
BUILD_CONFIG="Development"

# Paths
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$SCRIPT_DIR/atari800-MacOSX/src/Atari800MacX"
BUILD_DIR="$SCRIPT_DIR/build"
RELEASE_DIR="$SCRIPT_DIR/release"
# Find the actual build path from xcodebuild
DERIVED_DATA_PATH="$HOME/Library/Developer/Xcode/DerivedData"
APP_PATH=$(find "$DERIVED_DATA_PATH" -name "$APP_NAME.app" -path "*/Build/Products/$BUILD_CONFIG/*" | head -1)
DMG_NAME="$APP_NAME-$VERSION-beta1"
DMG_PATH="$RELEASE_DIR/$DMG_NAME.dmg"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Atari800MacX Build and Release Script ===${NC}"
echo "Version: $VERSION Beta 1"
echo ""

# Step 1: Clean previous builds
echo -e "${YELLOW}Step 1: Cleaning previous builds...${NC}"
rm -rf "$BUILD_DIR"
rm -rf "$RELEASE_DIR"
mkdir -p "$RELEASE_DIR"

# Step 2: Build the app
echo -e "${YELLOW}Step 2: Building $APP_NAME...${NC}"
cd "$PROJECT_DIR"

# Clean extended attributes thoroughly
echo "Cleaning extended attributes from source..."
xattr -cr .
find . -name "._*" -delete 2>/dev/null || true
find . -name ".DS_Store" -delete 2>/dev/null || true

xcodebuild -project "$APP_NAME.xcodeproj" \
    -scheme "$APP_NAME" \
    -configuration "$BUILD_CONFIG" \
    clean build

# Update APP_PATH after build
APP_PATH=$(find "$DERIVED_DATA_PATH" -name "$APP_NAME.app" -path "*/Build/Products/$BUILD_CONFIG/*" | head -1)

# Check if app was built successfully
if [ ! -d "$APP_PATH" ]; then
    echo -e "${RED}Error: App not found at expected location${NC}"
    echo "Searched in: $DERIVED_DATA_PATH"
    echo "Looking for: $APP_NAME.app in */Build/Products/$BUILD_CONFIG/*"
    exit 1
fi

echo "Found app at: $APP_PATH"

# Clean extended attributes from built app more thoroughly
if [ -d "$APP_PATH" ]; then
    echo "Thoroughly cleaning extended attributes from built app..."
    xattr -cr "$APP_PATH"
    find "$APP_PATH" -name "._*" -delete 2>/dev/null || true
    find "$APP_PATH" -name ".DS_Store" -delete 2>/dev/null || true
    find "$APP_PATH" -name "*.dSYM" -exec xattr -cr {} \; 2>/dev/null || true
    
    # Remove any resource fork data specifically
    find "$APP_PATH" -type f -exec xattr -d com.apple.ResourceFork {} \; 2>/dev/null || true
    find "$APP_PATH" -type f -exec xattr -d com.apple.FinderInfo {} \; 2>/dev/null || true
fi

# Step 3: Verify code signing
echo -e "${YELLOW}Step 3: Verifying code signature...${NC}"
codesign --verify --deep --strict --verbose=2 "$APP_PATH"
echo -e "${GREEN}✓ Code signature verified${NC}"

# Step 4: Create DMG
echo -e "${YELLOW}Step 4: Creating DMG...${NC}"
BUILT_APP_PATH="$APP_PATH" "$SCRIPT_DIR/create_dmg.sh"

# Step 5: Sign the DMG
echo -e "${YELLOW}Step 5: Signing DMG...${NC}"
codesign --force --sign "$DEVELOPER_ID" "$DMG_PATH"
echo -e "${GREEN}✓ DMG signed${NC}"

# Step 6: Notarize the DMG
echo -e "${YELLOW}Step 6: Notarizing DMG with Apple...${NC}"
"$SCRIPT_DIR/notarize.sh" "$DMG_PATH"

# Step 7: Staple the notarization ticket
echo -e "${YELLOW}Step 7: Stapling notarization ticket...${NC}"
xcrun stapler staple "$DMG_PATH"
echo -e "${GREEN}✓ Notarization ticket stapled${NC}"

# Step 8: Verify everything
echo -e "${YELLOW}Step 8: Final verification...${NC}"
spctl -a -t open --context context:primary-signature -v "$DMG_PATH"
echo -e "${GREEN}✓ DMG verified and ready for distribution${NC}"

echo ""
echo -e "${GREEN}=== Build Complete! ===${NC}"
echo "Release file: $DMG_PATH"
echo ""
echo "Next steps:"
echo "1. Test the DMG on a clean Mac"
echo "2. Upload to GitHub releases"
echo "3. Share the download link with beta testers"