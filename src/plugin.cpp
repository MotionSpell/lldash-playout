#define BUILDING_DLL
#include "signals_unity_bridge.h"

#include <cstdio>
#include <atomic>
#include <mutex>
#include <queue>
#include <cstring> // memcpy

#include "lib_pipeline/pipeline.hpp"

// modules
#include "lib_media/demux/dash_demux.hpp"
#include "lib_media/demux/gpac_demux_mp4_simple.hpp"
#include "lib_media/in/mpeg_dash_input.hpp"
#include "lib_media/out/null.hpp"

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
  OutStub(KHost*, std::function<void(Data)> onFrame_) : onFrame(onFrame_) {}

  void processOne(Data data) override { onFrame(data); }

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

struct sub_handle
{
  sub_handle()
  {
    dropEverything = false;
  }

  ~sub_handle()
  {
    // prevent queuing further data buffers
    dropEverything = true;

    // release all data buffers (= unblock potential calls to 'alloc' inside the pipeline)
    {
      std::unique_lock<std::mutex> lock(transferMutex);

      for(auto& s : streams)
        while(!s.fifo.empty())
          s.fifo.pop();
    }

    // destroy the pipeline
    pipe.reset();
  }

  Logger logger;

  struct Stream
  {
    std::queue<Data> fifo;
  };

  std::atomic<bool> dropEverything;
  std::mutex transferMutex; // protects below members
  std::vector<Stream> streams;
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
    fflush(stderr);
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
    fflush(stderr);
  }
}

int sub_get_stream_count(sub_handle* h)
{
  try
  {
    if(!h->pipe)
      throw runtime_error("Can only get stream count when the pipeline is playing");

    return (int)h->streams.size();
  }
  catch(exception const& err)
  {
    fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
    fflush(stderr);
    return 0;
  }
}

bool sub_play(sub_handle* h, const char* url)
{
  try
  {
    h->pipe = make_unique<Pipeline>(&h->logger);

    auto& pipe = *h->pipe;

    auto addStream = [&] (OutputPin p)
      {
        auto const idx = (int)h->streams.size();
        h->streams.push_back({});

        auto onFrame = [idx, h] (Data data)
          {
            if(h->dropEverything)
              return;

            if(isDeclaration(data))
              return;

            std::unique_lock<std::mutex> lock(h->transferMutex);
            h->streams[idx].fifo.push(data);
          };

        auto name = string("stream #") + to_string(idx);
        auto render = pipe.addNamedModule<OutStub>(name.c_str(), onFrame);
        pipe.connect(p, render);

        fprintf(stderr, "Added: %s\n", name.c_str());
      };

    if(startsWith(url, "http://"))
    {
      DashDemuxConfig cfg;
      cfg.url = url;
      auto demux = pipe.add("DashDemuxer", &cfg);

      for(int k = 0; k < demux->getNumOutputs(); ++k)
        addStream(GetOutputPin(demux, k));
    }
    else
    {
      Mp4DemuxConfig cfg;
      cfg.path = url;
      auto demux = pipe.add("GPACDemuxMP4Simple", &cfg);

      for(int k = 0; k < demux->getNumOutputs(); ++k)
        addStream(GetOutputPin(demux, k));
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

size_t sub_grab_frame(sub_handle* h, int streamIndex, uint8_t* dst, size_t dstLen, FrameInfo* info)
{
  try
  {
    std::unique_lock<std::mutex> lock(h->transferMutex);

    if(streamIndex < 0 || streamIndex >= (int)h->streams.size())
      throw runtime_error("Invalid stream index");

    auto& stream = h->streams[streamIndex];

    if(stream.fifo.empty())
      return 0;

    auto s = stream.fifo.front();
    auto const N = s->data().len;

    if(!dst)
      return N;

    stream.fifo.pop();

    if(N > dstLen)
      throw runtime_error("Buffer too small");

    memcpy(dst, s->data().ptr, N);

    if(info)
    {
      *info = {};
      info->timestamp = s->getMediaTime() / (IClock::Rate / 1000LL);
    }

    return N;
  }
  catch(exception const& err)
  {
    fprintf(stderr, "[%s] failure: %s\n", __func__, err.what());
    fflush(stderr);
    return 0;
  }
}

