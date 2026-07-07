#pragma once

#include <cstdint>
#include <wrl/client.h>
#include <d3d12.h>

using Microsoft::WRL::ComPtr;

namespace phonevr {

class StereoRenderTarget {
public:
    StereoRenderTarget() = default;
    ~StereoRenderTarget() = default;

    bool initialize(ID3D12Device* device, uint32_t width, uint32_t height);
    void release();

    ID3D12Resource* left_eye() const { return left_eye_.Get(); }
    ID3D12Resource* right_eye() const { return right_eye_.Get(); }
    ID3D12Resource* current_eye() const { return current_eye_; }

    void swap_eyes();

private:
    ComPtr<ID3D12Resource> left_eye_;
    ComPtr<ID3D12Resource> right_eye_;
    ID3D12Resource* current_eye_ = nullptr;
    bool initialized_ = false;
};

} // namespace phonevr
