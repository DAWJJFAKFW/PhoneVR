# PhoneVR Network Protocol

## Packet Format

Every packet has a 20-byte header followed by optional payload.

### Header (20 bytes)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 4 | magic | Magic number 0x50485256 ("PHRV") |
| 4 | 4 | sequence | Packet sequence number |
| 8 | 2 | type | Packet type (see below) |
| 10 | 2 | flags | Bit flags |
| 12 | 4 | payload_size | Payload size in bytes |
| 16 | 4 | crc32 | CRC32 of payload |

### Packet Types

| Value | Type | Transport | Description |
|-------|------|-----------|-------------|
| 0x0001 | Tracking | TCP | Head/Hand tracking data |
| 0x0002 | Video | UDP | Compressed video frames |
| 0x0003 | Control | TCP | Control messages |
| 0x0004 | Audio | UDP | Audio stream |

### Flags

| Bit | Flag | Description |
|-----|------|-------------|
| 0 | Reliable | Acknowledge required |
| 1 | Keyframe | Video keyframe |
| 2 | Fragment | Packet is fragmented |
| 3 | FinalFragment | Last fragment |
| 4 | Encrypted | Payload is AES-GCM encrypted |
| 5 | Compressed | Payload is zlib compressed |

## Control Messages

Control messages use type 0x0003 and contain a serialized protobuf message.

### Message Types

| ID | Type | Direction | Payload |
|----|------|-----------|---------|
| 0 | Heartbeat | Bidirectional | Empty |
| 1 | Recenter | iOS → PC | Empty |
| 2 | Calibrate | Bidirectional | Calibration data |
| 3 | SetBitrate | PC → iOS | uint32 bitrate |
| 4 | SetResolution | PC → iOS | uint32 width, uint32 height |
| 5 | SetFPS | PC → iOS | uint32 fps |
| 6 | SetIPD | PC → iOS | float ipd |
| 7 | RequestKeyframe | PC → iOS | Empty |
| 8 | Disconnect | Bidirectional | Empty |
| 9 | StatsReport | Bidirectional | StatsReport |
| 10 | DeviceInfo | iOS → PC | DeviceInfo |

## Encryption

- AES-256-GCM for all control packets
- Session key exchanged at connection start via ECDH
- Nonce (12 bytes) prepended to each encrypted payload
- 16-byte authentication tag appended
