# PhoneVR Architecture

## System Overview

```
┌─────────────────────────────────────────────────────────┐
│                     iPhone 16 Pro                        │
│  ┌──────────┐  ┌──────────┐  ┌────────────────────┐    │
│  │ CoreMotion│  │  Vision  │  │   VideoToolbox     │    │
│  │ (IMU)    │  │(Hands)   │  │ (H.264/H.265 Enc)  │    │
│  └─────┬────┘  └────┬─────┘  └─────────┬──────────┘    │
│        │            │                   │               │
│  ┌─────┴────────────┴───────────────────┴──────────┐   │
│  │           SensorFusionEngine                     │   │
│  │  (Kalman Filter + Complementary + Predictor)     │   │
│  └─────────────────────┬───────────────────────────┘   │
│                        │                               │
│  ┌─────────────────────┴───────────────────────────┐   │
│  │              StreamingService                   │   │
│  │  (Network Service + Encoder + Camera Manager)   │   │
│  └─────────────────────┬───────────────────────────┘   │
│                        │ UDP/TCP                       │
└────────────────────────┼───────────────────────────────┘
                         │
┌────────────────────────┼───────────────────────────────┐
│                   Windows PC                            │
│  ┌─────────────────────┴───────────────────────────┐   │
│  │              NetworkReceiver                    │   │
│  │        (TCP Control + UDP Video)                │   │
│  └─────┬───────────────────────────────┬───────────┘   │
│        │                               │               │
│  ┌─────┴──────────┐          ┌─────────┴──────────┐    │
│  │ HardwareDecoder │          │   PhoneVR Driver   │    │
│  │ (Media Found.) │          │   (SteamVR/OpenXR) │    │
│  └─────┬──────────┘          └─────────┬──────────┘    │
│        │                               │               │
│  ┌─────┴───────────────────────────────┴───────────┐   │
│  │              D3D12Renderer                      │   │
│  │   (Stereoscopic + Distortion + Chroma Ab.)      │   │
│  └─────────────────────┬───────────────────────────┘   │
│                        │                               │
│  ┌─────────────────────┴───────────────────────────┐   │
│  │           SteamVR / OpenXR Runtime              │   │
│  └─────────────────────┬───────────────────────────┘   │
│                        │                               │
│  ┌─────────────────────┴───────────────────────────┐   │
│  │              VRChat / Any VR App                 │   │
│  └─────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

## Data Flow

1. **Tracking**: iPhone IMU data → SensorFusionEngine → Pose estimation → Network → PC → SteamVR driver
2. **Hands**: iPhone camera → Vision Framework → Hand landmarks → Gesture recognition → Network → PC → SteamVR skeleton
3. **Video**: iPhone render → VideoToolbox HW encode → Network UDP → PC HW decode → DirectX 12 → OpenXR/SteamVR
4. **Control**: TCP bidirectional channel for heartbeat, settings, calibration, stats

## Key Design Decisions

- **Kalman + Complementary Filter**: Provides drift-free orientation with gyro-based high-frequency response
- **Motion Prediction**: 8ms prediction compensates for network and encode/decode latency
- **Hardware Encoding/Decoding**: H.265 HEVC for best quality/bandwidth ratio on iPhone 16 Pro
- **UDP + TCP Split**: Video over unreliable UDP with packet loss prediction; control over reliable TCP
- **Own Protocol**: Custom packet format with CRC32, AES-GCM encryption, sequence numbers
- **SteamVR Driver**: Full driver using openvr_driver API for maximum compatibility (VRChat, etc.)
- **DirectX 12**: Low-level GPU control for minimal render latency
