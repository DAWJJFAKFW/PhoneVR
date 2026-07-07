import Foundation
import CoreMotion
import simd

public class SensorFusionEngine {
    private let motionManager = CMMotionManager()
    private let queue = OperationQueue()

    private var complementaryFilter: ComplementaryFilter
    private var kalmanFilter: KalmanFilter
    private var motionPredictor: MotionPredictor

    private var referenceFrame: simd_quatf = .init(ix: 0, iy: 0, iz: 0, r: 1)
    private var recenterOffset: simd_quatf = .init(ix: 0, iy: 0, iz: 0, r: 1)
    private var isRecentered: Bool = false

    public private(set) var currentPose = VRPose()
    public private(set) var currentImu = VRImuData()

    public var onPoseUpdate: ((VRPose) -> Void)?
    public var sensitivity: Float = 1.0

    public init() {
        self.complementaryFilter = ComplementaryFilter()
        self.kalmanFilter = KalmanFilter()
        self.motionPredictor = MotionPredictor()
        self.queue.maxConcurrentOperationCount = 1
        self.queue.qualityOfService = .userInteractive
    }

    public func startTracking() {
        guard motionManager.isDeviceMotionAvailable else { return }

        motionManager.deviceMotionUpdateInterval = 1.0 / 1000.0
        motionManager.showsDeviceMovementDisplay = true

        motionManager.startDeviceMotionUpdates(
            using: .xArbitraryZVertical,
            to: queue
        ) { [weak self] motion, error in
            guard let self = self, let motion = motion, error == nil else { return }
            self.processMotionUpdate(motion)
        }
    }

    public func stopTracking() {
        motionManager.stopDeviceMotionUpdates()
    }

    private func processMotionUpdate(_ motion: CMDeviceMotion) {
        let timestamp = motion.timestamp
        let timestampUs = UInt64(timestamp * 1_000_000)

        let rawAccel = motion.userAcceleration
        let rawGyro = motion.rotationRate
        let rawMag = motion.magneticField.field

        currentImu = VRImuData(
            accelerometer: VRVector3(
                x: Float(rawAccel.x),
                y: Float(rawAccel.y),
                z: Float(rawAccel.z)
            ),
            gyroscope: VRVector3(
                x: Float(rawGyro.x),
                y: Float(rawGyro.y),
                z: Float(rawGyro.z)
            ),
            magnetometer: VRVector3(
                x: Float(rawMag.x),
                y: Float(rawMag.y),
                z: Float(rawMag.z)
            ),
            timestampUs: timestampUs
        )

        let attitude = motion.attitude

        let cmQuat = attitude.quaternion
        var rawQuat = simd_quatf(ix: Float(cmQuat.x), iy: Float(cmQuat.y), iz: Float(cmQuat.z), r: Float(cmQuat.w))

        if sensitivity != 1.0 {
            let axis = rawQuat.axis
            let angle = rawQuat.angle * sensitivity
            rawQuat = simd_quatf(angle: angle, axis: axis)
        }

        let fused = complementaryFilter.update(
            gyro: rawGyro,
            accel: motion.gravity,
            quat: rawQuat,
            dt: 1.0 / 1000.0
        )

        let kalmanCorrected = kalmanFilter.update(measurement: fused, dt: 1.0 / 1000.0)

        let predicted = motionPredictor.predict(
            current: kalmanCorrected,
            gyro: rawGyro,
            predictionTime: 0.008
        )

        var finalQuat = predicted
        if isRecentered {
            finalQuat = recenterOffset * finalQuat
        }

        currentPose = VRPose(
            position: VRVector3(
                x: Float(motion.userAcceleration.x * 9.81),
                y: Float(motion.userAcceleration.y * 9.81),
                z: Float(motion.userAcceleration.z * 9.81)
            ),
            orientation: VRQuaternion(
                x: finalQuat.imag.x,
                y: finalQuat.imag.y,
                z: finalQuat.imag.z,
                w: finalQuat.real
            ),
            timestampUs: timestampUs
        )

        DispatchQueue.main.async { [weak self] in
            guard let self = self else { return }
            self.onPoseUpdate?(self.currentPose)
        }
    }

    public func recenter() {
        let currentAttitude = simd_quatf(
            ix: currentPose.orientation.x,
            iy: currentPose.orientation.y,
            iz: currentPose.orientation.z,
            r: currentPose.orientation.w
        )
        recenterOffset = currentAttitude.inverse
        isRecentered = true
    }

    public func reset() {
        isRecentered = false
        recenterOffset = .init(ix: 0, iy: 0, iz: 0, r: 1)
        kalmanFilter.reset()
        motionPredictor.reset()
    }
}

// MARK: - Complementary Filter

private class ComplementaryFilter {
    private var filteredQuat = simd_quatf(ix: 0, iy: 0, iz: 0, r: 1)
    private let alpha: Float = 0.98

    func update(gyro: CMRotationRate, accel: CMAcceleration, quat: simd_quatf, dt: Float) -> simd_quatf {
        let gyroRate = simd_float3(x: Float(gyro.x), y: Float(gyro.y), z: Float(gyro.z))
        let gyroStep = simd_quatf(angle: simd_length(gyroRate) * dt, axis: simd_normalize(gyroRate))

        let gyroQuat = filteredQuat * gyroStep

        let accelQuat = accelToQuaternion(accel)

        filteredQuat = simd_slerp(gyroQuat, accelQuat, 1.0 - alpha)
        filteredQuat = simd_normalize(filteredQuat)

        return filteredQuat
    }

    private func accelToQuaternion(_ accel: CMAcceleration) -> simd_quatf {
        let g = simd_normalize(simd_float3(x: Float(accel.x), y: Float(accel.y), z: Float(accel.z)))
        let ref = simd_float3(x: 0, y: -1, z: 0)
        let dot = simd_dot(g, ref)
        let cross = simd_cross(g, ref)
        let w = sqrt(1 + dot) * 0.5
        let scale = sqrt(1 - dot * dot) * 0.5
        if scale < 0.001 { return .init(ix: 0, iy: 0, iz: 0, r: 1) }
        return simd_quatf(ix: cross.x * scale, iy: cross.y * scale, iz: cross.z * scale, r: w)
    }

    func reset() {
        filteredQuat = .init(ix: 0, iy: 0, iz: 0, r: 1)
    }
}

// MARK: - Kalman Filter

private class KalmanFilter {
    private var stateEstimate = simd_float4(0, 0, 0, 1)
    private var errorCovariance = matrix_identity_float4x4
    private let processNoise: Float = 0.001
    private let measurementNoise: Float = 0.01

    func update(measurement: simd_quatf, dt: Float) -> simd_quatf {
        let F = matrix_identity_float4x4
        let Q = matrix_identity_float4x4 * processNoise
        let H = matrix_identity_float4x4
        let R = matrix_identity_float4x4 * measurementNoise

        let predictedState = F * stateEstimate
        let predictedCovariance = F * errorCovariance * F.transpose + Q

        let innovation = simd_float4(measurement.imag.x, measurement.imag.y, measurement.imag.z, measurement.real) - H * predictedState
        let S = H * predictedCovariance * H.transpose + R
        let K = predictedCovariance * H.transpose * S.inverse

        stateEstimate = predictedState + K * innovation
        errorCovariance = (matrix_identity_float4x4 - K * H) * predictedCovariance

        let quat = simd_quatf(ix: stateEstimate.x, iy: stateEstimate.y, iz: stateEstimate.z, r: stateEstimate.w)
        return simd_normalize(quat)
    }

    func reset() {
        stateEstimate = simd_float4(0, 0, 0, 1)
        errorCovariance = matrix_identity_float4x4
    }
}

// MARK: - Motion Predictor

private class MotionPredictor {
    private var previousQuat = simd_quatf(ix: 0, iy: 0, iz: 0, r: 1)
    private var angularVelocity = simd_float3.zero

    func predict(current: simd_quatf, gyro: CMRotationRate, predictionTime: Float) -> simd_quatf {
        let gyroVec = simd_float3(x: Float(gyro.x), y: Float(gyro.y), z: Float(gyro.z))
        angularVelocity = angularVelocity * 0.9 + gyroVec * 0.1

        let predictionStep = simd_quatf(
            angle: simd_length(angularVelocity) * predictionTime,
            axis: simd_normalize(angularVelocity)
        )
        let predicted = current * predictionStep

        previousQuat = current
        return simd_normalize(predicted)
    }

    func reset() {
        previousQuat = .init(ix: 0, iy: 0, iz: 0, r: 1)
        angularVelocity = .zero
    }
}
