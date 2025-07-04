# Atari800MacX 7.0.0 Beta 1 Release Notes

## Overview
Atari800MacX 7.0.0 Beta 1 introduces significant new networking capabilities and enhanced user interface improvements. Building on the solid Atari800 4.2.0 core that has been part of Atari800MacX since version 5.3 (2019), this release focuses on modern connectivity features and improved video configuration options.

## Major New Features

### 🌐 FujiNet Network Device Support ⭐ **NEW**
- **Complete FujiNet Integration**: Full support for FujiNet-PC network adapter emulation
- **NetSIO Protocol**: Complete UDP-based communication system with FujiNet-PC
- **Threading Architecture**: Dedicated receiver thread for reliable network communication
- **20+ Network Commands**: Full protocol implementation for device communication
- **FIFO Buffer System**: 4KB bidirectional data transfer buffers
- **Connection Management**: Device connect/disconnect, ping/alive protocols
- **Large Buffer Support**: 65KB+ buffers for FujiNet operations

### 🎛️ Enhanced Video Interface ⭐ **IMPROVED**
- **NTSC Filter Preset Menu**: Easy access to video filter presets in Display menu
- **TV Mode-Aware Artifacts**: Separate artifact settings for NTSC and PAL modes
- **Dynamic Menu Updates**: Display menu automatically adjusts based on current TV mode
- **Persistent Preferences**: Artifact settings now properly saved between sessions
- **Improved Artifact Selection**: Enhanced UI for selecting video artifact modes

### ⚙️ Preferences System Overhaul ⭐ **IMPROVED**
- **New Expansions Tab**: Dedicated section for expansion device settings
- **FujiNet Configuration**: Enable/disable toggle with real-time status display
- **Enhanced Video Settings**: Improved organization of display and artifact options
- **Auto-Boot Integration**: FujiNet automatically initializes with SIO patch
- **Connection Monitoring**: Real-time display of FujiNet connection state

## User Interface Improvements

### Preferences Window
- **New Expansions Tab**: Consolidated location for expansion device settings including FujiNet
- **Enhanced Display Organization**: Better grouping of video and artifact settings
- **Real-time Status Updates**: Live connection status for network devices
- **Persistent Settings**: All preferences properly saved and restored

### Display Menu Enhancements
- **NTSC Filter Presets**: Quick access menu items for Composite, S-Video, RGB, Monochrome
- **Dynamic Artifact Options**: Menu automatically adapts to current NTSC/PAL mode
- **Improved Workflow**: Streamlined access to commonly used video settings

## Bug Fixes and Stability
- **Fixed Emulator Crashes**: Resolved EXC_BAD_ACCESS crashes on emulator restart
- **Fixed Settings Crashes**: Eliminated NSInvalidArgumentException in preferences window
- **Fixed Boot Issues**: Resolved problems with SIO patch initialization preventing emulator start
- **Fixed SIDE2 ROM Loading**: Corrected ROM loading errors for SIDE2 cartridges
- **Fixed Preference Persistence**: Artifact and video settings now properly saved between sessions
- **Fixed Display Issues**: Resolved various NTSC/PAL mode switching problems

## Technical Improvements
- **NetSIO Integration**: Complete integration of network SIO system for modern connectivity
- **Threading Enhancements**: Dedicated network receiver threads for reliable communication
- **Memory Management**: Improved handling of large network buffers and data transfers
- **Error Handling**: Better error reporting and recovery for network operations
- **Configuration Compatibility**: Maintains compatibility with existing Atari800MacX configurations

## Known Issues (Beta)
- This is a beta release - please report any issues you encounter
- FujiNet requires separate FujiNet-PC software to be running for network functionality
- Some advanced video filter parameters may require manual configuration
- Network features are currently optimized for FujiNet-PC protocol

## Installation Notes
- **Full Backward Compatibility**: Existing Atari800MacX configurations will work without changes
- **Automatic Migration**: Preferences will be automatically updated to new format
- **ROM Compatibility**: Continues to support both built-in Altirra ROMs and external ROM files
- **Network Setup**: FujiNet features are disabled by default - enable in Expansions preferences if needed

## Coming in Future Releases
- **Enhanced FujiNet Features**: Additional network device emulation and configuration options
- **Performance Optimizations**: Further improvements for modern Mac hardware
- **Advanced Network Options**: More sophisticated network device configuration
- **UI Refinements**: Continued improvements to preferences and display options

## Acknowledgments
Special thanks to:
- **The Atari800 development team** for maintaining the excellent emulator core
- **The FujiNet team** for the network device specifications and protocol documentation
- **Avery Lee (phaeron)** for the Altirra ROMs that continue to power the emulator
- **The Atari community** for continued support, testing, and feedback

## Download
Available from: https://github.com/pedgarcia/Atari800MacX/releases/tag/v7.0.0-beta1

## Feedback
Please report issues at: https://github.com/pedgarcia/Atari800MacX/issues

---
*Atari800MacX 7.0.0 Beta 1 - Released [DATE]*