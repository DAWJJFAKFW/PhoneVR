# PhoneVR

Convierte un iPhone 16 Pro en un visor de realidad virtual para PC compatible con SteamVR y OpenXR.

## Arquitectura

```
PhoneVR/
├── apps/
│   ├── ios-client/     # App iOS (SwiftUI, Metal, CoreMotion, Vision)
│   ├── windows-host/   # Servidor Windows (C++, DirectX 12, Media Foundation)
│   ├── steamvr-driver/ # Driver SteamVR (C++, OpenVR)
│   └── launcher/       # Lanzador de escritorio (C++, Win32)
├── shared/             # Código compartido (protocolos, tipos)
│   ├── swift/
│   ├── cpp/
│   └── proto/
├── tests/
└── docs/
```

## Módulos

### iOS Client
- SwiftUI con pantalla dividida VR
- Metal para renderizado estereoscópico con corrección de distorsión
- CoreMotion con Kalman Filter + Complementary Filter + predicción
- Vision Framework para hand tracking (21 landmarks, 6 gestos)
- VideoToolbox para encoding H.264/H.265 hardware
- Network Framework para streaming UDP/TCP

### Windows Host
- Receptor de red multicliente (TCP para control, UDP para video)
- Decodificación hardware via Media Foundation
- Renderizado DirectX 12 estereoscópico
- Integración OpenXR y OpenVR/SteamVR

### SteamVR Driver
- Driver completo con HMD, 2 controllers, skeleton
- Distorsión de lentes (barrel + chromatic aberration)
- Input system con gestos y skeletal tracking
- Compatible con VRChat y cualquier app SteamVR/OpenXR

## Requisitos

- **iOS**: iPhone 16 Pro, iOS 18+, Xcode 16+
- **Windows**: Windows 11, GPU compatible con DirectX 12, SteamVR
- **Compilación**: CMake 3.22+, Visual Studio 2022, Xcode 16

## Compilación

### iOS
```bash
cd apps/ios-client
xcodebuild -project PhoneVR.xcodeproj -scheme PhoneVR -configuration Release
```

### Windows Host & Driver
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## Licencia

MIT
