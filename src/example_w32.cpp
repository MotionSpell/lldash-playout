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
// $ g++ app.cpp `pkg-config sdl2 --cflags --libs` -o dissidence_player
// $ ./dissidence_player signals-unity-bridge.dll [media url]
//
// Don't introduce direct dependencies to signals here.
// Keep this program standalone, as it's meant to be distributed as example.
//
#include "signals_unity_bridge.h"

#include <cstdio>
#include <string>
#include <vector>
#include <stdexcept>

#include <windows.h>

using namespace std;

void* safeImport(HMODULE lib, const char* name)
{
  auto r = (void*)GetProcAddress(lib, name);

  if(!r)
  {
    char msg[256];
    sprintf(msg, "Symbol not found: '%s'", name);
    throw runtime_error(msg);
  }

  return r;
}

#define IMPORT(name) ((decltype(name)*)safeImport(lib, # name))

void safeMain(int argc, char const* argv[])
{
  if(argc != 3)
    throw runtime_error("Usage: app.exe <signals-unity-bridge.dll> [media url]");

  const auto libraryPath = argv[1];
  const auto mediaUrl = argv[2];

  auto lib = LoadLibrary(libraryPath);

  if(!lib)
  {
    char msg[256];
    sprintf(msg, "Can't load '%s': %d", libraryPath, GetLastError());
    throw runtime_error(msg);
  }

  auto func_sub_create = IMPORT(sub_create);
  auto func_sub_play = IMPORT(sub_play);
  auto func_sub_destroy = IMPORT(sub_destroy);
  auto func_sub_grab_frame = IMPORT(sub_grab_frame);
  auto func_sub_get_stream_count = IMPORT(sub_get_stream_count);

  auto handle = func_sub_create("MyMediaPipeline");

  func_sub_play(handle, mediaUrl);

  if(func_sub_get_stream_count(handle) == 0)
    throw runtime_error("No streams found");

  vector<uint8_t> buffer;

  for(int i = 0; i < 100; ++i)
  {
    FrameInfo info {};
    buffer.resize(1024 * 1024 * 10);
    auto size = func_sub_grab_frame(handle, 0, buffer.data(), buffer.size(), &info);
    buffer.resize(size);

    if(size == 0)
      Sleep(100);
    else
    {
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
  }

  func_sub_destroy(handle);
  FreeLibrary(lib);
}

int main(int argc, char const* argv[])
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

