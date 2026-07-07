# Building PhoneVR

## Prerequisites

### iOS
- Xcode 16+
- macOS 15+
- iPhone 16 Pro (for deployment)

### Windows
- Visual Studio 2022 with C++ desktop development
- CMake 3.22+
- Windows 11
- SteamVR installed
- GPU with DirectX 12 support

## Build Instructions

### 1. iOS Client

```bash
cd apps/ios-client/PhoneVR

# Create Xcode project
swift package generate-xcodeproj

# Build
xcodebuild -project PhoneVR.xcodeproj \
  -scheme PhoneVR \
  -configuration Release \
  -sdk iphoneos \
  CODE_SIGN_IDENTITY="" \
  CODE_SIGNING_REQUIRED=NO

# The .ipa will be in build/Release-iphoneos/
```

### 2. Windows Host

```bash
mkdir build && cd build

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DOPENXR_INCLUDE_DIR="path/to/OpenXR-SDK/include" \
  -DOPENVR_INCLUDE_DIR="path/to/openvr/headers"

cmake --build . --config Release --target PhoneVRHost
```

### 3. SteamVR Driver

```bash
cmake --build . --config Release --target phonevr_driver

# Install driver
cmake --install . --config Release \
  -DSTEAMVR_DRIVERS_DIR="C:/Program Files (x86)/Steam/steamapps/common/SteamVR/drivers"
```

### 4. Tests

```bash
cmake --build . --config Release --target shared_test
cmake --build . --config Release --target windows_test
cmake --build . --config Release --target steamvr_test

ctest --output-on-failure
```

## Configuration

### iOS
Settings are stored in UserDefaults. Configure via the Settings UI in the app.

### Windows Host
Configuration file: `phonevr_config.json` in the working directory.

### SteamVR Driver
Settings file: `steamvr/config/phonevr.vrsettings`
