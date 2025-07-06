# Atari800MacX Upgrade Analysis & Strategy

## Project Overview

**Atari800MacX** is a sophisticated macOS port of the cross-platform Atari800 emulator. It wraps the core Atari800 emulation engine with a native Cocoa interface, providing Mac-specific features while maintaining the accuracy and performance of the original emulator.

## Version 7.0.0 Development Notes

### Video Feature Changes Analysis (July 2025)

**CRITICAL**: During 7.0.0 development, significant video/NTSC filter changes were made. Before finalizing the release, verify what video features were already available in the previous Atari800MacX version.

#### Changes Made in 7.0.0 (Commits b718228 → e07bd03):

**NTSC Filter Integration:**
- Added NTSC_FILTER and PAL_BLENDING preprocessor definitions
- Integrated with new `artifact.h` system from Atari800 core
- Replaced legacy `ANTIC_UpdateArtifacting` with `ARTIFACT_Set()` API
- Added NTSC filter preset support (Composite, S-Video, RGB, Monochrome)

**UI Enhancements:**
- **Display Menu**: Added NTSC Filter Presets submenu
- **Preferences**: TV mode-aware artifact dropdown (NTSC vs PAL options)
- **Dynamic Menus**: Menu items enable/disable based on current TV mode
- **Persistent Settings**: Separate preferences for NTSC/PAL artifact modes

**New Artifact Modes:**
- NTSC-NEW, NTSC-FULL (full NTSC filter)
- PAL-SIMPLE, PAL-BLEND (PAL blending modes)

**Files Modified:**
- `DisplayManager.h/m` - Major changes to artifact handling
- `Preferences.h/m` - TV mode-aware artifact selection
- `atari_mac_sdl.c` - Core artifact system integration
- `SDLMain.nib` - Interface Builder connections for new menus

#### Core Version Facts (from official Atari800 releases):
- **Atari800 4.1.0 (April 2019)**: Built-in Altirra ROMs introduced
- **Atari800 4.2.0 (December 2019)**: Pokey recording, libatari800, R: device
- **Atari800 5.0.0 (May 2022)**: AVI recording, "gamma values in NTSC filter presets updated" (indicates NTSC filters existed before)
- **Current**: Atari800 5.2.0 (December 2023)

#### ACTION REQUIRED:
**Test the previous Atari800MacX version 6.x to determine:**
1. Did it have NTSC filter presets in the Display menu?
2. Did it support NTSC-FULL, PAL-BLEND artifact modes?
3. Did it have TV mode-aware artifact selection?

**If YES** → Consider reverting video changes as redundant
**If NO** → Changes are legitimate UI improvements exposing existing core features

#### Revert Commands (if needed):
```bash
git revert b718228..e07bd03
# Or selectively revert specific commits:
git revert e07bd03 ee25e43 674221c 8b9450f 4ca8a09 54ae172 043efaf 7b44962 b718228
```

### Current Architecture

```
┌─────────────────────────────────────────┐
│           Native macOS UI               │
│        (Cocoa/Objective-C)             │
├─────────────────────────────────────────┤
│         Mac Integration Layer           │
│     (C/Objective-C Bridge Code)        │
├─────────────────────────────────────────┤
│            SDL2 Framework              │
│   (Cross-platform Graphics/Audio)      │
├─────────────────────────────────────────┤
│         Atari800 Core Engine           │
│        (Cross-platform C Code)         │
└─────────────────────────────────────────┘
```

## Current State Analysis

### Atari800MacX Version Information
- **Last Updated**: January 2022 (commit e611f03)
- **Core Version**: Based on older Atari800 codebase (~2014-2016 era)
- **Recent Changes**: Focused on Mac-specific fixes and features
  - Fix fullscreen garbage issue
  - Paste delay improvements for BASIC
  - ARM target codesigning
  - Configuration loading fixes

### Newer Atari800 Version Information
- **Version**: 4.2.0 (released 2019-12-28) + 8 commits
- **Last Activity**: January 2020 (minimal recent activity)
- **Key Features**: Enhanced video, audio, and platform support

## Critical Differences & Upgrade Requirements

### 1. Video System Overhaul (HIGH PRIORITY)

#### Current MacX Video Pipeline
```
Atari Core → Screen Buffer → SDL → OpenGL → macOS Display
```

#### New Video System Features
- **NTSC Filter System** (`filter_ntsc.c/h`, `atari_ntsc/`)
  - Composite, S-Video, RGB, Monochrome presets
  - Adjustable sharpness, resolution, artifacts, fringing, bleed
- **Enhanced Artifacting** (`artifact.c/h`)
  - Multiple modes: NONE, NTSC-OLD, NTSC-NEW, NTSC-FULL, PAL-SIMPLE, PAL-BLEND
- **Separate Color Management** 
  - `colours_ntsc.c/h` - NTSC-specific palettes
  - `colours_pal.c/h` - PAL-specific palettes
  - `colours_external.c/h` - External palette support
- **PAL Blending** (`pal_blending.c/h`)
- **Video Mode Management** (`videomode.c/h`)

#### Integration Impact
- **Major API Changes**: Color initialization now split into NTSC/PAL variants
- **New Dependencies**: NTSC filter library integration required
- **Performance**: Additional processing overhead for filters
- **Configuration**: New video settings need Mac UI integration

### 2. Audio Enhancements (MEDIUM PRIORITY)

#### New Audio Features
- **POKEY Recording** (`pokeyrec.c/h`)
  - Record POKEY register changes to files
  - Command line option: `-pokeyrec`
- **Voice Box Emulation** (`voicebox.c/h`)
  - Speech synthesizer support
- **Enhanced Votrax** (`votraxsnd.c/h`)
  - Improved speech synthesis

#### Mac Integration Requirements
- Audio recording UI controls
- File save dialogs for POKEY recordings
- Audio preferences integration

### 3. ROM System Updates (HIGH PRIORITY)

#### Built-in ROM Support
The newer version includes built-in **Altirra ROMs**:
- `roms/altirraos_800.c/h` - AltirraOS for 400/800
- `roms/altirraos_xl.c/h` - AltirraOS for XL/XE
- `roms/altirra_basic.c/h` - Altirra BASIC
- `roms/altirra_5200_os.c/h` - Altirra 5200 BIOS

#### Benefits
- **Reduced Setup Complexity**: No need for users to provide ROM files
- **Legal Distribution**: Altirra ROMs are freely redistributable
- **SIO Patch Support**: Enhanced compatibility with Altirra ROMs

### 4. FujiNet Support Status

#### Major Discovery: NetSIO Implementation
**✅ COMPREHENSIVE FUJINET SUPPORT** found via NetSIO system:
- **Full FujiNet-PC Integration**: Complete UDP-based communication system
- **Threading Architecture**: Dedicated receiver thread (`fujinet_rx_thread`)
- **Protocol Implementation**: 20+ command types for device communication
- **FIFO Buffer System**: 4KB buffers for bidirectional data transfer

#### NetSIO Technical Architecture
**Core Files**:
- `src/netsio.h/c` - Complete NetSIO implementation (710+ lines)
- `src/sio.c` - Enhanced with NetSIO integration (`#ifdef NETSIO`)
- Build support via `--enable-netsio` configure option

**Key Features**:
- **UDP Socket Communication**: Connects to FujiNet-PC on configurable port
- **Connection Management**: Device connect/disconnect, ping/alive protocols
- **Data Transfer**: Byte-level and block-level data transmission
- **Synchronization**: Command synchronization with FujiNet-PC
- **Large Buffer Support**: 65KB+ buffers for FujiNet operations
- **Cross-Platform**: Linux, macOS, Unix support

**Protocol Commands**:
```c
#define NETSIO_DATA_BYTE          0x01    // Send single byte
#define NETSIO_DATA_BLOCK         0x02    // Send data block  
#define NETSIO_COMMAND_ON         0x11    // Command line control
#define NETSIO_DEVICE_CONNECTED   0xC1    // Device management
#define NETSIO_PING_REQUEST       0xC2    // Connection monitoring
#define NETSIO_SYNC_RESPONSE      0x81    // Command synchronization
```

#### Integration with SIO System
**Conditional Compilation**: 
```c
#ifdef NETSIO
    // Enhanced SIO functions with NetSIO support
    // Modified SIO_PutByte() and SIO_GetByte()
    // Large buffer allocation for FujiNet data
#endif
```

#### Enhanced R: Device (Also Available)
The newer Atari800 also includes enhanced **R: device** (`rdevice.c`):
- **Network Support**: TCP/IP connections via host networking  
- **Telnet Capability**: `ATDI <hostname> <port>` commands
- **Serial Port Support**: Physical serial port access
- **BBS Compatibility**: Works with terminal programs

### 5. Build System & Configuration Changes

#### New Configuration Options
```c
#define NTSC_FILTER          // Enable NTSC video filtering  
#define PAL_BLENDING         // Enable PAL blending
#define POKEYREC            // Enable POKEY recording
#define VOICEBOX            // Enable Voice Box emulation
#define NETSIO              // Enable FujiNet NetSIO support
#define LIBATARI800         // Build as library
```

#### Mac-Specific Requirements
- Update Xcode project with new source files
- Configure conditional compilation flags
- Update framework dependencies
- Integrate new build options into Mac preferences

## Upgrade Strategy & Implementation Plan

### Phase 1: Core Engine Update (Foundation)
**Priority: HIGH | Effort: HIGH | Risk: HIGH**

#### 1.1 Source File Integration
- [ ] Add new core files to Xcode project:
  - `artifact.c/h`, `filter_ntsc.c/h`, `pal_blending.c/h`
  - `colours_ntsc.c/h`, `colours_pal.c/h`, `colours_external.c/h`
  - `videomode.c/h`, `pokeyrec.c/h`, `voicebox.c/h`, `votraxsnd.c/h`
  - `netsio.c/h` - **FujiNet NetSIO support**
  - Complete `atari_ntsc/` directory

#### 1.2 Core File Updates  
- [ ] Update major core files:
  - `atari.c` (1,513 lines vs 1,062 - 43% larger)
  - `antic.c`, `gtia.c`, `pokey.c` - enhanced functionality
  - `colours.c` - major refactoring for NTSC/PAL separation

#### 1.3 API Compatibility
- [ ] Update Mac integration layer (`atari_mac_sdl.c`) for new APIs
- [ ] Modify initialization sequences for new subsystems
- [ ] Update color management calls for NTSC/PAL separation

#### 1.4 Testing & Validation
- [ ] Ensure basic emulation functionality works
- [ ] Verify existing Mac features still function
- [ ] Test with various ROM types and game/software

### Phase 2: Enhanced Video Features (User-Visible Improvements)
**Priority: HIGH | Effort: MEDIUM | Risk: MEDIUM**

#### 2.1 NTSC Filter Integration
- [ ] Add NTSC filter controls to Mac preferences
- [ ] Implement preset management (Composite, S-Video, RGB, etc.)
- [ ] Add real-time filter adjustment UI
- [ ] Performance optimization for Mac hardware

#### 2.2 Video Mode Enhancements
- [ ] Dynamic video mode switching support
- [ ] Enhanced artifacting controls
- [ ] PAL blending options
- [ ] External palette loading UI

#### 2.3 Display Menu Updates
- [ ] Update Display menu with new video options
- [ ] Add filter presets to menu system
- [ ] Implement video mode switching shortcuts

### Phase 3: ROM System Modernization (Setup Simplification)
**Priority: HIGH | Effort: LOW | Risk: LOW**

#### 3.1 Built-in ROM Integration
- [ ] Add Altirra ROM files to Xcode project
- [ ] Update ROM selection UI to include built-in options
- [ ] Implement automatic ROM fallback system
- [ ] Update first-run experience to use built-in ROMs

#### 3.2 ROM Management
- [ ] Enhanced ROM path detection
- [ ] Improved ROM validation and error reporting
- [ ] Support for multiple ROM sets

### Phase 4: Audio Enhancements (Nice-to-Have Features)
**Priority: MEDIUM | Effort: MEDIUM | Risk: LOW**

#### 4.1 POKEY Recording
- [ ] Add recording controls to Audio preferences
- [ ] Implement file save dialogs for recordings
- [ ] Add menu items for start/stop recording
- [ ] Format selection (if multiple formats supported)

#### 4.2 Voice Box Support
- [ ] Add Voice Box emulation toggle
- [ ] Integrate with existing audio system
- [ ] Add configuration options if needed

### Phase 5: FujiNet NetSIO Integration (High-Value Feature)
**Priority: MEDIUM | Effort: MEDIUM | Risk: MEDIUM**

#### 5.1 NetSIO Core Integration
- [ ] Add `netsio.c/h` to Xcode project build
- [ ] Update `sio.c` with NetSIO conditional compilation
- [ ] Enable `NETSIO` preprocessor definition
- [ ] Add pthread linking for threading support

#### 5.2 Mac UI Integration
- [ ] Add FujiNet settings to Preferences window
- [ ] **Port Configuration**: UI for NetSIO UDP port setting
- [ ] **Connection Status**: Display FujiNet-PC connection state
- [ ] **Enable/Disable Toggle**: Runtime NetSIO control

#### 5.3 Build System Updates
- [ ] Update configure flags for NetSIO support
- [ ] Add socket and pthread framework dependencies
- [ ] Conditional compilation setup for Mac builds
- [ ] Test NetSIO functionality with FujiNet-PC

#### 5.4 Integration Testing
- [ ] Test with actual FujiNet-PC software
- [ ] Verify UDP communication functionality
- [ ] Test device connect/disconnect scenarios
- [ ] Performance testing with network operations

## Technical Considerations

### Integration Challenges

#### 1. Video Pipeline Complexity
- New video system requires significant changes to rendering
- NTSC filter adds computational overhead
- Color management split across multiple modules
- Potential conflicts with existing Mac-specific video code

#### 2. Memory Management
- Core uses manual C memory management
- SDL manages graphics buffers
- Cocoa uses ARC for UI objects
- Need careful coordination between layers

#### 3. Configuration Management
- Many new configuration options
- Need Mac-native UI for all settings
- Preference file format updates required
- Backward compatibility considerations

#### 4. Performance Impact
- NTSC filtering adds processing overhead
- Need optimization for Mac hardware
- Consider GPU acceleration options
- Maintain 60fps emulation performance

### Risk Mitigation

#### 1. Incremental Updates
- Implement in phases to reduce risk
- Maintain working builds throughout process
- Keep rollback options available

#### 2. Testing Strategy
- Extensive testing with various ROM types
- Performance benchmarking on different Mac hardware
- User acceptance testing for UI changes

#### 3. Compatibility
- Maintain existing configuration file compatibility
- Provide migration path for preferences
- Ensure existing features continue working

## Development Recommendations

### Immediate Actions
1. **Create Development Branch**: Establish dedicated upgrade branch
2. **Backup Current State**: Full backup of working codebase
3. **Environment Setup**: Ensure development environment ready
4. **Core Analysis**: Detailed analysis of critical integration points

### Phase 1 Priorities (Core Update)
1. **Start with Video System**: Most complex but highest impact
2. **Focus on Compatibility**: Ensure existing functionality preserved
3. **Incremental Integration**: Add new files gradually
4. **Continuous Testing**: Test after each major integration step

### Long-term Considerations
1. **Regular Upstream Syncing**: Keep updated with Atari800 development
2. **Mac-Specific Optimizations**: Leverage Mac hardware capabilities
3. **Modern macOS Features**: Take advantage of latest macOS APIs
4. **Community Feedback**: Engage with Mac Atari community for testing

## Conclusion

The upgrade from the current Atari800MacX to the newer Atari800 core represents a significant improvement in video quality, audio features, and overall emulation accuracy. While the integration effort is substantial, the benefits include:

- **Enhanced Visual Quality**: NTSC filtering and improved color accuracy
- **Simplified Setup**: Built-in Altirra ROMs reduce user complexity
- **Improved Audio**: POKEY recording and voice synthesis
- **Future-Proofing**: More modern codebase for ongoing development

The **comprehensive NetSIO/FujiNet support** in the newer core provides complete FujiNet-PC integration capability, making this a high-value upgrade feature for modern Atari networking.

**Recommended Approach**: Proceed with Phase 1 (Core Engine Update) to modernize the foundation, then selectively implement additional phases based on user feedback and development resources.

---

# Build System & Release Infrastructure (July 2025)

## Overview

Comprehensive build and release system implemented for Atari800MacX 7.0.0 Beta 1, providing automated building, code signing, DMG creation, and Apple notarization support.

## Build Scripts Created

### 1. Main Build Script (`build_release.sh`)
**Purpose**: Master script for complete build and release pipeline

**Features**:
- Clean build environment setup
- Automated app building with Xcode
- Code signature verification
- DMG creation and signing
- Apple notarization integration
- Error handling and status reporting

**Configuration**:
```bash
APP_NAME="Atari800MacX"
BUNDLE_ID="com.atarimac.atari800macx"
DEVELOPER_ID="Apple Development: Paulo Garcia (952JHD69PH)"
TEAM_ID="ZC4PGBX6D6"  # Apple Developer account Team ID
VERSION="7.0.0"
BUILD_CONFIG="Development"
```

**Key Technical Solutions**:
- **Dynamic APP_PATH detection**: Automatically finds built app in DerivedData directory
- **Extended attribute cleanup**: Prevents resource fork code signing errors
- **Conditional error handling**: Graceful failure at each pipeline stage

### 2. DMG Creation Script (`create_dmg.sh`)
**Purpose**: Professional installer DMG creation

**Features**:
- 200MB DMG with HFS+ filesystem
- Professional layout with app and Applications symlink
- Automatic inclusion of release notes
- Custom window positioning and icon arrangement
- Compressed final DMG (~10MB output)

**Layout**:
```
DMG Contents:
├── Atari800MacX.app
├── Applications (symlink)
└── Release Notes.md
```

**AppleScript Integration**: Automated Finder window customization for professional appearance

### 3. Notarization Script (`notarize.sh`)
**Purpose**: Apple notarization for public distribution

**Features**:
- Keychain credential management
- Submission tracking with unique IDs
- Automatic status monitoring
- Detailed error reporting
- Secure credential storage

**Usage**:
```bash
./notarize.sh path/to/app.dmg
```

### 4. Utility Scripts
- **`test_build.sh`**: Quick development builds
- **`clean_and_build.sh`**: Clean builds for testing
- **`build_simple.sh`**: Minimal build without signing

## Critical Issues Resolved

### 1. Bundle Identifier Consistency
**Problem**: Window sizing and UI issues due to bundle ID change
**Root Cause**: Changed from `com.atarimac.atari800macx` to `com.paulogarcia.atari800macx`
**Solution**: Reverted to original bundle ID throughout all scripts and project

**Impact**: Bundle ID changes cause macOS to lose app preferences, resulting in:
- Window sizing problems
- UI element state issues (e.g., "Disable Basic" showing as "Load Basic")
- Lost user preferences

### 2. Build Path Detection
**Problem**: Scripts couldn't find built app due to Xcode using DerivedData
**Original Approach**: Hardcoded paths to project build directory
**Solution**: Dynamic detection using `find` command in DerivedData

```bash
DERIVED_DATA_PATH="$HOME/Library/Developer/Xcode/DerivedData"
APP_PATH=$(find "$DERIVED_DATA_PATH" -name "$APP_NAME.app" -path "*/Build/Products/$BUILD_CONFIG/*" | head -1)
```

### 3. Resource Fork Code Signing Errors
**Problem**: "resource fork, Finder information, or similar detritus not allowed"
**Solution**: Comprehensive extended attribute cleanup

```bash
# Thorough cleanup process
xattr -cr .
find . -name "._*" -delete 2>/dev/null || true
find . -name ".DS_Store" -delete 2>/dev/null || true
find "$APP_PATH" -type f -exec xattr -d com.apple.ResourceFork {} \; 2>/dev/null || true
find "$APP_PATH" -type f -exec xattr -d com.apple.FinderInfo {} \; 2>/dev/null || true
```

### 4. DMG Volume Mounting Conflicts
**Problem**: Multiple DMG creation attempts caused naming conflicts
**Solution**: Proactive volume unmounting and cleanup

```bash
hdiutil detach "$MOUNT_DIR" 2>/dev/null || true
```

## Apple Developer Configuration

### Team ID Resolution
**Development Certificate Team ID**: `952JHD69PH` (from local certificate)
**Apple Developer Account Team ID**: `ZC4PGBX6D6` (from online account)

**Resolution**: Use Apple Developer account Team ID (`ZC4PGBX6D6`) for notarization while keeping existing development certificate for local signing.

### Notarization Setup Process
1. **Create app-specific password** at https://appleid.apple.com
2. **Store credentials in keychain**:
   ```bash
   xcrun notarytool store-credentials "AC_PASSWORD" \
       --apple-id "contact@paulogarcia.com" \
       --team-id "ZC4PGBX6D6" \
       --password "app-specific-password"
   ```
3. **Test credential storage**:
   ```bash
   xcrun notarytool history --keychain-profile "AC_PASSWORD"
   ```

## Notarization Results & Analysis

### Submission Details
**Submission ID**: `18e7b765-cb01-40a1-ac5e-fd8f093a6219`
**Status**: Invalid (Failed)
**File**: `Atari800MacX-7.0.0-beta1.dmg`
**SHA256**: `ab53222b34ab924f6ce54d1c209b73b251c41fa18ff79af81faaa1c392ca24d4`

### Critical Issues Identified

#### 1. Certificate Type Issues
**Problem**: Using "Apple Development" certificate instead of "Apple Distribution"
**Files Affected**:
- `Atari800MacX.app/Contents/MacOS/Atari800MacX`
- `Atari800MacX.app/Contents/Frameworks/SDL2.framework/Versions/A/SDL2`

**Error**: "The binary is not signed with a valid Developer ID certificate"

#### 2. Missing Hardened Runtime
**Problem**: Hardened runtime not enabled (required for notarization)
**Error**: "The executable does not have the hardened runtime enabled"
**File**: Main executable

#### 3. Development Entitlements
**Problem**: Development-only entitlements present in distribution build
**Error**: "The executable requests the com.apple.security.get-task-allow entitlement"
**Cause**: `com.apple.security.get-task-allow` is for development/debugging only

#### 4. Missing Secure Timestamps
**Problem**: Code signatures lack secure timestamps
**Error**: "The signature does not include a secure timestamp"
**Cause**: Development builds don't include timestamping by default

#### 5. Third-Party Framework Signing
**Problem**: SDL2 framework not properly signed for distribution
**Cause**: Embedded frameworks need re-signing with distribution certificate

## Current Status & Next Steps

### ✅ Working Features (Development Distribution)
- **Complete build pipeline**: App builds and signs successfully
- **Professional DMG creation**: ~10MB compressed installer
- **Local code signing**: Works with development certificate
- **DMG signing**: Successfully signed and verified
- **Error handling**: Comprehensive error detection and reporting

### ❌ Notarization Blockers (For Public Distribution)
1. Need Apple Distribution certificate (not Development)
2. Enable hardened runtime in Xcode project
3. Remove development entitlements
4. Add secure timestamp support
5. Properly sign embedded SDL2 framework

### Distribution Options

#### Option 1: Beta Testing (Current Capability)
**Status**: ✅ Ready
**Distribution Method**: Direct DMG distribution
**User Instructions**: "Right-click app and select 'Open' to bypass Gatekeeper"
**Suitable For**: Beta testers, developer distribution

#### Option 2: Public Distribution (Requires Additional Work)
**Status**: ❌ Blocked by notarization issues
**Requirements**:
- Apple Distribution certificate
- Hardened runtime configuration
- Entitlements cleanup
- Framework re-signing
- Full notarization compliance

## File Structure

### Generated Build Artifacts
```
/release/
└── Atari800MacX-7.0.0-beta1.dmg    # Signed, ready for beta distribution

/build/
└── Development/
    └── Atari800MacX.app             # Local copy of built app
```

### Build Scripts Location
```
/build_release.sh                    # Master build script
/create_dmg.sh                      # DMG creation
/notarize.sh                        # Apple notarization
/test_build.sh                      # Development builds
/clean_and_build.sh                 # Clean builds
/build_simple.sh                    # Minimal builds
```

## Performance Metrics

### Build Performance
- **Clean Build Time**: ~2-3 minutes
- **DMG Creation Time**: ~10 seconds
- **Final DMG Size**: ~10MB (from 200MB uncompressed)
- **Compression Ratio**: 95% space savings

### Code Signing Performance
- **App Signing**: ~5 seconds
- **Framework Signing**: ~3 seconds (SDL2)
- **DMG Signing**: ~2 seconds
- **Verification**: <1 second

## Lessons Learned

### 1. Bundle Identifier Stability
**Critical**: Never change bundle identifiers during development unless absolutely necessary. macOS ties app preferences, security permissions, and UI state to bundle IDs.

### 2. Xcode DerivedData Behavior
**Important**: Don't rely on hardcoded build paths. Xcode's DerivedData location is dynamic and project-specific.

### 3. Extended Attributes Management
**Essential**: Resource fork cleanup is mandatory for successful code signing. Must be performed both before and after build.

### 4. Apple Developer Team Management
**Key Insight**: Development certificates and notarization can use different Team IDs. Always use the Apple Developer Portal Team ID for notarization.

### 5. Notarization Requirements
**Critical Knowledge**: Development builds cannot be notarized. Distribution requires:
- Distribution certificate (not Development)
- Hardened runtime enabled
- Development entitlements removed
- Secure timestamps
- All embedded frameworks properly signed

## Recommendations

### For Immediate Beta Distribution
1. Use current build system as-is
2. Distribute DMG with Gatekeeper bypass instructions
3. Test with beta users before investing in full notarization

### For Future Public Distribution
1. Obtain Apple Distribution certificate
2. Configure Xcode project for hardened runtime
3. Create separate build configuration for distribution
4. Implement framework re-signing in build pipeline
5. Add secure timestamp support

### For Ongoing Development
1. Maintain bundle identifier consistency
2. Test build scripts regularly
3. Keep credentials secure and up-to-date
4. Monitor Apple's notarization requirement changes

## Technical Debt & Future Work

### Build System Improvements
- [ ] Add support for Release configuration builds
- [ ] Implement automatic version bumping
- [ ] Add build artifact archival
- [ ] Create automated testing integration

### Distribution Enhancements
- [ ] Implement distribution certificate support
- [ ] Add hardened runtime configuration
- [ ] Create entitlements management system
- [ ] Develop framework re-signing pipeline

### Developer Experience
- [ ] Add build status notifications
- [ ] Implement build caching
- [ ] Create development environment validation
- [ ] Add automated documentation generation

This comprehensive build system provides a solid foundation for Atari800MacX distribution while clearly documenting the path forward for full Apple notarization compliance.