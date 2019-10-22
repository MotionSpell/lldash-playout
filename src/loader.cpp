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

  std::vector<uint8_t> buffer(10 * 1024 * 1024);

  for (int i = 0; i < 100; ++i)
  {
	  for (int j = 0; j < func_sub_get_stream_count(pipeline); ++j) {
		  FrameInfo info{};
		  auto size = func_sub_grab_frame(pipeline, j, buffer.data(), buffer.size(), &info);
		  if (!size)
			  continue;

		  printf("[%d] %lf (size=%d)\n", j, info.timestamp / (double)1000, (int)size);
	  }
	  std::this_thread::sleep_for(10ms);
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

