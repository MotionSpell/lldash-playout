#include "signals_unity_bridge.h"

#include <cstdio>
#include "lib_pipeline/pipeline.hpp"

// modules
#include "lib_media/demux/dash_demux.hpp"
#include "lib_media/demux/libav_demux.hpp"
#include "lib_media/in/mpeg_dash_input.hpp"
#include "lib_media/out/null.hpp"

[[noreturn]] void NotImplemented(const char* file, int line, const char* func)
{
  fprintf(stderr, "Not implemented: %s (%s:%d)\n", func, file, line);
  exit(1);
}

#define NOT_IMPLEMENTED \
  NotImplemented(__FILE__, __LINE__, __func__)

using namespace Modules;
using namespace Pipelines;
using namespace std;

static
bool startsWith(string s, string prefix)
{
  return s.substr(0, prefix.size()) == prefix;
}

void sub_stop(void* handle)
{
  try
  {
    unique_ptr<Pipeline> pipeline(static_cast<Pipeline*>(handle));

    if(!pipeline)
      throw runtime_error("invalid handle");

    pipeline->waitForEndOfStream();
  }
  catch(runtime_error const& err)
  {
    fprintf(stderr, "cannot stop: %s\n", err.what());
  }
}

void UnitySetGraphicsDevice(void* device, int deviceType, int eventType)
{
  (void)device;
  (void)deviceType;
  (void)eventType;
  NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////////////////////////////
// Pipeline
///////////////////////////////////////////////////////////////////////////////

struct GUBPipeline
{
  std::unique_ptr<Pipeline> pipe;
};

// creates a new pipeline.
// The returned pipeline must be freed using 'gub_pipeline_destroy'.
GUBPipeline* gub_pipeline_create(const char* name, GUBPipelineOnEosPFN eos_handler, GUBPipelineOnErrorPFN error_handler, GUBPipelineOnQosPFN qos_handler, void* userData)
{
  (void)name;
  (void)eos_handler;
  (void)error_handler;
  (void)qos_handler;
  (void)userData;

  try
  {
    return new GUBPipeline;
  }
  catch(exception const& err)
  {
    fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
    return nullptr;
  }
}

// Closes and frees a pipeline.
void gub_pipeline_destroy(GUBPipeline* pipeline)
{
  try
  {
    delete pipeline;
  }
  catch(exception const& err)
  {
    fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
  }
}

// Closes a pipeline. This frees all the resources associated with the pipeline, but not the pipeline object itself.
// The pipeline is left in an invalid state, where the only valid operation is 'gub_pipeline_destroy'.
void gub_pipeline_close(GUBPipeline* pipeline)
{
  (void)pipeline;
  NOT_IMPLEMENTED;
}

// Creates a pipeline that decodes 'uri'.
void gub_pipeline_setup_decoding(GUBPipeline* p, GUBPipelineVars* pipeVars)
{
  try
  {
    p->pipe = make_unique<Pipeline>();

    auto& pipe = *p->pipe;

    auto createDemuxer = [&] (string url) {
        if(startsWith(url, "http://"))
        {
          DashDemuxConfig cfg;
          cfg.url = url;
          return pipe.add("DashDemuxer", &cfg);
        }
        else
        {
          DemuxConfig cfg;
          cfg.url = url;
          return pipe.add("LibavDemux", &cfg);
        }
      };

    auto demuxer = createDemuxer(pipeVars->uri);

    for(int k = 0; k < (int)demuxer->getNumOutputs(); ++k)
    {
      auto metadata = demuxer->getOutputMetadata(k);

      if(!metadata || metadata->isSubtitle() /*only render audio and video*/)
      {
        g_Log->log(Debug, format("Ignoring stream #%s", k).c_str());
        continue;
      }

      auto decode = pipe.add("Decoder", (void*)(uintptr_t)metadata->type);
      pipe.connect(GetOutputPin(demuxer, k), decode);

      auto render = pipe.addModule<Out::Null>();
      pipe.connect(decode, render);
    }
  }
  catch(exception const& err)
  {
    fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
  }
}

void gub_pipeline_setup_decoding_clock(GUBPipeline* pipeline, const char* uri, int video_index, int audio_index, const char* net_clock_addr, int net_clock_port, uint64_t basetime, float crop_left, float crop_top, float crop_right, float crop_bottom, bool isDvbWc)
{
  (void)pipeline;
  (void)uri;
  (void)video_index;
  (void)audio_index;
  (void)net_clock_addr;
  (void)net_clock_port;
  (void)basetime;
  (void)crop_left;
  (void)crop_top;
  (void)crop_right;
  (void)crop_bottom;
  (void)isDvbWc;
  NOT_IMPLEMENTED;
}

void gub_pipeline_play(GUBPipeline* pipeline)
{
  pipeline->pipe->start();
}

void gub_pipeline_pause(GUBPipeline* pipeline)
{
  (void)pipeline;
  NOT_IMPLEMENTED;
}

void gub_pipeline_stop(GUBPipeline* pipeline)
{
  (void)pipeline;
  NOT_IMPLEMENTED;
}

// injects an 'end-of-stream' at source level
void gub_pipeline_stop_encoding(GUBPipeline* pipeline)
{
  (void)pipeline;
  NOT_IMPLEMENTED;
}

void gub_pipeline_set_adaptive_bitrate_limit(GUBPipeline* pipeline, float bitrate_limit)
{
  (void)pipeline;
  (void)bitrate_limit;
  NOT_IMPLEMENTED;
}

void gub_pipeline_set_basetime(GUBPipeline* pipeline, uint64_t basetime)
{
  (void)pipeline;
  (void)basetime;
  NOT_IMPLEMENTED;
}

void gub_pipeline_set_position(GUBPipeline* pipeline, double position)
{
  (void)pipeline;
  (void)position;
  NOT_IMPLEMENTED;
}

void gub_pipeline_set_volume(GUBPipeline* pipeline, double volume)
{
  (void)pipeline;
  (void)volume;
  NOT_IMPLEMENTED;
}

// increase the plugin user count (used for plugin shared-resources initialization)
static std::atomic<int> g_refCount;

void gub_ref(const char* gst_debug_string)
{
  (void)gst_debug_string;
  ++g_refCount;
}

void gub_unref()
{
  --g_refCount;
}

// returns true if the plugin user count is non-zero
int32_t gub_is_active()
{
  return g_refCount > 0;
}

// update the "last_frame" buffer with the last decoded frame, and retrieves its size.
int32_t gub_pipeline_grab_frame(GUBPipeline* pipeline, int* width, int* height)
{
  (void)pipeline;
  (void)width;
  (void)height;
  NOT_IMPLEMENTED;
}

int32_t gub_pipeline_grab_frame_with_info(GUBPipeline* pipeline, GUBPFrameInfo* info)
{
  (void)pipeline;
  (void)info;
  NOT_IMPLEMENTED;
}

double gub_pipeline_get_framerate(GUBPipeline* pipeline)
{
  (void)pipeline;
  NOT_IMPLEMENTED;
}

// blit the "last_frame" buffer to the native texture handle
void gub_pipeline_blit_image(GUBPipeline* pipeline, void* _TextureNativePtr)
{
  (void)_TextureNativePtr;
  (void)pipeline;
  NOT_IMPLEMENTED;
}

void gub_pipeline_blit_audio(GUBPipeline* pipeline, void* _AudioNativePtr)
{
  (void)_AudioNativePtr;
  (void)pipeline;
  NOT_IMPLEMENTED;
}

double gub_pipeline_get_duration(GUBPipeline* pipeline)
{
  (void)pipeline;
  NOT_IMPLEMENTED;
}

double gub_pipeline_get_position(GUBPipeline* pipeline)
{
  (void)pipeline;
  NOT_IMPLEMENTED;
}

int32_t gub_pipeline_is_loaded(GUBPipeline* pipeline)
{
  (void)pipeline;
  NOT_IMPLEMENTED;
}

int32_t gub_pipeline_is_playing(GUBPipeline* pipeline)
{
  (void)pipeline;
  NOT_IMPLEMENTED;
}

void gub_pipeline_set_camera_rotation(GUBPipeline* pipeline, double pitch, double roll, double yaw)
{
  (void)pipeline;
  (void)pitch;
  (void)roll;
  (void)yaw;
  NOT_IMPLEMENTED;
}

void gub_pipeline_set_camera_rotation_offset(GUBPipeline* pipeline, double pitch, double roll, double yaw)
{
  (void)pipeline;
  (void)pitch;
  (void)roll;
  (void)yaw;
  NOT_IMPLEMENTED;
}

void gub_pipeline_qualities(GUBPipeline* pipeline, const char* w, const char* h, const char* qualities)
{
  (void)pipeline;
  (void)w;
  (void)h;
  (void)qualities;
  NOT_IMPLEMENTED;
}

size_t gub_pipeline_pop_dash_report(GUBPipeline* pipeline, char* out_chunk_address, size_t max_size)
{
  (void)pipeline;
  (void)out_chunk_address;
  (void)max_size;
  NOT_IMPLEMENTED;
}

void gub_pipeline_set_dot_data_handler(GUBPipeline* pipeline, GUBPipelineDotDataHandlerPFN pfn)
{
  (void)pipeline;
  (void)pfn;
  NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////////////////////////////
// Logging
///////////////////////////////////////////////////////////////////////////////

// redirects the plugin logs to the provided callback 'pfn'
void gub_log_set_unity_handler(GUBUnityDebugLogPFN pfn)
{
  (void)pfn;
  NOT_IMPLEMENTED;
}

// Forwarding messages to external logger
void gub_log_set_external_handler(GUBExternalLogPFN pfn)
{
  (void)pfn;
  NOT_IMPLEMENTED;
}

void gub_log_set_console(int set)
{
  (void)set;
  NOT_IMPLEMENTED;
}

void gub_log_set_file(const char* file)
{
  (void)file;
  NOT_IMPLEMENTED;
}

void gub_log_init(const char* gst_debug_string)
{
  (void)gst_debug_string;
  NOT_IMPLEMENTED;
}

void gub_log_set_cache(const char* dir)
{
  (void)dir;
  NOT_IMPLEMENTED;
}

void gub_log_set_cache_clock(const char* net_clock_address, int net_clock_port)
{
  (void)net_clock_address;
  (void)net_clock_port;
  NOT_IMPLEMENTED;
}

int gub_log_switch_cache(char** output)
{
  (void)output;
  NOT_IMPLEMENTED;
}

