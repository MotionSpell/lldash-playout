///////////////////////////////////////////////////////////////////////////////
// GUB interface
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <cstddef> // size_t

#define EXPORT __attribute__((visibility("default")))

extern "C" {

  // url: an http URL to a dash manifest (".mpd" file)
  // returns: an opaque player handle, or NULL on failure
	EXPORT void* sub_play(char const* url);

  // handle: a handle returned by 'sub_play'
	EXPORT void sub_stop(void* handle);

struct GUBPipeline;

///////////////////////////////////////////////////////////////////////////////
// Graphics
///////////////////////////////////////////////////////////////////////////////

// If exported by a plugin, this function will be called when graphics device is created, destroyed,
// and before and after it is reset (ie, resolution changed).
EXPORT void UnitySetGraphicsDevice(void* device, int deviceType, int eventType);

///////////////////////////////////////////////////////////////////////////////
// Pipeline
///////////////////////////////////////////////////////////////////////////////

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
using GUBPipelineDotDataHandlerPFN = void(*)(const char* fileName, const char *data);

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

EXPORT void gub_pipeline_play(GUBPipeline* pipeline);
EXPORT void gub_pipeline_pause(GUBPipeline* pipeline);
EXPORT void gub_pipeline_stop(GUBPipeline* pipeline);

// injects an 'end-of-stream' at source level
EXPORT void gub_pipeline_stop_encoding(GUBPipeline* pipeline);

EXPORT void gub_pipeline_set_adaptive_bitrate_limit(GUBPipeline* pipeline, float bitrate_limit);
EXPORT void gub_pipeline_set_basetime(GUBPipeline* pipeline, uint64_t basetime);
EXPORT void gub_pipeline_set_position(GUBPipeline* pipeline, double position);
EXPORT void gub_pipeline_set_volume(GUBPipeline* pipeline, double volume);

EXPORT void gub_pipeline_set_camera_rotation(GUBPipeline *pipeline, double pitch, double roll, double yaw);
EXPORT void gub_pipeline_set_camera_rotation_offset(GUBPipeline *pipeline, double pitch, double roll, double yaw);
EXPORT void gub_pipeline_qualities(GUBPipeline *pipeline, const char *w, const char *h, const char *qualities);
EXPORT size_t gub_pipeline_pop_dash_report(GUBPipeline *pipeline, char* out_chunk_address, size_t max_size);
EXPORT void gub_pipeline_set_dot_data_handler(GUBPipeline *pipeline, GUBPipelineDotDataHandlerPFN pfn);

// increase the plugin user count (used for plugin shared-resources initialization)
EXPORT void gub_ref(const char* gst_debug_string);
EXPORT void gub_unref();

// returns true if the plugin user count is non-zero
EXPORT int32_t gub_is_active();

// update the "last_frame" buffer with the last decoded frame, and retrieves its size.
EXPORT int32_t gub_pipeline_grab_frame(GUBPipeline* pipeline, int* width, int* height);

EXPORT double gub_pipeline_get_framerate(GUBPipeline *pipeline);

// blit the "last_frame" buffer to the native texture handle
EXPORT void gub_pipeline_blit_image(GUBPipeline* pipeline, void* _TextureNativePtr);

EXPORT void gub_pipeline_blit_audio(GUBPipeline *pipeline, void *_AudioNativePtr);

EXPORT double gub_pipeline_get_duration(GUBPipeline* pipeline);
EXPORT double gub_pipeline_get_position(GUBPipeline* pipeline);
EXPORT int32_t gub_pipeline_is_loaded(GUBPipeline* pipeline);
EXPORT int32_t gub_pipeline_is_playing(GUBPipeline* pipeline);

///////////////////////////////////////////////////////////////////////////////
// Logging
///////////////////////////////////////////////////////////////////////////////

using GUBUnityDebugLogPFN = void(*)(int32_t level, const char* message);
using GUBExternalLogPFN = void(*)(const char* category, int32_t level, const char* file, const char* function, int line, const char* object, int64_t elapsed, const char* message);

// redirects the plugin logs to the provided callback 'pfn'
EXPORT void gub_log_set_unity_handler(GUBUnityDebugLogPFN pfn);

//Forwarding messages to external logger
EXPORT  int gub_log_switch_cache(char** output);
EXPORT void gub_log_set_external_handler(GUBExternalLogPFN pfn);
EXPORT void gub_log_set_console(int set);
EXPORT void gub_log_set_file(const char * file);
EXPORT void gub_log_set_cache(const char * dir);
EXPORT void gub_log_set_cache_clock(const char* net_clock_address, int net_clock_port);
EXPORT void gub_log_init(const char *gst_debug_string);

}
