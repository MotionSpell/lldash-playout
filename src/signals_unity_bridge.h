//
// Copyright (C) 2019 Motion Spell
//

#pragma once

#include <cstdint>
#include <cstddef> // size_t

#define EXPORT __attribute__((visibility("default")))

extern "C" {
// opaque handle to a signals pipeline
struct sub_handle;

// If exported by a plugin, this function will be called when graphics device is created, destroyed,
// and before and after it is reset (ie, resolution changed).
EXPORT void UnitySetGraphicsDevice(void* device, int deviceType, int eventType);

// Creates a new pipeline.
// name: a display name for log messages. Can be NULL.
// The returned pipeline must be freed using 'sub_destroy'.
EXPORT sub_handle* sub_create(const char* name);

// Destroys a pipeline. This frees all the resources.
EXPORT void sub_destroy(sub_handle* h);

// Plays a given URL.
EXPORT bool sub_play(sub_handle* h, const char* URL);

// Copy the last decoded video frame to a native texture.
EXPORT void sub_copy_video(sub_handle* h, void* dstTextureNativeHandle);

// Copy the last decoded audio frame to a native texture.
// Returns: the size of audio data actually copied.
// Might be less than 'dstLen' if no enough audio was available.
// At the moment, the PCM format is fixed to:
// - 48kHz
// - 16 bit signed integer
// - interleaved stereo.
EXPORT size_t sub_copy_audio(sub_handle* h, uint8_t* dst, size_t dstLen);
}

