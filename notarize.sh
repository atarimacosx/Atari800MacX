#!/bin/bash

# Notarization Script for Atari800MacX
# Usage: ./notarize.sh <path-to-dmg>

set -e

# Check parameters
if [ $# -eq 0 ]; then
    echo "Usage: $0 <path-to-dmg>"
    exit 1
fi

DMG_PATH="$1"
BUNDLE_ID="com.atarimac.atari800macx"
TEAM_ID="ZC4PGBX6D6"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}Starting notarization process...${NC}"
echo "File: $DMG_PATH"

# Check if DMG exists
if [ ! -f "$DMG_PATH" ]; then
    echo -e "${RED}Error: DMG file not found!${NC}"
    exit 1
fi

# Get Apple ID from keychain or prompt
echo -e "${YELLOW}Enter your Apple ID email for notarization:${NC}"
read -p "Apple ID: " APPLE_ID

# Submit for notarization
echo -e "${YELLOW}Submitting to Apple for notarization...${NC}"
echo "(This may take several minutes)"

# Use notarytool (new method for Xcode 13+)
xcrun notarytool submit "$DMG_PATH" \
    --apple-id "$APPLE_ID" \
    --team-id "$TEAM_ID" \
    --wait \
    --keychain-profile "AC_PASSWORD" 2>&1 | tee notarize.log

# Check if successful
if grep -q "Successfully received submission" notarize.log; then
    echo -e "${GREEN}✓ Notarization submitted successfully${NC}"
    
    # Extract submission ID
    SUBMISSION_ID=$(grep "id:" notarize.log | head -1 | awk '{print $2}')
    echo "Submission ID: $SUBMISSION_ID"
    
    # Wait for notarization to complete
    echo -e "${YELLOW}Waiting for notarization to complete...${NC}"
    
    # Check status
    xcrun notarytool wait "$SUBMISSION_ID" \
        --apple-id "$APPLE_ID" \
        --team-id "$TEAM_ID" \
        --keychain-profile "AC_PASSWORD"
    
    # Get final status
    xcrun notarytool info "$SUBMISSION_ID" \
        --apple-id "$APPLE_ID" \
        --team-id "$TEAM_ID" \
        --keychain-profile "AC_PASSWORD"
    
    echo -e "${GREEN}✓ Notarization complete!${NC}"
else
    echo -e "${RED}✗ Notarization failed${NC}"
    echo "Check notarize.log for details"
    exit 1
fi

# Clean up
rm -f notarize.log

echo -e "${GREEN}✓ Notarization successful!${NC}"
echo ""
echo "Note: If this is your first time notarizing:"
echo "1. You may need to create an app-specific password at https://appleid.apple.com"
echo "2. Store it in keychain with:"
echo "   xcrun notarytool store-credentials \"AC_PASSWORD\" --apple-id \"$APPLE_ID\" --team-id \"$TEAM_ID\""