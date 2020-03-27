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

const uint64_t SUB_API_VERSION = 0x20200327A;

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

struct StreamDesc
{
  uint32_t MP4_4CC;
  uint32_t tileNumber;
  uint32_t quality;
};

// Creates a new pipeline.
// name: a display name for log messages. Can be NULL.
// The returned pipeline must be freed using 'sub_destroy'.
SUB_EXPORT sub_handle* sub_create(const char* name, uint64_t api_version = SUB_API_VERSION);

// Destroys a pipeline. This frees all the resources.
SUB_EXPORT void sub_destroy(sub_handle* h);

// Plays a given URL.
SUB_EXPORT bool sub_play(sub_handle* h, const char* URL);

// Returns the number of compressed streams.
SUB_EXPORT int sub_get_stream_count(sub_handle* h);

// Returns the 4CC of a given stream. Desc is owned by the caller.
SUB_EXPORT bool sub_get_stream_info(sub_handle* h, int streamIndex, struct StreamDesc* desc);

// Enables a quality or disables a tile. There is at most one stream enabled per tile.
// Associations between streamIndex and tiles are given by sub_get_stream_info().
// Beware that disabling all qualities from all tiles will stop the session.
SUB_EXPORT bool sub_enable_stream(sub_handle* h, int tileNumber, int quality);
SUB_EXPORT bool sub_disable_stream(sub_handle* h, int tileNumber);

// Copy the next received compressed frame to a buffer.
// Returns: the size of compressed data actually copied,
// or zero, if no frame was available for this stream.
// If 'dst' is null, the frame will not be dequeued, but its size will be returned.
// Note that you shall dequeue all data from all streams to avoid being locked.
SUB_EXPORT size_t sub_grab_frame(sub_handle* h, int streamIndex, uint8_t* dst, size_t dstLen, FrameInfo* info);
}

