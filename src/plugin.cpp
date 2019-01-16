#include "signals_unity_bridge.h"

#include <cstdio>
#include <cstring> // memcpy

#include "lib_pipeline/pipeline.hpp"
#include "lib_utils/system_clock.hpp" // for regulation

// modules
#include "lib_media/demux/dash_demux.hpp"
#include "lib_media/demux/libav_demux.hpp"
#include "lib_media/in/mpeg_dash_input.hpp"
#include "lib_media/in/video_generator.hpp"
#include "lib_media/out/null.hpp"
#include "lib_media/utils/regulator.hpp"

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

struct OutStub : ModuleS
{
  OutStub(KHost*, std::function<void(Data)> onFrame_) : onFrame(onFrame_)
  {
    addInput(this);
  }

  void process(Data data) override
  {
    onFrame(data);
  }

  std::function<void(Data)> const onFrame;
};

struct Logger : LogSink
{
  void log(Level level, const char* msg) override
  {
    if(level == Level::Debug)
      return;

    fprintf(stderr, "[%s] %s\n", name.c_str(), msg);
    fflush(stderr);
  }

  std::string name;
};

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
  Logger logger;
  std::shared_ptr<const DataPicture> lastPic;
  std::unique_ptr<Pipeline> pipe;
};

sub_handle* sub_create(const char* name)
{
  try
  {
    if(!name)
      name = "UnnamedPipeline";

    auto h = make_unique<sub_handle>();
    h->logger.name = name;
    return h.release();
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
  auto onFrame = [h] (Data data)
    {
      h->lastPic = safe_cast<const DataPicture>(data);
    };

  try
  {
    h->pipe = make_unique<Pipeline>(&h->logger);

    auto& pipe = *h->pipe;

    auto regulate = [&] (OutputPin source) {
        auto regulator = pipe.addModule<Regulator>(g_SystemClock);
        pipe.connect(source, regulator);
        return regulator;
      };

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
        h->logger.log(Warning, format("Ignoring stream #%s", k).c_str());
        continue;
      }

      if(metadata->type != VIDEO_RAW)
      {
        auto decode = pipe.add("Decoder", (void*)(uintptr_t)metadata->type);
        pipe.connect(flow, decode);
        flow = decode;
      }

      flow = regulate(flow);

      auto render = pipe.addModule<OutStub>(onFrame);
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

#include "GL/gl.h"

void sub_copy_video(sub_handle* h, void* dstTextureNativeHandle)
{
  auto pic = h->lastPic;

  auto fmt = pic->getFormat();

  std::vector<uint8_t> img(fmt.res.width* fmt.res.height * 3);

  for(int row = 0; row < fmt.res.height; ++row)
  {
    for(int col = 0; col < fmt.res.width; ++col)
    {
      int val = pic->getPlane(0)[row * pic->getStride(0) + col];
      int offset = (fmt.res.height - 1 - row) * fmt.res.width + col;
      img[offset * 3 + 0] = 255;
      img[offset * 3 + 1] = val;
      img[offset * 3 + 2] = val;
    }
  }

  glBindTexture(GL_TEXTURE_2D, (GLuint)(uintptr_t)dstTextureNativeHandle);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fmt.res.width, fmt.res.height, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data());
}

size_t sub_copy_audio(sub_handle* h, uint8_t* dst, size_t dstLen)
{
  (void)h;
  memset(dst, 0, dstLen);
  return 0;
}

