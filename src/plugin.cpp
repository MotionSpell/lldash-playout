#define BUILDING_DLL
#include "lldash_play.h"

#include <cstdio>
#include <atomic>
#include <mutex>
#include <queue>
#include <cstring> // memcpy

#include "lib_pipeline/pipeline.hpp"
#include "lib_utils/format.hpp"
#include "lib_utils/log.hpp"

// modules
#include "lib_media/common/attributes.hpp"
#include "lib_media/common/metadata.hpp" // MetadataPkt
#include "lib_media/demux/dash_demux.hpp"
#include "lib_media/demux/gpac_demux_mp4_simple.hpp"
#include "lib_media/demux/libav_demux.hpp"
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
  OutStub(KHost*, function<void(Data)> onFrame_) : onFrame(onFrame_) {}

  void processOne(Data data) override { onFrame(data); }

  function<void(Data)> const onFrame;
};

struct Logger : LogSink
{
  void send(Level level, const char* msg) override
  {
    if(level > maxLevel)
      return;

    if (onError) {
      onError(format("[lldplay::%s] %s\n", name.c_str(), msg).c_str(), (int)level);
    } else {
      fprintf(stderr, "[lldplay::%s] %s\n", name.c_str(), msg);
      fflush(stderr);
    }
  }

  Level maxLevel = Level::Info;
  string name;
  std::function<void(const char*, int level)> onError = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
// API
///////////////////////////////////////////////////////////////////////////////

struct lldplay_handle
{
  lldplay_handle()
  {
    dropEverything = false;
  }

  ~lldplay_handle()
  {
    // prevent queuing further data buffers
    dropEverything = true;

    // release all data buffers (= unblock potential calls to 'alloc' inside the pipeline)
    {
      unique_lock<mutex> lock(transferMutex);

      for(auto& s : streams)
        while(!s.fifo.empty())
          s.fifo.pop();
    }

    // destroy the pipeline
    pipe.reset();
  }

  void adaptationControlCbk(IAdaptationControl* i)
  {
    adaptationControl = i;
  }

  IAdaptationControl* adaptationControl = nullptr;

  Logger logger;

  struct Stream
  {
    queue<Data> fifo;
    string fourcc;
  };

  std::function<bool(const char*)> errorCbk;
  atomic<bool> dropEverything;
  mutex transferMutex; // protects below members
  vector<Stream> streams;
  unique_ptr<Pipeline> pipe;
};

lldplay_handle* lldplay_create(const char* name, LLDashPlayoutMessageCallback onError, int maxLevel, uint64_t api_version)
{
  try
  {
    if(api_version != LLDASH_PLAYOUT_API_VERSION)
      throw runtime_error(format("Inconsistent API version between compilation (%s) and runtime (%s). Aborting.", LLDASH_PLAYOUT_API_VERSION, api_version).c_str());

    if(!name)
      name = "UnnamedPipeline";

    auto h = make_unique<lldplay_handle>();
    h->logger.name = name;
    h->logger.maxLevel = (Level)maxLevel;
    h->logger.setLevel((Level)maxLevel);
    h->logger.onError = onError;
    h->errorCbk = [onError](const char *msg) {
      if (onError) onError(msg, Level::Error);
      return true;
    };
    setGlobalLogger(h->logger);

    return h.release();
  }
  catch(exception const& err)
  {
    fprintf(stderr, "[%s] exception caught: %s\n", __func__, err.what());
    fflush(stderr);
    if (onError) {
      char errbuf[128];
      snprintf(errbuf, sizeof(errbuf), "[%s] exception caught: %s\n", __func__, err.what());
      onError(errbuf, Level::Error);
    }
    return nullptr;
  }
}

void lldplay_destroy(lldplay_handle* h)
{
  try
  {
    for (int i=0; i<lldplay_get_stream_count(h); ++i)
      lldplay_disable_stream(h, i);
    delete h;
  }
  catch(exception const& err)
  {
    fprintf(stderr, "[%s] exception caught: %s\n", __func__, err.what());
    fflush(stderr);
    if (h && h->errorCbk) {
      char errbuf[128];
      snprintf(errbuf, sizeof(errbuf), "[%s] exception caught: %s\n", __func__, err.what());
      h->errorCbk(errbuf);
    }
  }
}

int lldplay_get_stream_count(lldplay_handle* h)
{
  try
  {
    if(!h)
      throw runtime_error("handle can't be NULL");

    if(!h->pipe)
      throw runtime_error("Can only get stream count when the pipeline is playing");

    if(h->adaptationControl)
    {
      int num = 0;

      for(int as = 0; as < h->adaptationControl->getNumAdaptationSets(); ++as)
        num += h->adaptationControl->getNumRepresentationsInAdaptationSet(as);

      return num;
    }
    else
      return (int)h->streams.size();
  }
  catch(exception const& err)
  {
    h->logger.log(Level::Error, format("[%s] exception caught: %s\n", __func__, err.what()).c_str());
    return 0;
  }
}

static int get_stream_index(lldplay_handle* h, int i)
{
  if(h->adaptationControl)
  {
    for(int as = 0; as < h->adaptationControl->getNumAdaptationSets(); ++as)
      for(int rep = 0; rep < h->adaptationControl->getNumRepresentationsInAdaptationSet(as); --i, ++rep)
        if(i == 0)
          return as;
  }
  else
    return i;

  throw runtime_error("Invalid stream index.");
}

bool lldplay_get_stream_info(lldplay_handle* h, int streamIndex, struct StreamDesc* desc)
{
  try
  {
    if(!h)
      throw runtime_error("handle can't be NULL");

    if(!h->pipe)
      throw runtime_error("Can only get stream 4CC when the pipeline is playing");

    if(streamIndex < 0 || streamIndex >= lldplay_get_stream_count(h))
      throw runtime_error("Invalid streamIndex: must be positive and inferior to the number of streams");

    if(!desc)
      throw runtime_error("desc can't be NULL");

    auto const streamFirstIndex = get_stream_index(h, streamIndex);

    if(h->streams[streamIndex].fourcc.size() > 4) {
      h->logger.log(Level::Warning, format("[%s] 4CC \"%s\" will be truncated\n", __func__, h->streams[streamFirstIndex].fourcc.c_str()).c_str());

    }
    *desc = {};
    memcpy(&desc->MP4_4CC, h->streams[streamFirstIndex].fourcc.c_str(), 4);

    if(h->adaptationControl)
    {
      int i = 0, as = 0, rep = 0;

      for(as = 0; as < h->adaptationControl->getNumAdaptationSets(); ++as)
      {
        for(rep = 0; rep < h->adaptationControl->getNumRepresentationsInAdaptationSet(as); ++i, ++rep)
        {
          if(i == streamIndex)
          {
            auto srd = h->adaptationControl->getSRD(as);

            if(!srd.empty())
            {
              auto const parsed = sscanf(srd.c_str(), "0,%u,%u,%u,%u,%u,%u",
                                         &desc->objectX, &desc->objectY, &desc->objectWidth, &desc->objectHeight, &desc->totalWidth, &desc->totalHeight);

              if(parsed != 6)
              {
                h->logger.log(Level::Error, format("[%s] Invalid SRD format: \"%s\"\n", __func__, srd.c_str()).c_str());
                return false;
              }
            }
            return true;
          }
        }
      }

      return false;
    }

    return true;
  }
  catch(exception const& err)
  {
    h->logger.log(Level::Error, format("[%s] exception caught: %s\n", __func__, err.what()).c_str());
    return false;
  }
}

bool lldplay_play(lldplay_handle* h, const char* url)
{
  try
  {
    if(!h)
      throw runtime_error("handle can't be NULL");

    if(!url)
      throw runtime_error("URL can't be NULL");

    h->pipe = make_unique<Pipeline>(&h->logger);

    auto& pipe = *h->pipe;

    pipe.registerErrorCallback(h->errorCbk);

    auto addStream = [&] (OutputPin p)
      {
        auto const idx = (int)h->streams.size();
        h->streams.push_back({});
        auto meta = dynamic_pointer_cast<const MetadataPkt>(p.mod->getOutputMetadata(p.index));

        if(meta)
          h->streams[idx].fourcc = meta->codec;

        auto onFrame = [idx, h] (Data data)
          {
            if(h->dropEverything)
              return;

            if(isDeclaration(data))
              return;

            unique_lock<mutex> lock(h->transferMutex);
            h->streams[idx].fifo.push(data);
          };

        auto name = string("stream #") + to_string(idx);
        auto render = pipe.addNamedModule<OutStub>(name.c_str(), onFrame);
        pipe.connect(p, render);

        fprintf(stderr, "Added: %s\n", name.c_str());
      };

    if(startsWith(url, "http://") || startsWith(url, "https://"))
    {
      DashDemuxConfig cfg;
      cfg.url = url;
      cfg.adaptationControlCbk = bind(&lldplay_handle::adaptationControlCbk, h, placeholders::_1);
      auto demux = pipe.add("DashDemuxer", &cfg);

      for(int k = 0; k < demux->getNumOutputs(); ++k)
        addStream(GetOutputPin(demux, k));
    }
    else if(startsWith(url, "rtmp://"))
    {
      DemuxConfig cfg;
      cfg.url = url;
      auto demux = pipe.add("LibavDemux", &cfg);

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
    h->logger.log(Level::Error, format("[%s] exception caught: %s\n", __func__, err.what()).c_str());
    return false;
  }
}

bool lldplay_enable_stream(lldplay_handle* h, int tileNumber, int quality)
{
  try
  {
    if(!h)
      throw runtime_error("handle can't be NULL");

    h->adaptationControl->enableStream(tileNumber, quality);

    return true;
  }
  catch(exception const& err)
  {
    h->logger.log(Level::Error, format("[%s] exception caught: %s\n", __func__, err.what()).c_str());
    return false;
  }
}

bool lldplay_disable_stream(lldplay_handle* h, int tileNumber)
{
  try
  {
    if(!h)
      throw runtime_error("handle can't be NULL");

    h->adaptationControl->disableStream(tileNumber);

    return true;
  }
  catch(exception const& err)
  {
    h->logger.log(Level::Error, format("[%s] exception caught:failure %s\n", __func__, err.what()).c_str());
    return false;
  }
}

size_t lldplay_grab_frame(lldplay_handle* h, int i, uint8_t* dst, size_t dstLen, FrameInfo* info)
{
  try
  {
    if(!h)
      throw runtime_error("handle can't be NULL");

    unique_lock<mutex> lock(h->transferMutex);

    if(i < 0 || i >= lldplay_get_stream_count(h))
      throw runtime_error("Invalid stream index");

    auto const streamIndex = get_stream_index(h, i);
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
      info->timestamp = s->get<PresentationTime>().time / (IClock::Rate / 1000LL);

      auto meta = dynamic_pointer_cast<const MetadataPkt>(s->getMetadata());

      if(meta)
      {
        auto dsi = meta->codecSpecificInfo;

        if(dsi.size() > sizeof(info->dsi))
          throw runtime_error("DSI buffer too small");

        memcpy(info->dsi, dsi.data(), dsi.size());
        info->dsi_size = dsi.size();
      }
    }

    return N;
  }
  catch(exception const& err)
  {
    h->logger.log(Level::Error, format("[%s] exception caught: %s\n", __func__, err.what()).c_str());
    return 0;
  }
}

const char *lldplay_get_version() {
#ifdef LLDASH_VERSION
#define LLDASH_VERSION_STRINGIFY2(x) LLDASH_VERSION_STRINGIFY(x)
#define LLDASH_VERSION_STRINGIFY(x) #x
  return LLDASH_VERSION_STRINGIFY2(LLDASH_VERSION);
#else
	return "unknown";
#endif
}

