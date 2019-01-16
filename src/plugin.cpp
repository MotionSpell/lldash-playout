#include "signals_unity_bridge.h"

#include <cstdio>
#include <cstring> // memcpy

#include "lib_pipeline/pipeline.hpp"

// modules
#include "lib_media/demux/dash_demux.hpp"
#include "lib_media/demux/libav_demux.hpp"
#include "lib_media/in/mpeg_dash_input.hpp"
#include "lib_media/in/video_generator.hpp"
#include "lib_media/out/null.hpp"

using namespace Modules;
using namespace Pipelines;
using namespace std;

///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////

[[noreturn]] void NotImplemented(const char* file, int line, const char* func)
{
  fprintf(stderr, "Not implemented: %s (%s:%d)\n", func, file, line);
  exit(1);
}

#define NOT_IMPLEMENTED \
  NotImplemented(__FILE__, __LINE__, __func__)

static
bool startsWith(string s, string prefix)
{
  return s.substr(0, prefix.size()) == prefix;
}

///////////////////////////////////////////////////////////////////////////////
// API
///////////////////////////////////////////////////////////////////////////////

void UnitySetGraphicsDevice(void* device, int deviceType, int eventType)
{
  (void)device;
  (void)eventType;

  if(deviceType == 0) // OpenGL
    return;

  if(deviceType == -1) // dummy (non-interactive, used for tests)
    return;

  fprintf(stderr, "Unsupported graphic device: %d\n", deviceType);
}

struct sub_handle
{
  std::unique_ptr<Pipeline> pipe;
};

sub_handle* sub_create()
{
  try
  {
    return new sub_handle;
  }
  catch(exception const& err)
  {
    fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
    return nullptr;
  }
}

void sub_destroy(sub_handle* h)
{
  try
  {
    delete h;
  }
  catch(exception const& err)
  {
    fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
  }
}

bool sub_play(sub_handle* h, const char* uri)
{
  try
  {
    h->pipe = make_unique<Pipeline>();

    auto& pipe = *h->pipe;

    auto createSource = [&] (string url) {
        if(startsWith(url, "http://"))
        {
          DashDemuxConfig cfg;
          cfg.url = url;
          return pipe.add("DashDemuxer", &cfg);
        }
        else if(startsWith(url, "videogen://"))
        {
          return pipe.addModule<In::VideoGenerator>();
        }
        else
        {
          DemuxConfig cfg;
          cfg.url = url;
          return pipe.add("LibavDemux", &cfg);
        }
      };

    auto source = createSource(uri);

    for(int k = 0; k < (int)source->getNumOutputs(); ++k)
    {
      auto flow = GetOutputPin(source, k);
      auto metadata = source->getOutputMetadata(k);

      if(!metadata || metadata->isSubtitle() /*only render audio and video*/)
      {
        g_Log->log(Debug, format("Ignoring stream #%s", k).c_str());
        continue;
      }

      if(metadata->type != VIDEO_RAW)
      {
        auto decode = pipe.add("Decoder", (void*)(uintptr_t)metadata->type);
        pipe.connect(flow, decode);
        flow = decode;
      }

      auto render = pipe.addModule<Out::Null>();
      pipe.connect(flow, render);
    }

    pipe.start();
    return true;
  }
  catch(exception const& err)
  {
    fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
    return false;
  }
}

void sub_get_video_info(sub_handle* h, sub_video_info* info)
{
  (void)h;
  *info = {};
  info->width = 128;
  info->height = 128;
}

#include "GL/gl.h"

void sub_copy_video(sub_handle* h, void* dstTextureNativeHandle)
{
  (void)h;

  sub_video_info info;
  sub_get_video_info(h, &info);

  std::vector<uint8_t> img(info.width* info.height * 4);

  for(int i = 0; i < (int)img.size(); ++i)
    img[i] = i * 7;

  glBindTexture(GL_TEXTURE_2D, (GLuint)(uintptr_t)dstTextureNativeHandle);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info.width, info.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data());
}

size_t sub_copy_audio(sub_handle* h, uint8_t* dst, size_t dstLen)
{
  (void)h;
  memset(dst, 0, dstLen);
  return 0;
}

