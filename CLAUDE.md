# Atari800MacX Upgrade Analysis & Strategy

## Project Overview

**Atari800MacX** is a sophisticated macOS port of the cross-platform Atari800 emulator. It wraps the core Atari800 emulation engine with a native Cocoa interface, providing Mac-specific features while maintaining the accuracy and performance of the original emulator.

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