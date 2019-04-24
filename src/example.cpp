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
#include <cstdio>
#include <string>
#include <exception>
#include <functional>
#include <stdexcept>
#include "signals_unity_bridge.h"

#include <SDL.h>

#undef main

using namespace std;

#define IMPORT(name) ((decltype(name)*)SDL_LoadFunction(lib, # name))

struct SdlAudioOutput
{
  SdlAudioOutput(std::function<void(uint8_t*, int)> callback) : userCallback(callback)
  {
    SDL_AudioSpec wanted {};
    wanted.freq = 48000;
    wanted.channels = 2;
    wanted.samples = 2048;
    wanted.format = AUDIO_S16;
    wanted.callback = &audioCallback;
    wanted.userdata = this;

    SDL_AudioSpec actual {};

    if(SDL_OpenAudio(&wanted, &actual) < 0)
      throw runtime_error("Couldn't open SDL audio");

    SDL_PauseAudio(0);
  }

  ~SdlAudioOutput()
  {
    SDL_PauseAudio(1);
    SDL_CloseAudio();
  }

private:
  static void audioCallback(void* userParam, uint8_t* dst, int size)
  {
    auto pThis = reinterpret_cast<SdlAudioOutput*>(userParam);
    pThis->userCallback(dst, size);
  }

  std::function<void(uint8_t*, int)> const userCallback;
};

void safeMain(int argc, char const* argv[])
{
  if(argc != 2 && argc != 3)
    throw runtime_error("Usage: app.exe <signals-unity-bridge.dll> [media url]");

  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    throw runtime_error(string("Unable to initialize SDL: ") + SDL_GetError());

  const string libraryPath = argv[1];
  const string mediaUrl = argc > 2 ? argv[2] : "videogen://";

  auto lib = SDL_LoadObject(libraryPath.c_str());
  auto func_sub_create = IMPORT(sub_create);
  auto func_sub_play = IMPORT(sub_play);
  auto func_sub_destroy = IMPORT(sub_destroy);
  auto func_sub_grab_frame = IMPORT(sub_grab_frame);

  auto handle = func_sub_create("MyMediaPipeline");

  func_sub_play(handle, mediaUrl.c_str());

  (void)func_sub_grab_frame;

  func_sub_destroy(handle);
  SDL_UnloadObject(lib);

  SDL_Quit();
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

