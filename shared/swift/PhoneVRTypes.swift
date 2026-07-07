import Foundation
import simd

// MARK: - Tracking Types

public struct VRVector3: Codable, Sendable {
    public var x: Float
    public var y: Float
    public var z: Float

    public init(x: Float = 0, y: Float = 0, z: Float = 0) {
        self.x = x; self.y = y; self.z = z
    }

    public var simd: SIMD3<Float> { SIMD3<Float>(x, y, z) }

    public static let zero = VRVector3()
}

public struct VRQuaternion: Codable, Sendable {
    public var x: Float
    public var y: Float
    public var z: Float
    public var w: Float

    public init(x: Float = 0, y: Float = 0, z: Float = 0, w: Float = 1) {
        self.x = x; self.y = y; self.z = z; self.w = w
    }

    public var simd: simd_quatf { simd_quatf(ix: x, iy: y, iz: z, r: w) }

    public static let identity = VRQuaternion()
}

public struct VRPose: Codable, Sendable {
    public var position: VRVector3
    public var orientation: VRQuaternion
    public var timestampUs: UInt64

    public init(position: VRVector3 = .zero, orientation: VRQuaternion = .identity, timestampUs: UInt64 = 0) {
        self.position = position; self.orientation = orientation; self.timestampUs = timestampUs
    }
}

public struct VRImuData: Codable, Sendable {
    public var accelerometer: VRVector3
    public var gyroscope: VRVector3
    public var magnetometer: VRVector3
    public var timestampUs: UInt64

    public init(accelerometer: VRVector3 = .zero, gyroscope: VRVector3 = .zero, magnetometer: VRVector3 = .zero, timestampUs: UInt64 = 0) {
        self.accelerometer = accelerometer
        self.gyroscope = gyroscope
        self.magnetometer = magnetometer
        self.timestampUs = timestampUs
    }
}

public struct VRHandLandmark: Codable, Sendable {
    public var position: VRVector3
    public var confidence: Float
    public var jointType: HandJointType

    public init(position: VRVector3 = .zero, confidence: Float = 0, jointType: HandJointType = .wrist) {
        self.position = position; self.confidence = confidence; self.jointType = jointType
    }

    public enum HandJointType: Int, Codable, Sendable {
        case wrist = 0
        case thumbCMC = 1, thumbMCP = 2, thumbIP = 3, thumbTip = 4
        case indexMCP = 5, indexPIP = 6, indexDIP = 7, indexTip = 8
        case middleMCP = 9, middlePIP = 10, middleDIP = 11, middleTip = 12
        case ringMCP = 13, ringPIP = 14, ringDIP = 15, ringTip = 16
        case littleMCP = 17, littlePIP = 18, littleDIP = 19, littleTip = 20
    }
}

public enum VRHandGesture: Int, Codable, Sendable {
    case unknown = 0
    case pinch = 1
    case grab = 2
    case point = 3
    case thumbUp = 4
    case openHand = 5
    case fist = 6
}

public struct VRHandData: Codable, Sendable {
    public var handId: UInt32
    public var landmarks: [VRHandLandmark]
    public var handRadius: Float
    public var wristPosition: VRVector3
    public var wristRotation: VRQuaternion
    public var gesture: VRHandGesture
    public var timestampUs: UInt64

    public init(handId: UInt32 = 0, landmarks: [VRHandLandmark] = [], handRadius: Float = 0, wristPosition: VRVector3 = .zero, wristRotation: VRQuaternion = .identity, gesture: VRHandGesture = .unknown, timestampUs: UInt64 = 0) {
        self.handId = handId; self.landmarks = landmarks; self.handRadius = handRadius
        self.wristPosition = wristPosition; self.wristRotation = wristRotation
        self.gesture = gesture; self.timestampUs = timestampUs
    }
}

public struct VRTrackingFrame: Codable, Sendable {
    public var headPose: VRPose
    public var rawImu: VRImuData
    public var leftHand: VRHandData?
    public var rightHand: VRHandData?
    public var frameSequence: UInt32
    public var quality: VRTrackingQuality

    public init(headPose: VRPose = .init(), rawImu: VRImuData = .init(), leftHand: VRHandData? = nil, rightHand: VRHandData? = nil, frameSequence: UInt32 = 0, quality: VRTrackingQuality = .high) {
        self.headPose = headPose; self.rawImu = rawImu; self.leftHand = leftHand
        self.rightHand = rightHand; self.frameSequence = frameSequence; self.quality = quality
    }
}

public enum VRTrackingQuality: Int, Codable, Sendable {
    case low = 0
    case medium = 1
    case high = 2
}

// MARK: - Video Types

public enum VRVideoCodec: Int, Codable, Sendable {
    case h264 = 0
    case h265 = 1
}

public enum VRVideoEye: Int, Codable, Sendable {
    case left = 0
    case right = 1
    case both = 2
}

public struct VRVideoFrameHeader: Codable, Sendable {
    public var width: UInt32
    public var height: UInt32
    public var ptsUs: UInt64
    public var frameSequence: UInt32
    public var codec: VRVideoCodec
    public var eye: VRVideoEye
    public var nalCount: UInt32
    public var spsPps: Data
    public var frameSize: UInt32

    public init(width: UInt32 = 0, height: UInt32 = 0, ptsUs: UInt64 = 0, frameSequence: UInt32 = 0, codec: VRVideoCodec = .h265, eye: VRVideoEye = .both, nalCount: UInt32 = 0, spsPps: Data = .init(), frameSize: UInt32 = 0) {
        self.width = width; self.height = height; self.ptsUs = ptsUs; self.frameSequence = frameSequence
        self.codec = codec; self.eye = eye; self.nalCount = nalCount; self.spsPps = spsPps; self.frameSize = frameSize
    }
}

public struct VRVideoPacket: Codable, Sendable {
    public var header: VRVideoFrameHeader
    public var nalData: Data
    public var packetSequence: UInt32
    public var totalPackets: UInt32
    public var isKeyframe: Bool

    public init(header: VRVideoFrameHeader = .init(), nalData: Data = .init(), packetSequence: UInt32 = 0, totalPackets: UInt32 = 1, isKeyframe: Bool = false) {
        self.header = header; self.nalData = nalData; self.packetSequence = packetSequence
        self.totalPackets = totalPackets; self.isKeyframe = isKeyframe
    }
}

// MARK: - Control Types

public enum VRControlMessageType: UInt32, Codable, Sendable {
    case heartbeat = 0
    case recenter = 1
    case calibrate = 2
    case setBitrate = 3
    case setResolution = 4
    case setFps = 5
    case setIpd = 6
    case requestKeyframe = 7
    case disconnect = 8
    case statsReport = 9
    case deviceInfo = 10
}

public struct VRControlMessage: Codable, Sendable {
    public var type: VRControlMessageType
    public var payload: Data
    public var timestampUs: UInt64
    public var messageId: UInt32

    public init(type: VRControlMessageType = .heartbeat, payload: Data = .init(), timestampUs: UInt64 = 0, messageId: UInt32 = 0) {
        self.type = type; self.payload = payload; self.timestampUs = timestampUs; self.messageId = messageId
    }
}

public struct VRStatsReport: Codable, Sendable {
    public var roundTripTimeUs: UInt64
    public var packetsLost: UInt32
    public var packetsSent: UInt32
    public var bitrateMbps: Float
    public var encodeTimeMs: Float
    public var decodeLatencyMs: Float
    public var renderLatencyMs: Float
    public var motionToPhotonLatencyMs: Float
    public var frameRate: UInt32
    public var batteryLevel: UInt32

    public init(roundTripTimeUs: UInt64 = 0, packetsLost: UInt32 = 0, packetsSent: UInt32 = 0, bitrateMbps: Float = 0, encodeTimeMs: Float = 0, decodeLatencyMs: Float = 0, renderLatencyMs: Float = 0, motionToPhotonLatencyMs: Float = 0, frameRate: UInt32 = 0, batteryLevel: UInt32 = 0) {
        self.roundTripTimeUs = roundTripTimeUs; self.packetsLost = packetsLost; self.packetsSent = packetsSent
        self.bitrateMbps = bitrateMbps; self.encodeTimeMs = encodeTimeMs; self.decodeLatencyMs = decodeLatencyMs
        self.renderLatencyMs = renderLatencyMs; self.motionToPhotonLatencyMs = motionToPhotonLatencyMs
        self.frameRate = frameRate; self.batteryLevel = batteryLevel
    }
}

public struct VRDeviceInfo: Codable, Sendable {
    public var deviceName: String
    public var osVersion: String
    public var appVersion: String
    public var displayRefreshRate: Float
    public var displayWidth: UInt32
    public var displayHeight: UInt32
    public var displayDpi: Float
    public var supportsHEVC: Bool
    public var maxBitrate: UInt32
    public var minBitrate: UInt32
    public var ipd: Float

    public init(deviceName: String = "", osVersion: String = "", appVersion: String = "", displayRefreshRate: Float = 0, displayWidth: UInt32 = 0, displayHeight: UInt32 = 0, displayDpi: Float = 0, supportsHEVC: Bool = false, maxBitrate: UInt32 = 0, minBitrate: UInt32 = 0, ipd: Float = 0) {
        self.deviceName = deviceName; self.osVersion = osVersion; self.appVersion = appVersion
        self.displayRefreshRate = displayRefreshRate; self.displayWidth = displayWidth; self.displayHeight = displayHeight
        self.displayDpi = displayDpi; self.supportsHEVC = supportsHEVC; self.maxBitrate = maxBitrate
        self.minBitrate = minBitrate; self.ipd = ipd
    }
}
