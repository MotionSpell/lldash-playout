#include <cstdio>
#include <cassert>
#include <thread>

#include "lldash_play.h"

using namespace std;

int main(int argc, char const* argv[])
{
  if(argc != 2)
  {
    fprintf(stderr, "Usage: %s [media url]\n", argv[0]);
    return 1;
  }

  auto handle = sub_create("LatencyPipeline");
  sub_play(handle, argv[1]);
  assert(sub_get_stream_count(handle) == 1);

  int frameCount = 0;

  for(int i = 0;; ++i)
  {
    FrameInfo info {};
    int64_t frame {};
    auto size = sub_grab_frame(handle, 0, (uint8_t*)&frame, sizeof frame, &info);

    if(size == 0)
    {
      this_thread::sleep_for(chrono::milliseconds(10));
      continue;
    }

    assert(size == 8);
    printf("Frame %04d: t=%.3f [%.16llx]\n", frameCount, info.timestamp / 1000.0, frame);
    ++frameCount;
  }

  sub_destroy(handle);

  return 0;
}

