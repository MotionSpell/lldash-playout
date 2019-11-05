// Copyright (C) 2019 Motion Spell
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Interactive standalone application, for signals-unity-bridge.so testing.
// This shows how to use the signals-unity-bridge DLL.
//
// Usage example:
// $ g++ example.cpp signals-unity-bridge.so -o example.exe
// $ ./example.exe [media url]
//
// Don't introduce direct dependencies to signals here.
// Keep this program standalone, as it's meant to be distributed as example.
//
#include <cstdio>
#include <vector>
#include "signals_unity_bridge.h"

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

  auto handle = sub_create("MyMediaPipeline");

  sub_play(handle, mediaUrl);

  auto const streamCount = sub_get_stream_count(handle);

  if(streamCount == 0)
  {
    fprintf(stderr, "No streams found\n");
    return 1;
  }

  printf("%d stream(s):\n", sub_get_stream_count(handle));
  for (int i = 0; i < sub_get_stream_count(handle); ++i) {
    streamDesc desc = {};
    sub_get_stream_info(handle, i, &desc);
    auto fourcc = (char*)& desc.MP4_4CC;
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

