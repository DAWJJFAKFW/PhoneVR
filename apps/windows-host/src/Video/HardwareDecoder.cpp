#include "Video/HardwareDecoder.h"
#include <iostream>

#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "mfreadwrite")

namespace phonevr {

HardwareDecoder::HardwareDecoder() {
    MFStartup(MF_VERSION);
}

HardwareDecoder::~HardwareDecoder() {
    shutdown();
    MFShutdown();
}

bool HardwareDecoder::initialize() {
    initialized_ = true;
    return true;
}

void HardwareDecoder::shutdown() {
    decoder_.Reset();
    output_buffer_.Reset();
    output_sample_.Reset();
    initialized_ = false;
}

bool HardwareDecoder::create_decoder(VideoCodec codec, uint32_t width,
                                      uint32_t height,
                                      const std::vector<uint8_t>& sps_pps) {
    decoder_.Reset();
    output_buffer_.Reset();
    output_sample_.Reset();

    MFT_REGISTER_TYPE_INFO output_type = {};
    MFT_REGISTER_TYPE_INFO input_type = {};

    if (codec == VideoCodec::H264) {
        input_type.guidMajorType = MFMediaType_Video;
        input_type.guidSubtype = MFVideoFormat_H264;
    } else {
        input_type.guidMajorType = MFMediaType_Video;
        input_type.guidSubtype = MFVideoFormat_HEVC;
    }

    output_type.guidMajorType = MFMediaType_Video;
    output_type.guidSubtype = MFVideoFormat_NV12;

    IMFActivate** activates = nullptr;
    UINT32 count = 0;

    HRESULT hr = MFTEnumEx(MFT_CATEGORY_VIDEO_DECODER,
                           MFT_ENUM_FLAG_HARDWARE | MFT_ENUM_FLAG_SORTANDFILTER,
                           &input_type, &output_type, &activates, &count);

    if (FAILED(hr) || count == 0) {
        hr = MFTEnumEx(MFT_CATEGORY_VIDEO_DECODER,
                       MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_SORTANDFILTER,
                       &input_type, &output_type, &activates, &count);
    }

    if (FAILED(hr) || count == 0) {
        std::cerr << "No decoder found for "
                  << (codec == VideoCodec::H264 ? "H264" : "HEVC") << std::endl;
        return false;
    }

    hr = activates[0]->ActivateObject(IID_PPV_ARGS(&decoder_));

    for (UINT32 i = 0; i < count; ++i) {
        activates[i]->Release();
    }
    CoTaskMemFree(activates);

    if (FAILED(hr)) {
        std::cerr << "Failed to activate decoder: " << hr << std::endl;
        return false;
    }

    current_codec_ = codec;
    current_width_ = width;
    current_height_ = height;

    // Set input type
    ComPtr<IMFMediaType> input_media_type;
    MFCreateMediaType(&input_media_type);
    input_media_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    input_media_type->SetGUID(MF_MT_SUBTYPE,
                              codec == VideoCodec::H264 ? MFVideoFormat_H264 : MFVideoFormat_HEVC);
    MFSetAttributeSize(input_media_type.Get(), MF_MT_FRAME_SIZE, width, height);

    hr = decoder_->SetInputType(0, input_media_type.Get(), 0);
    if (FAILED(hr)) {
        std::cerr << "Failed to set input type: " << hr << std::endl;
        return false;
    }

    // Set output type
    ComPtr<IMFMediaType> output_media_type;
    MFCreateMediaType(&output_media_type);
    output_media_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    output_media_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    MFSetAttributeSize(output_media_type.Get(), MF_MT_FRAME_SIZE, width, height);

    hr = decoder_->SetOutputType(0, output_media_type.Get(), 0);
    if (FAILED(hr)) {
        std::cerr << "Failed to set output type: " << hr << std::endl;
        return false;
    }

    hr = decoder_->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    hr = decoder_->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);

    if (!sps_pps.empty()) {
        ComPtr<IMFSample> sps_sample;
        MFCreateSample(&sps_sample);

        ComPtr<IMFMediaBuffer> sps_buffer;
        MFCreateMemoryBuffer(static_cast<DWORD>(sps_pps.size()), &sps_buffer);

        BYTE* data = nullptr;
        sps_buffer->Lock(&data, nullptr, nullptr);
        std::memcpy(data, sps_pps.data(), sps_pps.size());
        sps_buffer->Unlock();
        sps_buffer->SetCurrentLength(static_cast<DWORD>(sps_pps.size()));

        sps_sample->AddBuffer(sps_buffer.Get());

        MFT_INPUT_STREAM_INFO info;
        decoder_->GetInputStreamInfo(0, &info);

        decoder_->ProcessInput(0, sps_sample.Get(), 0);
    }

    return true;
}

std::optional<DecodedFrame> HardwareDecoder::decode(const VideoPacket& packet) {
    if (!initialized_) return std::nullopt;

    if (!decoder_ || current_width_ != packet.header.width ||
        current_height_ != packet.header.height) {
        if (!create_decoder(packet.header.codec, packet.header.width,
                            packet.header.height, packet.header.sps_pps)) {
            return std::nullopt;
        }
    }

    // Send compressed data to decoder
    ComPtr<IMFSample> input_sample;
    MFCreateSample(&input_sample);

    ComPtr<IMFMediaBuffer> input_buffer;
    MFCreateMemoryBuffer(static_cast<DWORD>(packet.nal_data.size()), &input_buffer);

    BYTE* data = nullptr;
    input_buffer->Lock(&data, nullptr, nullptr);
    std::memcpy(data, packet.nal_data.data(), packet.nal_data.size());
    input_buffer->Unlock();
    input_buffer->SetCurrentLength(static_cast<DWORD>(packet.nal_data.size()));

    input_sample->AddBuffer(input_buffer.Get());

    MFT_OUTPUT_STREAM_INFO stream_info;
    decoder_->GetOutputStreamInfo(0, &stream_info);

    HRESULT hr = decoder_->ProcessInput(0, input_sample.Get(), 0);
    if (FAILED(hr)) {
        return std::nullopt;
    }

    // Get decoded output
    ComPtr<IMFSample> out_sample;
    MFCreateSample(&out_sample);

    ComPtr<IMFMediaBuffer> out_buffer;
    MFCreateMemoryBuffer(stream_info.cbSize, &out_buffer);

    MFT_OUTPUT_DATA_BUFFER output_data = {};
    output_data.dwStreamID = 0;
    output_data.pSample = out_sample.Get();
    output_data.dwStatus = 0;

    DWORD status = 0;
    hr = decoder_->ProcessOutput(0, 1, &output_data, &status);

    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
        return std::nullopt;
    }

    if (FAILED(hr)) {
        return std::nullopt;
    }

    ComPtr<IMFMediaBuffer> decoded_buffer;
    hr = out_sample->GetBufferByIndex(0, &decoded_buffer);
    if (FAILED(hr)) return std::nullopt;

    BYTE* buf_data = nullptr;
    DWORD buf_len = 0;
    decoded_buffer->Lock(&buf_data, nullptr, &buf_len);

    DecodedFrame frame;
    frame.data.resize(buf_len);
    std::memcpy(frame.data.data(), buf_data, buf_len);
    frame.width = packet.header.width;
    frame.height = packet.header.height;
    frame.codec = packet.header.codec;
    frame.eye = packet.header.eye;
    frame.pts_us = packet.header.pts_us;
    frame.is_keyframe = packet.is_keyframe;

    decoded_buffer->Unlock();

    return frame;
}

} // namespace phonevr
