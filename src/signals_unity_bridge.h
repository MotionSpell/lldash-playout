///////////////////////////////////////////////////////////////////////////////
// SUB binary API
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <cstddef> // size_t

#define EXPORT __attribute__((visibility("default")))

extern "C" {
// opaque handle to a signals pipeline
struct sub_handle;

struct sub_video_info
{
  int width, height;
};

// If exported by a plugin, this function will be called when graphics device is created, destroyed,
// and before and after it is reset (ie, resolution changed).
EXPORT void UnitySetGraphicsDevice(void* device, int deviceType, int eventType);

// Creates a new pipeline.
// The returned pipeline must be freed using 'sub_destroy'.
EXPORT sub_handle* sub_create();

// Destroys a pipeline. This frees all the resources.
EXPORT void sub_destroy(sub_handle* h);

// Plays a given URL.
EXPORT bool sub_play(sub_handle* h, const char* URL);

// Get info about the last decoded video frame.
EXPORT void sub_get_video_info(sub_handle* h, sub_video_info* info);

// Copy the last decoded video frame to a native texture.
EXPORT void sub_copy_video(sub_handle* h, void* dstTextureNativeHandle);

// Copy the last decoded audio frame to a native texture.
// Returns: the size of audio data actually copied.
// Might be less than 'dstLen' if no enought audio was available.
EXPORT size_t sub_copy_audio(sub_handle* h, uint8_t* dst, size_t dstLen);
}

