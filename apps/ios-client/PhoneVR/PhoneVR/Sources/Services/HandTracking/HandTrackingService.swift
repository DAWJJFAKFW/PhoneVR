import Foundation
import Vision
import CoreGraphics
import UIKit

public class HandTrackingService: NSObject {
    private let handPoseRequest = VNDetectHumanHandPoseRequest()
    private let sequenceHandler = VNSequenceRequestHandler()
    private var isTracking = false

    public var onHandUpdate: ((VRHandData, VRHandData?) -> Void)?

    private let landmarkOrder: [VRHandLandmark.HandJointType] = [
        .wrist,
        .thumbCMC, .thumbMCP, .thumbIP, .thumbTip,
        .indexMCP, .indexPIP, .indexDIP, .indexTip,
        .middleMCP, .middlePIP, .middleDIP, .middleTip,
        .ringMCP, .ringPIP, .ringDIP, .ringTip,
        .littleMCP, .littlePIP, .littleDIP, .littleTip
    ]

    public override init() {
        super.init()
        handPoseRequest.maximumHandCount = 2
        handPoseRequest.revision = VNDetectHumanHandPoseRequestRevision1
    }

    public func startHandTracking() {
        isTracking = true
    }

    public func stopHandTracking() {
        isTracking = false
    }

    public func processPixelBuffer(_ pixelBuffer: CVPixelBuffer, timestamp: UInt64) {
        guard isTracking else { return }

        let handler = VNImageRequestHandler(cvPixelBuffer: pixelBuffer, orientation: .up, options: [:])

        do {
            try handler.perform([handPoseRequest])
        } catch {
            return
        }

        guard let observations = handPoseRequest.results, !observations.isEmpty else { return }

        var leftHand: VRHandData?
        var rightHand: VRHandData?

        for observation in observations {
            let handData = extractHandData(from: observation, timestamp: timestamp)
            switch observation.chirality {
            case .left:
                leftHand = handData
            case .right:
                rightHand = handData
            case .unknown:
                break
            @unknown default:
                break
            }
        }

        DispatchQueue.main.async { [weak self] in
            guard let self = self else { return }
            if let left = leftHand {
                self.onHandUpdate?(left, rightHand ?? leftHand)
            } else if let right = rightHand {
                self.onHandUpdate?(right, nil)
            }
        }
    }

    private func extractHandData(from observation: VNHumanHandPoseObservation, timestamp: UInt64) -> VRHandData {
        var landmarks: [VRHandLandmark] = []

        for jointType in landmarkOrder {
            let visionJoint: VNHumanHandPoseObservation.JointName
            switch jointType {
            case .wrist: visionJoint = .wrist
            case .thumbCMC: visionJoint = .thumbCMC
            case .thumbMCP: visionJoint = .thumbMP
            case .thumbIP: visionJoint = .thumbIP
            case .thumbTip: visionJoint = .thumbTip
            case .indexMCP: visionJoint = .indexMCP
            case .indexPIP: visionJoint = .indexPIP
            case .indexDIP: visionJoint = .indexDIP
            case .indexTip: visionJoint = .indexTip
            case .middleMCP: visionJoint = .middleMCP
            case .middlePIP: visionJoint = .middlePIP
            case .middleDIP: visionJoint = .middleDIP
            case .middleTip: visionJoint = .middleTip
            case .ringMCP: visionJoint = .ringMCP
            case .ringPIP: visionJoint = .ringPIP
            case .ringDIP: visionJoint = .ringDIP
            case .ringTip: visionJoint = .ringTip
            case .littleMCP: visionJoint = .littleMCP
            case .littlePIP: visionJoint = .littlePIP
            case .littleDIP: visionJoint = .littleDIP
            case .littleTip: visionJoint = .littleTip
            }

            if let point = try? observation.recognizedPoint(visionJoint), point.confidence > 0.3 {
                landmarks.append(VRHandLandmark(
                    position: VRVector3(
                        x: Float(point.location.x),
                        y: Float(point.location.y),
                        z: 0
                    ),
                    confidence: Float(point.confidence),
                    jointType: jointType
                ))
            }
        }

        let gesture = recognizeGesture(from: landmarks)
        let wristPos = landmarks.first(where: { $0.jointType == .wrist })?.position ?? .zero

        return VRHandData(
            handId: observation.chirality == .left ? 0 : 1,
            landmarks: landmarks,
            handRadius: estimateHandRadius(landmarks: landmarks),
            wristPosition: wristPos,
            wristRotation: VRQuaternion.identity,
            gesture: gesture,
            timestampUs: timestamp
        )
    }

    private func recognizeGesture(from landmarks: [VRHandLandmark]) -> VRHandGesture {
        let tips: [VRHandLandmark.HandJointType] = [.thumbTip, .indexTip, .middleTip, .ringTip, .littleTip]
        let tipPositions = tips.compactMap { tip in landmarks.first(where: { $0.jointType == tip })?.position }
        let mcpPositions: [VRHandLandmark.HandJointType] = [.indexMCP, .middleMCP, .ringMCP, .littleMCP]
        _ = mcpPositions.compactMap { mcp in landmarks.first(where: { $0.jointType == mcp })?.position }

        guard tipPositions.count >= 3, let indexTip = landmarks.first(where: { $0.jointType == .indexTip })?.position,
              let thumbTip = landmarks.first(where: { $0.jointType == .thumbTip })?.position,
              let wrist = landmarks.first(where: { $0.jointType == .wrist })?.position else {
            return .unknown
        }

        let thumbIndexDist = simd_distance(thumbTip.simd, indexTip.simd)

        let extendedFingers = tipPositions.filter { tip in
            let wristDist = simd_distance(tip.simd, wrist.simd)
            let shouldBeExtended = wristDist > 0.15
            return shouldBeExtended
        }.count

        if thumbIndexDist < 0.04 && extendedFingers <= 2 {
            return .pinch
        }

        if extendedFingers <= 1 && thumbIndexDist < 0.06 {
            return .fist
        }

        if extendedFingers >= 4 && thumbIndexDist > 0.1 {
            return .openHand
        }

        if let indexTip = landmarks.first(where: { $0.jointType == .indexTip })?.position,
           let indexPIP = landmarks.first(where: { $0.jointType == .indexPIP })?.position,
           let thumbTip = landmarks.first(where: { $0.jointType == .thumbTip })?.position {
            let indexExtended = simd_distance(indexTip.simd, indexPIP.simd) > 0.05
            let thumbClosed = simd_distance(thumbTip.simd, indexPIP.simd) < 0.04
            if indexExtended && thumbClosed && extendedFingers <= 2 {
                return .point
            }
        }

        if thumbIndexDist > 0.15 && extendedFingers <= 2 {
            return .thumbUp
        }

        if thumbIndexDist < 0.08 && extendedFingers >= 3 {
            return .grab
        }

        return .unknown
    }

    private func estimateHandRadius(landmarks: [VRHandLandmark]) -> Float {
        guard let wrist = landmarks.first(where: { $0.jointType == .wrist })?.position else {
            return 0.08
        }
        let tips: [VRHandLandmark.HandJointType] = [.indexTip, .middleTip, .ringTip, .littleTip]
        let maxDist = tips.compactMap { tip in
            landmarks.first(where: { $0.jointType == tip })?.position
        }.map { tip in
            simd_distance(tip.simd, wrist.simd)
        }.max() ?? 0.08
        return maxDist
    }
}
