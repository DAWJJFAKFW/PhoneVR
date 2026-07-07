#include "Render/D3D12Renderer.h"
#include <iostream>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler")

namespace phonevr {

D3D12Renderer::D3D12Renderer() = default;
D3D12Renderer::~D3D12Renderer() { shutdown(); }

bool D3D12Renderer::initialize(const HostConfig& config) {
    config_ = config;

    hwnd_ = CreateWindowEx(
        0, L"PhoneVR", L"PhoneVR Host",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        static_cast<int>(config.render_width * 2),
        static_cast<int>(config.render_height),
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    if (!hwnd_) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }

    ShowWindow(hwnd_, SW_SHOW);

    if (!create_device()) return false;
    if (!create_swapchain(hwnd_)) return false;
    if (!create_render_resources()) return false;
    if (!create_shaders()) return false;

    return true;
}

void D3D12Renderer::shutdown() {
    wait_for_gpu();
    CloseHandle(fence_event_);
}

bool D3D12Renderer::create_device() {
    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1,
                                         IID_PPV_ARGS(&device_)))) {
            break;
        }
    }

    if (!device_) return false;

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue_));

    device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    fence_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    return true;
}

bool D3D12Renderer::create_swapchain(HWND hwnd) {
    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    ComPtr<IDXGISwapChain1> swap_chain1;
    DXGI_SWAP_CHAIN_DESC1 swap_desc = {};
    swap_desc.BufferCount = NUM_FRAMES;
    swap_desc.Width = config_.render_width * 2;
    swap_desc.Height = config_.render_height;
    swap_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_desc.SampleDesc.Count = 1;

    factory->CreateSwapChainForHwnd(command_queue_.Get(), hwnd, &swap_desc,
                                     nullptr, nullptr, &swap_chain1);
    swap_chain1.As(&swap_chain_);

    return true;
}

bool D3D12Renderer::create_render_resources() {
    // RTV heap
    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
    rtv_heap_desc.NumDescriptors = NUM_FRAMES;
    rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    device_->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap_));
    rtv_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Frame contexts
    for (int i = 0; i < NUM_FRAMES; ++i) {
        device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                         IID_PPV_ARGS(&frame_contexts_[i].command_allocator));

        swap_chain_->GetBuffer(i, IID_PPV_ARGS(&frame_contexts_[i].back_buffer));

        D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
        rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(
            rtv_heap_->GetCPUDescriptorHandleForHeapStart(),
            i, rtv_descriptor_size_);

        device_->CreateRenderTargetView(frame_contexts_[i].back_buffer.Get(),
                                         &rtv_desc, rtv_handle);
    }

    device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                frame_contexts_[0].command_allocator.Get(),
                                nullptr, IID_PPV_ARGS(&command_list_));
    command_list_->Close();

    // Eye textures
    D3D12_HEAP_PROPERTIES heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_NV12, config_.render_width, config_.render_height, 1, 1);

    device_->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE,
                                      &tex_desc, D3D12_RESOURCE_STATE_COMMON,
                                      nullptr, IID_PPV_ARGS(&left_eye_texture_));
    device_->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE,
                                      &tex_desc, D3D12_RESOURCE_STATE_COMMON,
                                      nullptr, IID_PPV_ARGS(&right_eye_texture_));

    return true;
}

bool D3D12Renderer::create_shaders() {
    // Root signature
    CD3DX12_ROOT_PARAMETER1 root_params[2];
    root_params[0].InitAsConstantBufferView(0);
    root_params[1].InitAsDescriptorTable(1);

    CD3DX12_DESCRIPTOR_RANGE1 tex_range;
    tex_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    root_params[1].InitAsDescriptorTable(1, &tex_range);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc;
    root_sig_desc.Init_1_1(2, root_params, 0, nullptr,
                           D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> root_sig_blob;
    ComPtr<ID3DBlob> error_blob;
    D3D12SerializeVersionedRootSignature(&root_sig_desc, &root_sig_blob, &error_blob);
    device_->CreateRootSignature(0, root_sig_blob->GetBufferPointer(),
                                  root_sig_blob->GetBufferSize(),
                                  IID_PPV_ARGS(&root_signature_));

    // Shaders
    ComPtr<ID3DBlob> vertex_shader;
    ComPtr<ID3DBlob> pixel_shader;

    const char* vs_code = R"(
        struct VSOut {
            float4 pos : SV_Position;
            float2 tex : TEXCOORD0;
        };
        VSOut main(float3 pos : POSITION, float2 tex : TEXCOORD) {
            VSOut out;
            out.pos = float4(pos, 1.0);
            out.tex = tex;
            return out;
        }
    )";

    const char* ps_code = R"(
        Texture2D tex : register(t0);
        SamplerState sam : register(s0);
        float4 main(float4 pos : SV_Position, float2 tex : TEXCOORD0) : SV_Target {
            return tex.Sample(sam, tex);
        }
    )";

    D3DCompile(vs_code, strlen(vs_code), nullptr, nullptr, nullptr, "main",
               "vs_5_0", 0, 0, &vertex_shader, &error_blob);
    D3DCompile(ps_code, strlen(ps_code), nullptr, nullptr, nullptr, "main",
               "ps_5_0", 0, 0, &pixel_shader, &error_blob);

    D3D12_INPUT_ELEMENT_DESC input_layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.InputLayout = {input_layout, 2};
    pso_desc.pRootSignature = root_signature_.Get();
    pso_desc.VS = {vertex_shader->GetBufferPointer(), vertex_shader->GetBufferSize()};
    pso_desc.PS = {pixel_shader->GetBufferPointer(), pixel_shader->GetBufferSize()};
    pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pso_desc.DepthStencilState = CD3DX12_DEPTHSTENCIL_DESC(D3D12_DEFAULT);
    pso_desc.SampleMask = UINT_MAX;
    pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso_desc.NumRenderTargets = 1;
    pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso_desc.SampleDesc.Count = 1;

    device_->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline_state_));

    return true;
}

void D3D12Renderer::render_frame() {
    frame_index_ = swap_chain_->GetCurrentBackBufferIndex();
    auto& ctx = frame_contexts_[frame_index_];

    ctx.command_allocator->Reset();
    command_list_->Reset(ctx.command_allocator.Get(), pipeline_state_.Get());

    // Set viewport for left eye
    D3D12_VIEWPORT viewport = {
        0, 0,
        static_cast<float>(config_.render_width),
        static_cast<float>(config_.render_height),
        0, 1
    };
    command_list_->RSSetViewports(1, &viewport);

    D3D12_RECT scissor = {0, 0,
                          static_cast<LONG>(config_.render_width),
                          static_cast<LONG>(config_.render_height)};
    command_list_->RSSetScissorRects(1, &scissor);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(
        rtv_heap_->GetCPUDescriptorHandleForHeapStart(),
        frame_index_, rtv_descriptor_size_);

    command_list_->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);

    float clear_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
    command_list_->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);

    command_list_->SetGraphicsRootSignature(root_signature_.Get());

    // TODO: Draw stereoscopic scene

    command_list_->Close();

    ID3D12CommandList* lists[] = {command_list_.Get()};
    command_queue_->ExecuteCommandLists(1, lists);

    swap_chain_->Present(1, 0);

    wait_for_gpu();
}

void D3D12Renderer::update_pose(const Pose& pose) {
    current_pose_ = pose;

    DirectX::XMVECTOR pos = DirectX::XMVectorSet(
        pose.position.x, pose.position.y, pose.position.z, 1.0f);
    DirectX::XMVECTOR quat = DirectX::XMVectorSet(
        pose.orientation.x, pose.orientation.y,
        pose.orientation.z, pose.orientation.w);

    DirectX::XMMATRIX rot = DirectX::XMMatrixRotationQuaternion(quat);
    DirectX::XMMATRIX trans = DirectX::XMMatrixTranslationFromVector(pos);

    DirectX::XMStoreFloat4x4(&uniforms_.head_pose, rot * trans);
    uniforms_.ipd = config_.render_width * 0.001f;
}

void D3D12Renderer::update_hands(const std::optional<HandData>& left,
                                   const std::optional<HandData>& right) {}

void D3D12Renderer::present_frame(const DecodedFrame& frame, VideoEye eye) {}

void D3D12Renderer::wait_for_gpu() {
    fence_value_++;
    command_queue_->Signal(fence_.Get(), fence_value_);

    if (fence_->GetCompletedValue() < fence_value_) {
        fence_->SetEventOnCompletion(fence_value_, fence_event_);
        WaitForSingleObject(fence_event_, INFINITE);
    }
}

} // namespace phonevr
