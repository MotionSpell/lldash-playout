//
// Copyright (C) 2019 Motion Spell
//

#pragma once

#include <cstdint>
#include <cstddef> // size_t

#ifdef _WIN32
#ifdef BUILDING_DLL
#define SUB_EXPORT __declspec(dllexport)
#else
#define SUB_EXPORT __declspec(dllimport)
#endif
#else
#define SUB_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
// opaque handle to a signals pipeline
struct sub_handle;

// Creates a new pipeline.
// name: a display name for log messages. Can be NULL.
// The returned pipeline must be freed using 'sub_destroy'.
SUB_EXPORT sub_handle* sub_create(const char* name);

// Destroys a pipeline. This frees all the resources.
SUB_EXPORT void sub_destroy(sub_handle* h);

// Plays a given URL.
SUB_EXPORT bool sub_play(sub_handle* h, const char* URL);

// Copy the last decoded video frame to a native texture.
SUB_EXPORT void sub_copy_video(sub_handle* h, void* dstTextureNativeHandle);

// Copy the last decoded audio frame to a native texture.
// Returns: the size of audio data actually copied.
// Might be less than 'dstLen' if no enough audio was available.
// At the moment, the PCM format is fixed to:
// - 48kHz
// - 16 bit signed integer
// - interleaved stereo.
SUB_EXPORT size_t sub_copy_audio(sub_handle* h, uint8_t* dst, size_t dstLen);
}

