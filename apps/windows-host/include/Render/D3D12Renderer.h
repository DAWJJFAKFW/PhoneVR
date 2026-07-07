#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include "PhoneVRTypes.h"
#include "Config.h"
#include "Video/HardwareDecoder.h"

using Microsoft::WRL::ComPtr;

namespace phonevr {

class D3D12Renderer {
public:
    D3D12Renderer();
    ~D3D12Renderer();

    D3D12Renderer(const D3D12Renderer&) = delete;
    D3D12Renderer& operator=(const D3D12Renderer&) = delete;

    bool initialize(const HostConfig& config);
    void shutdown();
    void render_frame();

    void update_pose(const Pose& pose);
    void update_hands(const std::optional<HandData>& left,
                      const std::optional<HandData>& right);
    void present_frame(const DecodedFrame& frame, VideoEye eye);

private:
    struct FrameContext {
        ComPtr<ID3D12CommandAllocator> command_allocator;
        ComPtr<ID3D12Resource> back_buffer;
        UINT64 fence_value = 0;
    };

    bool create_device();
    bool create_swapchain(HWND hwnd);
    bool create_render_resources();
    bool create_shaders();
    void wait_for_gpu();

    static constexpr int NUM_FRAMES = 3;

    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12CommandQueue> command_queue_;
    ComPtr<ID3D12CommandAllocator> command_allocator_;
    ComPtr<ID3D12GraphicsCommandList> command_list_;
    ComPtr<ID3D12Fence> fence_;
    HANDLE fence_event_ = nullptr;

    ComPtr<IDXGISwapChain3> swap_chain_;
    FrameContext frame_contexts_[NUM_FRAMES];

    ComPtr<ID3D12RootSignature> root_signature_;
    ComPtr<ID3D12PipelineState> pipeline_state_;

    ComPtr<ID3D12DescriptorHeap> rtv_heap_;
    UINT rtv_descriptor_size_ = 0;

    ComPtr<ID3D12Resource> left_eye_texture_;
    ComPtr<ID3D12Resource> right_eye_texture_;

    HWND hwnd_ = nullptr;
    HostConfig config_;
    uint32_t frame_index_ = 0;
    UINT64 fence_value_ = 0;

    struct SceneUniforms {
        DirectX::XMFLOAT4X4 left_eye_proj;
        DirectX::XMFLOAT4X4 right_eye_proj;
        DirectX::XMFLOAT4X4 head_pose;
        float ipd;
        float padding[3];
    };

    SceneUniforms uniforms_;
    Pose current_pose_;
};

} // namespace phonevr
