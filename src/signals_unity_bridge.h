// Signals-unity-bridge (SUB) interface

#pragma once

#include <cstdint>

#define EXPORT __attribute__((visibility("default")))

extern "C" {

  // url: an http URL to a dash manifest (".mpd" file)
  // returns: an opaque player handle, or NULL on failure
	EXPORT void* sub_play(char const* url);

  // handle: a handle returned by 'sub_play'
	EXPORT void sub_stop(void* handle);
}


///////////////////////////////////////////////////////////////////////////////
// GUB interface
///////////////////////////////////////////////////////////////////////////////
extern "C" {

typedef struct GUBPipeline_ GUBPipeline;

struct GUBPipelineVars
{
	const char *uri;
	int video_index;
	int audio_index;
	char *net_clock_address;
	int net_clock_port;
	//TODO: basetime should be removed - use sync_time:
	// - name is misleading in gstreamer context
	// - value can be negative
	unsigned long long basetime;
	float crop_left;
	float crop_top;
	float crop_right;
	float crop_bottom;
	bool isDvbWc;
	int screenWidth;
	int screenHeight;
};

using GUBPipelineOnEosPFN = void(*)(GUBPipeline* userData);
using GUBPipelineOnErrorPFN = void(*)(GUBPipeline* userData, char* message);
using GUBPipelineOnQosPFN = void(*)(GUBPipeline* userData, int64_t current_jitter, uint64_t current_running_time, uint64_t current_stream_time, uint64_t current_timestamp, double proportion, uint64_t processed, uint64_t dropped);
using GUBUnityDebugLogPFN = void(*)(int32_t level, const char* message);

EXPORT void UnitySetGraphicsDevice(void* device, int deviceType, int eventType);

// creates a new pipeline.
// The returned pipeline must be freed using 'gub_pipeline_destroy'.
EXPORT GUBPipeline* gub_pipeline_create(const char* name,
    GUBPipelineOnEosPFN eos_handler,
    GUBPipelineOnErrorPFN error_handler,
    GUBPipelineOnQosPFN qos_handler,
    void* userData);

// Closes and frees a pipeline.
EXPORT void gub_pipeline_destroy(GUBPipeline* pipeline);

// Closes a pipeline. This frees all the resources associated with the pipeline, but not the pipeline object itself.
// The pipeline is left in an invalid state, where the only valid operation is 'gub_pipeline_destroy'.
EXPORT void gub_pipeline_close(GUBPipeline* pipeline);

// Creates a pipeline that decodes 'uri'.
EXPORT void gub_pipeline_setup_decoding(GUBPipeline* pipeline, GUBPipelineVars* pipeVars);
EXPORT void gub_pipeline_setup_decoding_clock(GUBPipeline* pipeline, const char* uri, int video_index, int audio_index, const char* net_clock_addr, int net_clock_port, uint64_t basetime, float crop_left, float crop_top, float crop_right, float crop_bottom, bool isDvbWc);

EXPORT void gub_pipeline_setup_encoding(GUBPipeline* pipeline, const char* filename, int width, int height);

// push a raw frame for encoding (the data is copied)
EXPORT void gub_pipeline_consume_image(GUBPipeline* pipeline, uint8_t* rawdata, int size);

EXPORT void gub_pipeline_play(GUBPipeline* pipeline);
EXPORT void gub_pipeline_pause(GUBPipeline* pipeline);
EXPORT void gub_pipeline_stop(GUBPipeline* pipeline);

// injects an 'end-of-stream' at source level
EXPORT void gub_pipeline_stop_encoding(GUBPipeline* pipeline);

EXPORT void gub_pipeline_set_adaptive_bitrate_limit(GUBPipeline* pipeline, float bitrate_limit);
EXPORT void gub_pipeline_set_basetime(GUBPipeline* pipeline, uint64_t basetime);
EXPORT void gub_pipeline_set_position(GUBPipeline* pipeline, double position);
EXPORT void gub_pipeline_set_volume(GUBPipeline* pipeline, double volume);

// increase the plugin user count (used for plugin shared-resources initialization)
EXPORT void gub_ref(const char* gst_debug_string);
EXPORT void gub_unref();

// returns true if the plugin user count is non-zero
EXPORT int32_t gub_is_active();

// update the "last_frame" buffer with the last decoded frame, and retrieves its size.
EXPORT int32_t gub_pipeline_grab_frame(GUBPipeline* pipeline, int* width, int* height);

// blit the "last_frame" buffer to the native texture handle
EXPORT void gub_pipeline_blit_image(GUBPipeline* pipeline, void* _TextureNativePtr);

EXPORT double gub_pipeline_get_duration(GUBPipeline* pipeline);
EXPORT double gub_pipeline_get_position(GUBPipeline* pipeline);
EXPORT int32_t gub_pipeline_is_loaded(GUBPipeline* pipeline);
EXPORT int32_t gub_pipeline_is_playing(GUBPipeline* pipeline);

// redirects the plugin logs to the provided callback 'pfn'
EXPORT void gub_log_set_unity_handler(GUBUnityDebugLogPFN pfn);

}
