import Foundation
import Combine
import AVFoundation
import UIKit

public class StreamingService: ObservableObject {
    private let networkService = NetworkService()
    private let sensorFusion = SensorFusionEngine()
    private let handTracking = HandTrackingService()
    private var videoEncoder: VideoEncoderService?
    private var cameraService: CameraService?

    @Published public var connectionState: VRAppState.ConnectionState = .disconnected
    @Published public var stats = VRStatsReport()
    @Published public var currentFPS: Double = 0
    @Published public var currentLatency: Double = 0

    private var frameTimestamps: [UInt64] = []
    private var statsTimer: Timer?
    private var isStreaming = false

    public var appState: VRAppState?

    public init() {
        networkService.onConnectionStateChange = { [weak self] status in
            DispatchQueue.main.async {
                switch status {
                case .connected:
                    self?.connectionState = .connected(host: "", pingMs: 0)
                case .connecting:
                    self?.connectionState = .connecting("Connecting...")
                case .disconnected:
                    self?.connectionState = .disconnected
                case .failed(let error):
                    self?.connectionState = .error(error.localizedDescription)
                }
            }
        }

        networkService.onControlMessage = { [weak self] message in
            self?.handleControlMessage(message)
        }

        sensorFusion.onPoseUpdate = { [weak self] pose in
            self?.sendTrackingFrame(pose: pose)
        }

        handTracking.onHandUpdate = { [weak self] left, right in
            self?.appState?.leftHandPose = left
            self?.appState?.rightHandPose = right
        }
    }

    public func connect(host: String, port: UInt16, settings: VRSettings) {
        networkService.connect(host: host, port: port, useTCP: true)
        sensorFusion.startTracking()

        if settings.enableHandTracking {
            handTracking.startHandTracking()
        }

        startStatsTimer()
        isStreaming = true
    }

    public func disconnect() {
        networkService.disconnect()
        sensorFusion.stopTracking()
        handTracking.stopHandTracking()
        videoEncoder?.stopEncoding()
        cameraService?.stopCamera()
        stopStatsTimer()
        isStreaming = false
    }

    public func recenter() {
        sensorFusion.recenter()
        let message = VRControlMessage(type: .recenter, payload: Data(), timestampUs: nowUs(), messageId: 0)
        networkService.sendControlMessage(message)
    }

    public func startVideoEncoding(width: Int32, height: Int32) {
        let encoder = VideoEncoderService()
        encoder.codecType = appState?.settings.preferredCodec ?? .h265
        encoder.targetBitrate = appState?.settings.targetBitrate ?? 100_000_000
        encoder.targetFPS = appState?.settings.targetFPS ?? 90

        encoder.onEncodedFrame = { [weak self] data, header in
            guard let self = self else { return }
            let packet = VRVideoPacket(
                header: header,
                nalData: data,
                packetSequence: 0,
                totalPackets: 1,
                isKeyframe: header.nalCount == 0
            )
            self.networkService.sendVideoPacket(packet)
            self.trackFrame()
        }

        encoder.startEncoding(width: width, height: height)
        self.videoEncoder = encoder
    }

    public func stopVideoEncoding() {
        videoEncoder?.stopEncoding()
        videoEncoder = nil
    }

    public func startCamera() {
        let camera = CameraService()
        camera.onPixelBuffer = { [weak self] pixelBuffer, timestamp in
            self?.handTracking.processPixelBuffer(pixelBuffer, timestamp: timestamp)
        }
        camera.startCamera()
        self.cameraService = camera
    }

    private func sendTrackingFrame(pose: VRPose) {
        guard isStreaming else { return }

        var trackingFrame = VRTrackingFrame(
            headPose: pose,
            rawImu: sensorFusion.currentImu,
            leftHand: nil,
            rightHand: nil,
            frameSequence: 0,
            quality: .high
        )

        if let left = appState?.leftHandPose {
            trackingFrame.leftHand = left
        }
        if let right = appState?.rightHandPose {
            trackingFrame.rightHand = right
        }

        networkService.sendTrackingFrame(trackingFrame)
    }

    private func handleControlMessage(_ message: VRControlMessage) {
        switch message.type {
        case .setBitrate:
            let bitrate = message.payload.withUnsafeBytes { $0.load(as: UInt32.self) }
            videoEncoder?.updateBitrate(bitrate)
        case .setFps:
            let fps = message.payload.withUnsafeBytes { $0.load(as: UInt32.self) }
            videoEncoder?.updateFPS(fps)
        case .requestKeyframe:
            videoEncoder?.requestKeyframe()
        case .statsReport:
            if let report = try? JSONDecoder().decode(VRStatsReport.self, from: message.payload) {
                DispatchQueue.main.async { [weak self] in
                    self?.stats = report
                }
            }
        default:
            break
        }
    }

    private func startStatsTimer() {
        statsTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.updateStats()
        }
    }

    private func stopStatsTimer() {
        statsTimer?.invalidate()
        statsTimer = nil
    }

    private func updateStats() {
        let now = nowUs()
        frameTimestamps = frameTimestamps.filter { now - $0 < 1_000_000 }
        currentFPS = Double(frameTimestamps.count)
        currentLatency = Double(stats.motionToPhotonLatencyMs)
    }

    private func trackFrame() {
        frameTimestamps.append(nowUs())
    }

    private func nowUs() -> UInt64 {
        UInt64(Date().timeIntervalSince1970 * 1_000_000)
    }
}
