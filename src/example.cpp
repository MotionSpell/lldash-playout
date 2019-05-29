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
#include <functional>
#include "signals_unity_bridge.h"

#if _WIN32
#include <windows.h>
void sleep(int ms) { ::Sleep(ms); }

#else
#include <unistd.h>
void sleep(int ms) { usleep(ms * 1000); }

#endif

using namespace std;

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

  if(sub_get_stream_count(handle) == 0)
  {
    fprintf(stderr, "No streams found\n");
    return 1;
  }

  vector<uint8_t> buffer;

  for(int i = 0; i < 100; ++i)
  {
    FrameInfo info {};
    buffer.resize(1024 * 1024 * 10);
    auto size = sub_grab_frame(handle, 0, buffer.data(), buffer.size(), &info);
    buffer.resize(size);

    if(size == 0)
    {
      sleep(100);
      continue;
    }

    printf("Frame: % 5d bytes, t=%.3f [", (int)size, info.timestamp / 1000.0);

    for(int k = 0; k < (int)buffer.size(); ++k)
    {
      if(k == 8)
      {
        printf(" ...");
        break;
      }

      printf(" %.2X", buffer[k]);
    }

    printf(" ]\n");
  }

  sub_destroy(handle);
}

