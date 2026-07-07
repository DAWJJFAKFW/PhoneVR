#include "Render/StereoRenderTarget.h"
#include <iostream>

namespace phonevr {

bool StereoRenderTarget::initialize(ID3D12Device* device,
                                     uint32_t width, uint32_t height) {
    if (!device) return false;

    D3D12_HEAP_PROPERTIES heap_props = {};
    heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC resource_desc = {};
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource_desc.Width = width;
    resource_desc.Height = height;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = DXGI_FORMAT_NV12;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET |
                          D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    HRESULT hr = device->CreateCommittedResource(
        &heap_props, D3D12_HEAP_FLAG_NONE,
        &resource_desc, D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(&left_eye_));

    if (FAILED(hr)) {
        std::cerr << "Failed to create left eye texture: " << hr << std::endl;
        return false;
    }

    hr = device->CreateCommittedResource(
        &heap_props, D3D12_HEAP_FLAG_NONE,
        &resource_desc, D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(&right_eye_));

    if (FAILED(hr)) {
        std::cerr << "Failed to create right eye texture: " << hr << std::endl;
        return false;
    }

    current_eye_ = left_eye_.Get();
    initialized_ = true;
    return true;
}

void StereoRenderTarget::release() {
    left_eye_.Reset();
    right_eye_.Reset();
    current_eye_ = nullptr;
    initialized_ = false;
}

void StereoRenderTarget::swap_eyes() {
    current_eye_ = (current_eye_ == left_eye_.Get())
        ? right_eye_.Get()
        : left_eye_.Get();
}

} // namespace phonevr
