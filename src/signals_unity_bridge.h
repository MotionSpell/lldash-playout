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

struct FrameInfo
{
  // presentation timestamp, in milliseconds units.
  int64_t timestamp;

  // decoder specific info
  uint8_t dsi[256];
  int dsi_size;
};

extern "C" {
// opaque handle to a signals pipeline
struct sub_handle;

// Creates a new pipeline.
// name: a display name for log messages. Can be NULL.
// The returned pipeline must be freed using 'sub_destroy'.
SUB_EXPORT sub_handle* sub_create(const char* name);

// Destroys a pipeline. This frees all the resources.
SUB_EXPORT void sub_destroy(sub_handle* h);

// Returns the number of compressed streams.
SUB_EXPORT int sub_get_stream_count(sub_handle* h);

// Returns the 4CC of a given stream.
SUB_EXPORT uint32_t sub_get_stream_4cc(sub_handle* h, int streamIndex);

// Plays a given URL.
SUB_EXPORT bool sub_play(sub_handle* h, const char* URL);

// Copy the next received compressed frame to a buffer.
// Returns: the size of compressed data actually copied,
// or zero, if no frame was available for this stream.
// If 'dst' is null, the frame will not be dequeued, but its size will be returned.
// Note that you shall dequeue all data from all streams to avoid being locked.
SUB_EXPORT size_t sub_grab_frame(sub_handle* h, int streamIndex, uint8_t* dst, size_t dstLen, FrameInfo* info);
}

