// Non-interactive loader for signals-unity-bridge.so.
// Used for automated testing.
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <vector>
#include "dynlib.h"

// API entry points. We don't call these directly,
// as the implemtations live inside the dynamic library.
#include "signals_unity_bridge.h"

using namespace std;

#define IMPORT(name) ((decltype(name)*)lib->getSymbol(# name))

void safeMain(int argc, char* argv[])
{
  if(argc != 3)
    throw runtime_error("Usage: loader.exe <my_library> <my_url>");

  auto libName = argv[1];
  auto url = argv[2];

  auto lib = loadLibrary(libName);
  auto func_sub_create = IMPORT(sub_create);
  auto func_sub_destroy = IMPORT(sub_destroy);
  auto func_sub_play = IMPORT(sub_play);
  auto func_sub_get_stream_count = IMPORT(sub_get_stream_count);
  auto func_sub_grab_frame = IMPORT(sub_grab_frame);

  auto pipeline = func_sub_create(nullptr);
  auto ret = func_sub_play(pipeline, url);
  assert(ret);

  printf("%d stream(s)\n", func_sub_get_stream_count(pipeline));

  std::vector<uint8_t> buffer(100 * 1024 * 1024);

  int64_t cur = 0, prev = 0;
  for(int i = 0; i < 100000; ++i)
  {
    FrameInfo info {};
	if (!func_sub_grab_frame(pipeline, 0, buffer.data(), buffer.size(), &info)) {
		std::this_thread::sleep_for(1ms);
		continue;
	}
	memcpy(&cur, buffer.data(), sizeof(int64_t));
	int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    //printf("\tRomain: %lldms (diff=%lld, now=%lld, diffPrev=%lld)\n", info.timestamp, cur-now, now, cur-prev);
	prev = cur;
    //std::this_thread::sleep_for(1ms);
  }

  func_sub_destroy(pipeline);
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

