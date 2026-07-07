import SwiftUI
import Combine

public class VRAppState: ObservableObject {
    @Published public var connectionState: ConnectionState = .disconnected
    @Published public var trackingQuality: VRTrackingQuality = .high
    @Published public var frameRate: UInt32 = 90
    @Published public var batteryLevel: UInt32 = 100
    @Published public var fps: Double = 0
    @Published public var latencyMs: Double = 0
    @Published public var bitrateMbps: Double = 0
    @Published public var packetsLost: UInt32 = 0
    @Published public var isEncoding: Bool = false
    @Published public var isRendering: Bool = false

    @Published public var colorScheme: ColorScheme? = .dark

    @Published public var settings: VRSettings
    @Published public var stats: VRStatsReport = VRStatsReport()
    @Published public var deviceInfo: VRDeviceInfo?
    @Published public var leftHandPose: VRHandData?
    @Published public var rightHandPose: VRHandData?

    @Published public var showDebugOverlay: Bool = false
    @Published public var showSettings: Bool = false

    public init() {
        self.settings = VRSettings.load()
    }

    public enum ConnectionState: Equatable {
        case disconnected
        case connecting(String)
        case connected(host: String, pingMs: Double)
        case error(String)
    }
}

public struct VRSettings: Codable {
    public var hostAddress: String = "192.168.1.100"
    public var hostPort: UInt16 = 8765
    public var useWiFi: Bool = true
    public var preferredCodec: VRVideoCodec = .h265
    public var targetBitrate: UInt32 = 100_000_000
    public var targetFPS: UInt32 = 90
    public var ipd: Float = 63.5
    public var renderScale: Float = 1.0
    public var enableHandTracking: Bool = true
    public var recenterOnConnect: Bool = true
    public var lowPersistenceMode: Bool = true
    public var enableFoveated: Bool = false
    public var calibratedIPD: Float = 63.5
    public var calibrationOffset: Float = 0.0
    public var sensorSensitivity: Float = 1.0

    public static func load() -> VRSettings {
        guard let data = UserDefaults.standard.data(forKey: "VRSettings"),
              let settings = try? JSONDecoder().decode(VRSettings.self, from: data) else {
            return VRSettings()
        }
        return settings
    }

    public func save() {
        guard let data = try? JSONEncoder().encode(self) else { return }
        UserDefaults.standard.set(data, forKey: "VRSettings")
    }
}
