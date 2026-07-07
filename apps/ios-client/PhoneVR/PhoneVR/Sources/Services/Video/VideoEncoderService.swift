import Foundation
import VideoToolbox
import CoreVideo
import CoreMedia
import AVFoundation

public class VideoEncoderService {
    private var compressionSession: VTCompressionSession?
    private var encodeQueue = DispatchQueue(label: "com.phonevr.encode", qos: .userInteractive)

    public var onEncodedFrame: ((Data, VRVideoFrameHeader) -> Void)?
    public private(set) var isEncoding = false

    fileprivate var frameSequence: UInt32 = 0
    fileprivate var lastSPSPPS: Data?
    fileprivate var forceKeyframe: Bool = false

    public var targetBitrate: UInt32 = 100_000_000
    public var targetFPS: UInt32 = 90
    public var codecType: VRVideoCodec = .h265

    public func startEncoding(width: Int32, height: Int32) {
        guard !isEncoding else { return }

        let codec: CMVideoCodecType = codecType == .h265 ? kCMVideoCodecType_HEVC : kCMVideoCodecType_H264

        let encoderSpecification: NSDictionary = {
            if #available(iOS 17.4, *) {
                return [
                    kVTVideoEncoderSpecification_RequireHardwareAcceleratedVideoEncoder: true,
                    kVTVideoEncoderSpecification_EncoderID: codecType == .h265
                        ? "com.apple.videotoolbox.videoencoder.hevc" : "com.apple.videotoolbox.videoencoder.h264"
                ]
            } else {
                return [
                    kVTVideoEncoderSpecification_EncoderID: codecType == .h265
                        ? "com.apple.videotoolbox.videoencoder.hevc" : "com.apple.videotoolbox.videoencoder.h264"
                ]
            }
        }()

        var session: VTCompressionSession?
        let status = VTCompressionSessionCreate(
            allocator: kCFAllocatorDefault,
            width: width,
            height: height,
            codecType: codec,
            encoderSpecification: encoderSpecification,
            imageBufferAttributes: nil,
            compressedDataAllocator: kCFAllocatorDefault,
            outputCallback: encodeOutputCallback,
            refcon: Unmanaged.passUnretained(self).toOpaque(),
            compressionSessionOut: &session
        )

        guard status == noErr, let session = session else {
            print("Failed to create compression session: \(status)")
            return
        }

        self.compressionSession = session

        VTSessionSetProperty(session, key: kVTCompressionPropertyKey_RealTime, value: kCFBooleanTrue)
        VTSessionSetProperty(session, key: kVTCompressionPropertyKey_ProfileLevel, value: codecType == .h265
            ? kVTProfileLevel_HEVC_Main_AutoLevel : kVTProfileLevel_H264_High_AutoLevel)

        let bitrateKey = kVTCompressionPropertyKey_AverageBitRate
        VTSessionSetProperty(session, key: bitrateKey, value: NSNumber(value: targetBitrate))

        let fpsKey = kVTCompressionPropertyKey_ExpectedFrameRate
        VTSessionSetProperty(session, key: fpsKey, value: NSNumber(value: targetFPS))

        let limitKey = kVTCompressionPropertyKey_DataRateLimits
        let limit = [NSNumber(value: targetBitrate * 2), NSNumber(value: 1.0)]
        VTSessionSetProperty(session, key: limitKey, value: limit as NSArray)

        VTSessionSetProperty(session, key: kVTCompressionPropertyKey_AllowFrameReordering, value: kCFBooleanFalse)

        VTCompressionSessionPrepareToEncodeFrames(session)

        isEncoding = true
    }

    public func stopEncoding() {
        guard let session = compressionSession else { return }
        VTCompressionSessionInvalidate(session)
        compressionSession = nil
        isEncoding = false
        frameSequence = 0
    }

    public func encodeFrame(_ pixelBuffer: CVPixelBuffer, eye: VRVideoEye, pts: CMTime) {
        guard isEncoding, let session = compressionSession else { return }

        let frameSeq = frameSequence
        frameSequence += 1

        let duration = CMTime(value: 1, timescale: CMTimeScale(targetFPS))

        let isForced = forceKeyframe
        forceKeyframe = false
        let shouldForce = frameSeq % 90 == 0 || isForced
        let frameProperties: NSDictionary = [
            kVTEncodeFrameOptionKey_ForceKeyFrame: shouldForce as NSNumber
        ]

        VTCompressionSessionEncodeFrame(
            session,
            imageBuffer: pixelBuffer,
            presentationTimeStamp: pts,
            duration: duration,
            frameProperties: frameProperties as CFDictionary,
            sourceFrameRefcon: nil,
            infoFlagsOut: nil
        )
    }

    public func requestKeyframe() {
        guard isEncoding else { return }
        forceKeyframe = true
    }

    public func updateBitrate(_ bitrate: UInt32) {
        targetBitrate = bitrate
        guard let session = compressionSession else { return }
        VTSessionSetProperty(session, key: kVTCompressionPropertyKey_AverageBitRate,
                             value: NSNumber(value: bitrate))
        let limit = [NSNumber(value: bitrate * 2), NSNumber(value: 1.0)]
        VTSessionSetProperty(session, key: kVTCompressionPropertyKey_DataRateLimits,
                             value: limit as NSArray)
    }

    public func updateFPS(_ fps: UInt32) {
        targetFPS = fps
        guard let session = compressionSession else { return }
        VTSessionSetProperty(session, key: kVTCompressionPropertyKey_ExpectedFrameRate,
                             value: NSNumber(value: fps))
    }
}

// MARK: - VTCompressionSession Callback

private let encodeOutputCallback: VTCompressionOutputCallback = { (
    refcon: UnsafeMutableRawPointer?,
    frame: UnsafeMutableRawPointer?,
    status: OSStatus,
    infoFlags: VTEncodeInfoFlags,
    sampleBuffer: CMSampleBuffer?
) in
    guard status == noErr, let sampleBuffer = sampleBuffer,
          CMSampleBufferDataIsReady(sampleBuffer) else { return }

    let encoder = Unmanaged<VideoEncoderService>.fromOpaque(refcon!).takeUnretainedValue()

    guard let dataBuffer = CMSampleBufferGetDataBuffer(sampleBuffer) else { return }

    let attachements = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, createIfNecessary: false)
    let isKeyframe = attachements != nil && (attachements as! NSArray).contains { dict in
        (dict as! NSDictionary)[kCMSampleAttachmentKey_NotSync] == nil
    }

    var length: Int = 0
    var dataPointer: UnsafeMutablePointer<Int8>?
    CMBlockBufferGetDataPointer(dataBuffer, atOffset: 0, lengthAtOffsetOut: nil, totalLengthOut: &length, dataPointerOut: &dataPointer)

    guard let dataPtr = dataPointer, length > 0 else { return }

    let data = Data(bytes: dataPtr, count: length)

    var spsPps = Data()
    if isKeyframe {
        let formatDesc = CMSampleBufferGetFormatDescription(sampleBuffer)
        if let formatDesc = formatDesc {
            var spsSize: Int = 0
            var spsCount: Int = 0
            var nalHeaderLen: Int32 = 0
            var spsPtr: UnsafePointer<UInt8>?

            if CMVideoFormatDescriptionGetH264ParameterSetAtIndex(formatDesc, parameterSetIndex: 0, parameterSetPointerOut: &spsPtr, parameterSetSizeOut: &spsSize, parameterSetCountOut: &spsCount, nalUnitHeaderLengthOut: &nalHeaderLen) == noErr {
                spsPps.append(Data(bytes: spsPtr!, count: spsSize))
            }

            var ppsSize: Int = 0
            var ppsPtr: UnsafePointer<UInt8>?
            if CMVideoFormatDescriptionGetH264ParameterSetAtIndex(formatDesc, parameterSetIndex: 1, parameterSetPointerOut: &ppsPtr, parameterSetSizeOut: &ppsSize, parameterSetCountOut: nil, nalUnitHeaderLengthOut: nil) == noErr {
                spsPps.append(Data(bytes: ppsPtr!, count: ppsSize))
            }
        }
        encoder.lastSPSPPS = spsPps
    }

    let pts = CMSampleBufferGetPresentationTimeStamp(sampleBuffer)
    let ptsUs = UInt64(CMTimeGetSeconds(pts) * 1_000_000)

    let header = VRVideoFrameHeader(
        width: 0,
        height: 0,
        ptsUs: ptsUs,
        frameSequence: encoder.frameSequence,
        codec: encoder.codecType,
        eye: .both,
        nalCount: 0,
        spsPps: spsPps,
        frameSize: UInt32(length)
    )

    encoder.onEncodedFrame?(data, header)
}
