#include <cstdio>
#include <vector>
#include "lldash_play.h"

#if _WIN32
#include <windows.h>
void sleep(int ms) { ::Sleep(ms); }

#else
#include <unistd.h>
void sleep(int ms) { usleep(ms * 1000); }

#endif

int main(int argc, char const* argv[])
{
  if(argc != 2)
  {
    fprintf(stderr, "Usage: %s [media url]\n", argv[0]);
    return 1;
  }

  auto const mediaUrl = argv[1];

  auto handle = sub_create("MyMediaPipeline", nullptr, 3);

  sub_play(handle, mediaUrl);

  auto const streamCount = sub_get_stream_count(handle);

  if(streamCount == 0)
  {
    fprintf(stderr, "No streams found\n");
    return 1;
  }

  printf("%d stream(s):\n", sub_get_stream_count(handle));

  for(int i = 0; i < sub_get_stream_count(handle); ++i)
  {
    StreamDesc desc = {};
    sub_get_stream_info(handle, i, &desc);
    auto fourcc = (char*)&desc.MP4_4CC;
    printf("\tstream[%d]: %c%c%c%c\n", i, fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
  }

  std::vector<uint8_t> buffer;

  for(int i = 0;; ++i)
  {
    bool gotFrame = false;

    for(int k = 0; k < streamCount; ++k)
    {
      FrameInfo info {};
      buffer.resize(1024 * 1024 * 10);
      auto size = sub_grab_frame(handle, k, buffer.data(), buffer.size(), &info);
      buffer.resize(size);

      if(size > 0)
      {
        gotFrame = true;
        printf("[stream %d] Frame %d: % 5d bytes, t=%.3f\n", k, i, (int)size, info.timestamp / 1000.0);
      }
    }

    if(!gotFrame)
    {
      printf(".");
      fflush(stdout);
      sleep(100);
    }
  }

  sub_destroy(handle);

  return 0;
}
