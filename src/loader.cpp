// Non-interactive loader for signals-unity-bridge.so.
// Used for automated testing.
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <vector>
#include "dynlib.h"

// API entry points. We don't call these directly,
// as the implemtations live inside the dynamic library.
#include "lldash_play.h"

using namespace std;

#define IMPORT(name) ((decltype(name)*)lib->getSymbol(# name))

void safeMain(int argc, char* argv[])
{
  if(argc != 3)
    throw runtime_error("Usage: loader.exe <my_library> <my_url>");

  auto libName = argv[1];
  auto url = argv[2];

  auto lib = loadLibrary(libName);
  auto func_lldplay_create = IMPORT(lldplay_create);
  auto func_lldplay_destroy = IMPORT(lldplay_destroy);
  auto func_lldplay_play = IMPORT(lldplay_play);
  auto func_lldplay_get_stream_count = IMPORT(lldplay_get_stream_count);
  auto func_lldplay_get_stream_info = IMPORT(lldplay_get_stream_info);
  auto func_lldplay_grab_frame = IMPORT(lldplay_grab_frame);

  auto pipeline = func_lldplay_create(nullptr,  [](const char* msg, int level) { fprintf(stderr, "Level %d message: %s\n", level, msg); }, 2, LLDASH_PLAYOUT_API_VERSION);
  auto ret = func_lldplay_play(pipeline, url);
  (void)ret;
  assert(ret);

  printf("%d stream(s)\n", func_lldplay_get_stream_count(pipeline));

  for(int j = 0; j < func_lldplay_get_stream_count(pipeline); ++j)
  {
    StreamDesc desc {};
    func_lldplay_get_stream_info(pipeline, j, &desc);
    auto fourCC = (char*)&desc.MP4_4CC;
    printf("\t stream %d: 4CC=%c%c%c%c, SRD { %u, %u, %u, %u, %u, %u }\n", j, fourCC[0], fourCC[1], fourCC[2], fourCC[3],
           desc.objectX, desc.objectY, desc.objectWidth, desc.objectHeight, desc.totalWidth, desc.totalHeight);
  }

  std::vector<uint8_t> buffer(10 * 1024 * 1024);

  for(int i = 0; i < 100; ++i)
  {
    for(int j = 0; j < func_lldplay_get_stream_count(pipeline); ++j)
    {
      FrameInfo info {};
      auto size = func_lldplay_grab_frame(pipeline, j, buffer.data(), buffer.size(), &info);

      if(!size)
        continue;

      // printf("[%d] %lf (size=%d)\n", j, info.timestamp / (double)1000, (int)size);
    }

    // std::this_thread::sleep_for(10ms);
  }

  func_lldplay_destroy(pipeline);
}

int main(int argc, char* argv[])
{
  try
  {
    safeMain(argc, argv);
    return 0;
  }
  catch(exception const& e)
  {
    fprintf(stderr, "Fatal: %s\n", e.what());
    return 1;
  }
}
