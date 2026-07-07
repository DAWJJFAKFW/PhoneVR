import Metal
import MetalKit
import simd

public class VRRenderer: NSObject, MTKViewDelegate {
    private let device: MTLDevice
    private let commandQueue: MTLCommandQueue
    private var pipelineState: MTLRenderPipelineState?
    private var distortionPipelineState: MTLRenderPipelineState?
    private var vertexBuffer: MTLBuffer?
    private var indexBuffer: MTLBuffer?
    private var uniformBuffer: MTLBuffer?
    private var distortionBuffer: MTLBuffer?
    private var textureCache: CVMetalTextureCache?

    private var viewportSize = simd_uint2(0, 0)
    private var leftTexture: MTLTexture?
    private var rightTexture: MTLTexture?

    public var ipd: Float = 0.0635
    public var lensDistortionK1: Float = -0.15
    public var lensDistortionK2: Float = 0.05
    public var chromaticAberration: Float = 0.002
    public var renderScale: Float = 1.0

    public override init() {
        guard let device = MTLCreateSystemDefaultDevice() else {
            fatalError("Metal is not supported on this device")
        }
        self.device = device
        self.commandQueue = device.makeCommandQueue()!
        super.init()
        setupTextureCache()
        setupPipelines()
        setupGeometry()
    }

    private func setupTextureCache() {
        var cache: CVMetalTextureCache?
        CVMetalTextureCacheCreate(
            kCFAllocatorDefault,
            nil,
            device,
            nil,
            &cache
        )
        self.textureCache = cache
    }

    private func setupPipelines() {
        guard let library = device.makeDefaultLibrary() else {
            print("Failed to load Metal library")
            return
        }

        let vertexFunc = library.makeFunction(name: "vrVertexShader")
        let fragmentFunc = library.makeFunction(name: "vrFragmentShader")
        let distortionFragFunc = library.makeFunction(name: "distortionFragmentShader")

        // Main scene pipeline
        let pipelineDesc = MTLRenderPipelineDescriptor()
        pipelineDesc.vertexFunction = vertexFunc
        pipelineDesc.fragmentFunction = fragmentFunc
        pipelineDesc.colorAttachments[0].pixelFormat = .bgra8Unorm
        pipelineDesc.depthAttachmentPixelFormat = .depth32Float

        do {
            pipelineState = try device.makeRenderPipelineState(descriptor: pipelineDesc)
        } catch {
            print("Failed to create scene pipeline: \(error)")
        }

        // Distortion correction pipeline
        let distortionDesc = MTLRenderPipelineDescriptor()
        distortionDesc.vertexFunction = vertexFunc
        distortionDesc.fragmentFunction = distortionFragFunc
        distortionDesc.colorAttachments[0].pixelFormat = .bgra8Unorm

        do {
            distortionPipelineState = try device.makeRenderPipelineState(descriptor: distortionDesc)
        } catch {
            print("Failed to create distortion pipeline: \(error)")
        }
    }

    private func setupGeometry() {
        let vertices: [VRVertex] = [
            VRVertex(pos: simd_float2(-1, -1), tex: simd_float2(0, 1)),
            VRVertex(pos: simd_float2( 1, -1), tex: simd_float2(1, 1)),
            VRVertex(pos: simd_float2(-1,  1), tex: simd_float2(0, 0)),
            VRVertex(pos: simd_float2( 1,  1), tex: simd_float2(1, 0)),
        ]
        vertexBuffer = device.makeBuffer(bytes: vertices, length: MemoryLayout<VRVertex>.stride * 4, options: .storageModeShared)

        var indices: [UInt16] = [0, 1, 2, 2, 1, 3]
        indexBuffer = device.makeBuffer(bytes: &indices, length: MemoryLayout<UInt16>.stride * 6, options: .storageModeShared)

        let screenWidth = UIScreen.main.nativeBounds.width
        let screenHeight = UIScreen.main.nativeBounds.height
        let halfWidth = Float(screenWidth) / 2.0 * renderScale
        let halfHeight = Float(screenHeight) * renderScale

        struct DistortionUniforms {
            var halfWidth: Float
            var halfHeight: Float
            var ipd: Float
            var k1: Float
            var k2: Float
            var chromaAb: Float
            var eyeOffset: Float
            var padding: Float
        }

        var distLeft = DistortionUniforms(halfWidth: halfWidth, halfHeight: halfHeight, ipd: ipd, k1: lensDistortionK1, k2: lensDistortionK2, chromaAb: chromaticAberration, eyeOffset: -ipd / 2.0, padding: 0)
        var distRight = DistortionUniforms(halfWidth: halfWidth, halfHeight: halfHeight, ipd: ipd, k1: lensDistortionK1, k2: lensDistortionK2, chromaAb: chromaticAberration, eyeOffset: ipd / 2.0, padding: 0)

        var uniforms: [DistortionUniforms] = [distLeft, distRight]
        distortionBuffer = device.makeBuffer(bytes: &uniforms, length: MemoryLayout<DistortionUniforms>.stride * 2, options: .storageModeShared)
    }

    public func createTextures(from pixelBuffer: CVPixelBuffer, eye: VRVideoEye) {
        guard let textureCache = textureCache else { return }

        let width = CVPixelBufferGetWidth(pixelBuffer)
        let height = CVPixelBufferGetHeight(pixelBuffer)

        var cvTexture: CVMetalTexture?
        CVMetalTextureCacheCreateTextureFromImage(
            kCFAllocatorDefault,
            textureCache,
            pixelBuffer,
            nil,
            .bgra8Unorm,
            width, height,
            0,
            &cvTexture
        )

        guard let metalTexture = cvTexture.flatMap({ CVMetalTextureGetTexture($0) }) else { return }

        switch eye {
        case .left:
            leftTexture = metalTexture
        case .right:
            rightTexture = metalTexture
        case .both:
            leftTexture = metalTexture
            rightTexture = metalTexture
        }
    }

    // MARK: - MTKViewDelegate

    public func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        viewportSize = simd_uint2(UInt32(size.width), UInt32(size.height))
    }

    public func draw(in view: MTKView) {
        guard let drawable = view.currentDrawable,
              let descriptor = view.currentRenderPassDescriptor,
              let commandBuffer = commandQueue.makeCommandBuffer(),
              let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: descriptor) else { return }

        if let state = distortionPipelineState {
            encoder.setRenderPipelineState(state)
            encoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
            encoder.setFragmentBuffer(distortionBuffer, offset: 0, index: 0)

            if let leftTex = leftTexture {
                encoder.setFragmentTexture(leftTex, index: 0)
            }

            var viewport = MTLViewport(
                originX: 0, originY: 0,
                width: Double(viewportSize.x / 2), height: Double(viewportSize.y),
                znear: -1, zfar: 1
            )
            encoder.setViewport(viewport)
            encoder.drawIndexedPrimitives(type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer!, indexBufferOffset: 0)

            if let rightTex = rightTexture {
                encoder.setFragmentTexture(rightTex, index: 0)
            }

            viewport.originX = Double(viewportSize.x / 2)
            encoder.setViewport(viewport)

            if let distortionBuf = distortionBuffer {
                let ptr = distortionBuf.contents().assumingMemoryBound(to: simd_float4.self)
                ptr.advanced(by: 4).pointee.x = ipd / 2.0
            }

            encoder.drawIndexedPrimitives(type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer!, indexBufferOffset: 0)
        }

        encoder.endEncoding()
        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
}

struct VRVertex {
    var pos: simd_float2
    var tex: simd_float2
}
