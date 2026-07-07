#pragma once

#include <openvr_driver.h>

using namespace vr;

#if defined(_WIN32)
#define EXPORT_FUNC __declspec(dllexport)
#else
#define EXPORT_FUNC
#endif

extern "C" EXPORT_FUNC void* HmdDriverFactory(const char* interface_name, int* return_code);
