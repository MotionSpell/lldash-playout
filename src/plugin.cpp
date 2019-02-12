#define BUILDING_DLL
#include "signals_unity_bridge.h"

#include <cstdio>
#include <mutex>
#include <cstring> // memcpy

#include "lib_pipeline/pipeline.hpp"
#include "lib_utils/system_clock.hpp" // for regulation

// modules
#include "lib_media/demux/dash_demux.hpp"
#include "lib_media/demux/libav_demux.hpp"
#include "lib_media/in/mpeg_dash_input.hpp"
#include "lib_media/in/video_generator.hpp"
#include "lib_media/in/sound_generator.hpp"
#include "lib_media/out/null.hpp"
#include "lib_media/transform/audio_convert.hpp"
#include "lib_media/utils/regulator.hpp"

using namespace Modules;
using namespace Pipelines;
using namespace std;

///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////

static
bool startsWith(string s, string prefix)
{
  return s.substr(0, prefix.size()) == prefix;
}

struct OutStub : ModuleS
{
  OutStub(KHost*, std::function<void(Data)> onFrame_) : onFrame(onFrame_)
  {
  }

  void processOne(Data data) override
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

// non-blocking, overwriting fifo
struct Fifo
{
  uint8_t data[1024 * 1024] {};
  int writePos = 0;
  int readPos = 0;

  void write(const uint8_t* buf, int len)
  {
    for(int i = 0; i < len; ++i)
    {
      data[writePos] = buf[i];
      writePos = (writePos + 1) % (sizeof data);
    }
  }

  void read(uint8_t* buf, int len)
  {
    for(int i = 0; i < len; ++i)
    {
      buf[i] = data[readPos];

      // don't go below the write position
      if(readPos != writePos)
        readPos = (readPos + 1) % (sizeof data);
    }
  }
};

struct sub_handle
{
  Logger logger;
  std::unique_ptr<Pipeline> pipe;

  std::mutex transferMutex; // protects both below members
  Fifo audioFifo;
  std::shared_ptr<const DataPicture> lastPic;
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
    fflush(stderr);
    return nullptr;
  }
}

void sub_destroy(sub_handle* h)
{
  try
  {
    {
      std::unique_lock<std::mutex> lock(h->transferMutex);
      h->lastPic = nullptr;
    }
    delete h;
  }
  catch(exception const& err)
  {
    fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
    fflush(stderr);
  }
}

// introduce a 3s latency
struct DelayedClock : IClock
{
  DelayedClock(IClock* outer_, int delayInSeconds_) : outer(outer_), delayInSeconds(delayInSeconds_) {}

  Fraction now() const override { return outer->now() - delayInSeconds; }

  IClock* const outer;
  const int delayInSeconds;
};

bool sub_play(sub_handle* h, const char* url)
{
  auto delayedClock = std::make_shared<DelayedClock>(g_SystemClock.get(), 0);

  auto onFrame = [h] (Data data)
    {
      std::unique_lock<std::mutex> lock(h->transferMutex);

      if(auto pic = dynamic_pointer_cast<const DataPicture>(data))
        h->lastPic = pic;
      else if(auto pcm = dynamic_pointer_cast<const DataPcm>(data))
        h->audioFifo.write(pcm->data().ptr, pcm->data().len);
    };

  try
  {
    h->pipe = make_unique<Pipeline>(&h->logger);

    auto& pipe = *h->pipe;

    auto regulate = [&] (OutputPin source)
      {
        static int i;
        auto metadata = source.mod->getOutputMetadata(source.index);
        auto name = format("Regulator[%s]", i++);
        auto regulator = pipe.addNamedModule<Regulator>(name.c_str(), delayedClock);
        pipe.connect(source, regulator);
        return regulator;
      };

    auto decode = [&] (OutputPin flow)
      {
        auto metadata = flow.mod->getOutputMetadata(flow.index);
        auto decode = pipe.add("Decoder", (void*)(uintptr_t)metadata->type);
        pipe.connect(flow, decode);
        return decode;
      };

    auto getFirstPin = [] (IFilter* source, int type)
      {
        for(int k = 0; k < (int)source->getNumOutputs(); ++k)
        {
          auto metadata = source->getOutputMetadata(k);

          if(metadata && metadata->type == type)
            return GetOutputPin(source, k);
        }

        return OutputPin(nullptr);
      };

    auto videoPin = OutputPin(nullptr);
    auto audioPin = OutputPin(nullptr);

    if(startsWith(url, "http://"))
    {
      DashDemuxConfig cfg;
      cfg.url = url;
      auto demux = pipe.add("DashDemuxer", &cfg);
      videoPin = getFirstPin(demux, VIDEO_PKT);
      audioPin = getFirstPin(demux, AUDIO_PKT);

      // introduce a 3s latency
      delayedClock = std::make_shared<DelayedClock>(g_SystemClock.get(), 3);
    }
    else if(startsWith(url, "videogen://"))
    {
      videoPin = pipe.addNamedModule<In::VideoGenerator>("VideoGenerator", url);
      audioPin = pipe.addNamedModule<In::SoundGenerator>("AudioGenerator");
    }
    else if(startsWith(url, "audiogen://"))
    {
      audioPin = pipe.addNamedModule<In::SoundGenerator>("AudioGenerator");
    }
    else
    {
      DemuxConfig cfg;
      cfg.url = url;
      auto demux = pipe.add("LibavDemux", &cfg);
      videoPin = getFirstPin(demux, VIDEO_PKT);
      audioPin = getFirstPin(demux, AUDIO_PKT);
    }

    if(videoPin.mod)
    {
      auto flow = videoPin;

      {
        auto metadata = flow.mod->getOutputMetadata(flow.index);

        if(metadata->type == VIDEO_PKT)
          flow = decode(flow);
      }

      {
        auto fmt = PictureFormat({ 720, 576 }, PixelFormat::RGB24);
        auto convert = pipe.add("VideoConvert", &fmt);
        pipe.connect(flow, convert);
        flow = convert;
      }

      flow = regulate(flow);

      auto render = pipe.addNamedModule<OutStub>("VideoSink", onFrame);
      pipe.connect(flow, render);
    }

    if(audioPin.mod)
    {
      auto flow = audioPin;

      {
        auto metadata = flow.mod->getOutputMetadata(flow.index);

        if(metadata->type == AUDIO_PKT)
          flow = decode(flow);
      }

      {
        AudioConvertConfig cfg {
          { 0 }, PcmFormat(48000, 2, Stereo, S16, Interleaved), 256
        };
        auto convert = pipe.add("AudioConvert", &cfg);
        pipe.connect(flow, convert);
        flow = convert;
      }

      flow = regulate(flow);

      auto render = pipe.addNamedModule<OutStub>("AudioSink", onFrame);
      pipe.connect(flow, render);
    }

    pipe.start();
    return true;
  }
  catch(exception const& err)
  {
    fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
    fflush(stderr);
    return false;
  }
}

#include "GL/gl.h"

static
void ensureGl(char const* expr, int line)
{
  auto const errorCode = glGetError();

  if(errorCode == GL_NO_ERROR)
    return;

  string msg;
  msg += "OpenGL error\n";
  msg += "Expr: " + string(expr) + "\n";
  msg += "Line: " + to_string(line) + "\n";
  msg += "Code: " + to_string(errorCode) + "\n";
  throw runtime_error(msg);
}

#define SAFE_GL(a) \
  do { a; ensureGl(# a, __LINE__); } while(0)

void sub_copy_video(sub_handle* h, void* dstTextureNativeHandle)
{
  try
  {
    std::unique_lock<std::mutex> lock(h->transferMutex);
    auto pic = h->lastPic;

    if(!pic)
      return;

    auto fmt = pic->getFormat();

    std::vector<uint8_t> img(fmt.res.width* fmt.res.height * 3);

    const uint8_t* src = pic->getPlane(0);
    uint8_t* dst = img.data();

    for(int row = 0; row < fmt.res.height; ++row)
    {
      memcpy(dst, src, 3 * fmt.res.width);
      src += pic->getStride(0);
      dst += 3 * fmt.res.width;
    }

    SAFE_GL(glBindTexture(GL_TEXTURE_2D, (GLuint)(uintptr_t)dstTextureNativeHandle));
    SAFE_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fmt.res.width, fmt.res.height, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data()));
  }
  catch(exception const& err)
  {
    fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
    fflush(stderr);
  }
}

size_t sub_copy_audio(sub_handle* h, uint8_t* dst, size_t dstLen)
{
  try
  {
    std::unique_lock<std::mutex> lock(h->transferMutex);
    h->audioFifo.read(dst, dstLen);
    return dstLen;
  }
  catch(exception const& err)
  {
    fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
    fflush(stderr);
    return 0;
  }
}

#include "IUnityInterface.h"
#include "IUnityGraphics.h"
#include <cassert>

// Unity plugin load event
extern "C"
{
SUB_EXPORT void UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
  IUnityGraphics* graphics = unityInterfaces->Get<IUnityGraphics>();
  assert(graphics);

  auto rendererType = graphics->GetRenderer();
  fprintf(stderr, "UnityPluginLoad: rendererType: %d\n", rendererType);
  fflush(stderr);
  switch(rendererType)
  {
  case kUnityGfxRendererOpenGLCore:
    // OK, Nothing to do
    break;

  // dummy (non-interactive, used for tests)
  case kUnityGfxRendererNull:
    return;

  default:
    fprintf(stderr, "ERROR: Unsupported renderer type: %d\n", rendererType);
    fprintf(stderr, "At the moment, only OpenGL core (17) is supported. Please configure Unity to 'OpenGL 4.5'.\n");
    fprintf(stderr, "Aborting program.\n");
    fflush(stderr);
    exit(1);
    break;
  }
}
}

