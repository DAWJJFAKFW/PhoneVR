import Foundation
import AVFoundation
import UIKit

public class CameraService: NSObject {
    private let captureSession = AVCaptureSession()
    private let sessionQueue = DispatchQueue(label: "com.phonevr.camera", qos: .userInitiated)
    private let videoOutput = AVCaptureVideoDataOutput()
    private var isRunning = false

    public var onPixelBuffer: ((CVPixelBuffer, UInt64) -> Void)?

    public func startCamera() {
        guard !isRunning else { return }
        sessionQueue.async { [weak self] in
            self?.setupCamera()
        }
    }

    public func stopCamera() {
        guard isRunning else { return }
        sessionQueue.async { [weak self] in
            self?.captureSession.stopRunning()
            self?.isRunning = false
        }
    }

    private func setupCamera() {
        captureSession.beginConfiguration()
        captureSession.sessionPreset = .high

        guard let camera = AVCaptureDevice.default(.builtInWideAngleCamera, for: .video, position: .front) else {
            print("No front camera available")
            captureSession.commitConfiguration()
            return
        }

        do {
            let input = try AVCaptureDeviceInput(device: camera)
            guard captureSession.canAddInput(input) else { return }
            captureSession.addInput(input)
        } catch {
            print("Camera input error: \(error)")
            captureSession.commitConfiguration()
            return
        }

        videoOutput.setSampleBufferDelegate(self, queue: sessionQueue)
        videoOutput.alwaysDiscardsLateVideoFrames = true
        videoOutput.videoSettings = [
            kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_420YpCbCr8BiPlanarFullRange
        ]

        guard captureSession.canAddOutput(videoOutput) else { return }
        captureSession.addOutput(videoOutput)

        if let connection = videoOutput.connection(with: .video) {
            connection.videoOrientation = .landscapeRight
            if connection.isVideoMirroringSupported {
                connection.isVideoMirrored = true
            }
        }

        captureSession.commitConfiguration()
        captureSession.startRunning()
        isRunning = true
    }
}

extension CameraService: AVCaptureVideoDataOutputSampleBufferDelegate {
    public func captureOutput(_ output: AVCaptureOutput, didOutput sampleBuffer: CMSampleBuffer, from connection: AVCaptureConnection) {
        guard let pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer) else { return }
        let timestamp = CMSampleBufferGetPresentationTimeStamp(sampleBuffer)
        let timestampUs = UInt64(CMTimeGetSeconds(timestamp) * 1_000_000)
        onPixelBuffer?(pixelBuffer, timestampUs)
    }
}
