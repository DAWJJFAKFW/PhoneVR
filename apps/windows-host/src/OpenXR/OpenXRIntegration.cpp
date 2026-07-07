#include "OpenXR/OpenXRIntegration.h"
#include <iostream>

namespace phonevr {

OpenXRIntegration::~OpenXRIntegration() {
    shutdown();
}

bool OpenXRIntegration::initialize() {
#ifdef SUPPORT_OPENXR
    if (!create_instance()) return false;
    if (!create_session()) return false;
    if (!create_swapchains()) return false;

    available_ = true;
    return true;
#else
    std::cerr << "OpenXR support not compiled in" << std::endl;
    return false;
#endif
}

void OpenXRIntegration::shutdown() {
#ifdef SUPPORT_OPENXR
    if (left_swapchain_ != XR_NULL_HANDLE) {
        xrDestroySwapchain(left_swapchain_);
        left_swapchain_ = XR_NULL_HANDLE;
    }
    if (right_swapchain_ != XR_NULL_HANDLE) {
        xrDestroySwapchain(right_swapchain_);
        right_swapchain_ = XR_NULL_HANDLE;
    }
    if (head_space_ != XR_NULL_HANDLE) {
        xrDestroySpace(head_space_);
        head_space_ = XR_NULL_HANDLE;
    }
    if (local_space_ != XR_NULL_HANDLE) {
        xrDestroySpace(local_space_);
        local_space_ = XR_NULL_HANDLE;
    }
    if (session_ != XR_NULL_HANDLE) {
        xrDestroySession(session_);
        session_ = XR_NULL_HANDLE;
    }
    if (instance_ != XR_NULL_HANDLE) {
        xrDestroyInstance(instance_);
        instance_ = XR_NULL_HANDLE;
    }
#endif
    available_ = false;
    session_running_ = false;
}

bool OpenXRIntegration::create_instance() {
#ifdef SUPPORT_OPENXR
    XrApplicationInfo app_info = {};
    std::strncpy(app_info.applicationName, application_name_.c_str(),
                 XR_MAX_APPLICATION_NAME_SIZE);
    app_info.applicationVersion = 1;
    std::strncpy(app_info.engineName, engine_name_.c_str(),
                 XR_MAX_ENGINE_NAME_SIZE);
    app_info.engineVersion = 1;
    app_info.apiVersion = XR_CURRENT_API_VERSION;

    XrInstanceCreateInfo create_info = {XR_TYPE_INSTANCE_CREATE_INFO};
    create_info.applicationInfo = app_info;

    XrResult result = xrCreateInstance(&create_info, &instance_);
    if (XR_FAILED(result)) {
        std::cerr << "Failed to create XR instance: " << result << std::endl;
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool OpenXRIntegration::create_session() {
#ifdef SUPPORT_OPENXR
    XrSystemGetInfo system_info = {XR_TYPE_SYSTEM_GET_INFO};
    system_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XrResult result = xrGetSystem(instance_, &system_info, &system_id_);
    if (XR_FAILED(result)) {
        std::cerr << "No HMD system found: " << result << std::endl;
        return false;
    }

    // Enumerate view configurations
    xrEnumerateViewConfigurationViews(instance_, system_id_,
                                       XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                       0, &view_count_, nullptr);

    if (view_count_ > 0) {
        xrEnumerateViewConfigurationViews(instance_, system_id_,
                                           XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                           view_count_, &view_count_, view_configs_);
    }

    XrSessionCreateInfo session_info = {XR_TYPE_SESSION_CREATE_INFO};
    session_info.systemId = system_id_;

    result = xrCreateSession(instance_, &session_info, &session_);
    if (XR_FAILED(result)) {
        std::cerr << "Failed to create session: " << result << std::endl;
        return false;
    }

    XrReferenceSpaceCreateInfo space_info = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    space_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    space_info.poseInReferenceSpace = {
        {0, 0, 0, 1},
        {0, 0, 0}
    };

    result = xrCreateReferenceSpace(session_, &space_info, &local_space_);
    if (XR_FAILED(result)) {
        std::cerr << "Failed to create reference space: " << result << std::endl;
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool OpenXRIntegration::create_swapchains() {
#ifdef SUPPORT_OPENXR
    // Create swapchains for each view
    for (uint32_t i = 0; i < view_count_; ++i) {
        XrSwapchainCreateInfo swapchain_info = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        swapchain_info.arraySize = 1;
        swapchain_info.mipCount = 1;
        swapchain_info.faceCount = 1;
        swapchain_info.format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchain_info.width = view_configs_[i].recommendedImageRectWidth;
        swapchain_info.height = view_configs_[i].recommendedImageRectHeight;
        swapchain_info.sampleCount = view_configs_[i].recommendedSwapchainSampleCount;
        swapchain_info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
                                    XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

        XrSwapchain swapchain = XR_NULL_HANDLE;
        XrResult result = xrCreateSwapchain(session_, &swapchain_info, &swapchain);
        if (XR_FAILED(result)) {
            std::cerr << "Failed to create swapchain: " << result << std::endl;
            return false;
        }

        if (i == 0) left_swapchain_ = swapchain;
        else right_swapchain_ = swapchain;
    }

    return true;
#else
    return false;
#endif
}

bool OpenXRIntegration::submit_eye_swapchains(uint64_t left_texture,
                                               uint64_t right_texture) {
#ifdef SUPPORT_OPENXR
    if (!session_running_) return false;

    XrFrameWaitInfo wait_info = {XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frame_state = {XR_TYPE_FRAME_STATE};
    xrWaitFrame(session_, &wait_info, &frame_state);

    XrFrameBeginInfo begin_info = {XR_TYPE_FRAME_BEGIN_INFO};
    xrBeginFrame(session_, &begin_info);

    XrViewState view_state = {XR_TYPE_VIEW_STATE};
    XrView views[2] = {{XR_TYPE_VIEW}, {XR_TYPE_VIEW}};

    XrViewLocateInfo locate_info = {XR_TYPE_VIEW_LOCATE_INFO};
    locate_info.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    locate_info.space = local_space_;

    uint32_t view_count = 0;
    xrLocateViews(session_, &locate_info, &view_state, 2, &view_count, views);

    // Submit layers
    XrCompositionLayerProjectionView layer_views[2] = {};
    XrSwapchainImageBaseHeader* swapchain_images[2] = {};

    if (left_texture) {
        layer_views[0].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        layer_views[0].pose = views[0].pose;
        layer_views[0].fov = views[0].fov;
        layer_views[0].subImage.swapchain = left_swapchain_;
        layer_views[0].subImage.imageRect = {
            {0, 0},
            {view_configs_[0].recommendedImageRectWidth,
             view_configs_[0].recommendedImageRectHeight}
        };
    }

    if (right_texture) {
        layer_views[1].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        layer_views[1].pose = views[1].pose;
        layer_views[1].fov = views[1].fov;
        layer_views[1].subImage.swapchain = right_swapchain_;
        layer_views[1].subImage.imageRect = {
            {0, 0},
            {view_configs_[1].recommendedImageRectWidth,
             view_configs_[1].recommendedImageRectHeight}
        };
    }

    XrCompositionLayerProjection layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    layer.space = local_space_;
    layer.viewCount = 2;
    layer.views = layer_views;

    XrFrameEndInfo end_info = {XR_TYPE_FRAME_END_INFO};
    end_info.displayTime = frame_state.predictedDisplayTime;
    end_info.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    end_info.layerCount = 1;
    end_info.layers = reinterpret_cast<const XrCompositionLayerBaseHeader* const*>(&layer);

    xrEndFrame(session_, &end_info);

    return true;
#else
    return false;
#endif
}

bool OpenXRIntegration::poll_events() {
#ifdef SUPPORT_OPENXR
    XrEventDataBuffer event = {XR_TYPE_EVENT_DATA_BUFFER};

    while (xrPollEvent(instance_, &event) == XR_SUCCESS) {
        switch (event.type) {
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
            auto* state = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
            if (state->state == XR_SESSION_STATE_READY ||
                state->state == XR_SESSION_STATE_SYNCHRONIZED ||
                state->state == XR_SESSION_STATE_VISIBLE ||
                state->state == XR_SESSION_STATE_FOCUSED) {
                if (!session_running_) {
                    XrSessionBeginInfo begin_info = {XR_TYPE_SESSION_BEGIN_INFO};
                    begin_info.primaryViewConfigurationType =
                        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                    xrBeginSession(session_, &begin_info);
                    session_running_ = true;
                }
            } else if (state->state == XR_SESSION_STATE_STOPPING) {
                session_running_ = false;
            } else if (state->state == XR_SESSION_STATE_EXITING ||
                       state->state == XR_SESSION_STATE_LOSS_PENDING) {
                return false;
            }
            break;
        }
        default:
            break;
        }
    }

    return true;
#else
    return false;
#endif
}

} // namespace phonevr
