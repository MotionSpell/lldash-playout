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
// $ g++ app.cpp `pkg-config sdl2 epoxy --cflags --libs` -o dissidence_player
// $ ./dissidence_player signals-unity-bridge.dll [media url]
//
// Don't introduce direct dependencies to signals here.
// Keep this program standalone, as it's meant to be distributed as example.
//
#include <cstdio>
#include <exception>
#include <stdexcept>
#include "signals_unity_bridge.h"

#include <SDL.h>
#include <epoxy/gl.h>

using namespace std;

enum { attrib_position, attrib_uv };

struct Vertex
{
  float x, y;
  float u, v;
};

const Vertex vertices[] =
{
  { /* xy */ -1, -1, /* uv */ 0, 1 },
  { /* xy */ -1, +1, /* uv */ 0, 0 },
  { /* xy */ +1, +1, /* uv */ 1, 0 },

  { /* xy */ -1, -1, /* uv */ 0, 1 },
  { /* xy */ +1, +1, /* uv */ 1, 0 },
  { /* xy */ +1, -1, /* uv */ 1, 1 },
};

static
GLuint createShader(int type, const char* code)
{
  auto vs = glCreateShader(type);

  glShaderSource(vs, 1, &code, nullptr);
  glCompileShader(vs);

  GLint status;
  glGetShaderiv(vs, GL_COMPILE_STATUS, &status);

  if(!status)
    throw runtime_error("Shader compilation failed");

  return vs;
}

static
GLuint createProgram()
{
  static const char* vertex_shader = R"(#version 130
in vec2 pos;
in vec2 uv;
out vec2 UV;

void main()
{
  UV = uv;
  gl_Position = vec4( pos, 0.0, 1.0 );
}
)";

  static const char* fragment_shader = R"(#version 130
in vec2 UV;
out vec4 color;
uniform sampler2D mySampler;

void main()
{
  color = texture(mySampler, UV);
}
)";

  auto vs = createShader(GL_VERTEX_SHADER, vertex_shader);
  auto fs = createShader(GL_FRAGMENT_SHADER, fragment_shader);

  auto program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);

  glBindAttribLocation(program, attrib_position, "pos");
  glBindAttribLocation(program, attrib_uv, "uv");
  glLinkProgram(program);

  return program;
}

#define IMPORT(name) ((decltype(name)*)SDL_LoadFunction(lib, # name))

sub_handle* g_subHandle;
decltype(sub_copy_audio) * func_sub_copy_audio;

void audioCallback(void*, uint8_t* dst, int size)
{
  memset(dst, 0, size);

  // transfer current audio from pipeline
  func_sub_copy_audio(g_subHandle, dst, size);
}

void safeMain(int argc, char* argv[])
{
  if(argc != 2 && argc != 3)
    throw runtime_error("Usage: app.exe <signals-unity-bridge.dll> [media url]");

  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    throw runtime_error(string("Unable to initialize SDL: ") + SDL_GetError());

  SDL_AudioSpec wanted {};
  wanted.freq = 48000;
  wanted.channels = 2;
  wanted.samples = 2048;
  wanted.format = AUDIO_S16;
  wanted.callback = &audioCallback;

  SDL_AudioSpec actual {};

  if(SDL_OpenAudio(&wanted, &actual) < 0)
    throw runtime_error("Couldn't open SDL audio");

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE | SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  // Enable vsync
  SDL_GL_SetSwapInterval(1);

  auto window = SDL_CreateWindow("App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 512, 512, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  auto context = SDL_GL_CreateContext(window);
  auto program = createProgram();

  {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
  }

  {
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
  }

  glUseProgram(program);

#define OFFSET(a) \
  ((GLvoid*)(&((Vertex*)nullptr)->a))

  glEnableVertexAttribArray(attrib_position);
  glVertexAttribPointer(attrib_position, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), OFFSET(x));

  glEnableVertexAttribArray(attrib_uv);
  glVertexAttribPointer(attrib_uv, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), OFFSET(u));

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
  GLuint texture;
  glGenTextures(1, &texture);

  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  auto libName = argv[1];
  string uri = "videogen://";

  if(argc > 2)
    uri = argv[2];

  {
    auto lib = SDL_LoadObject(libName);
    auto func_UnitySetGraphicsDevice = IMPORT(UnitySetGraphicsDevice);
    auto func_sub_create = IMPORT(sub_create);
    auto func_sub_play = IMPORT(sub_play);
    auto func_sub_destroy = IMPORT(sub_destroy);
    auto func_sub_copy_video = IMPORT(sub_copy_video);
    func_sub_copy_audio = IMPORT(sub_copy_audio);

    func_UnitySetGraphicsDevice(nullptr, 0 /* openGL */, 0);

    auto handle = func_sub_create("DecodePipeline");
    g_subHandle = handle;

    func_sub_play(handle, uri.c_str());

    SDL_PauseAudio(0);

    bool keepGoing = true;

    while(keepGoing)
    {
      SDL_Event evt;

      while(SDL_PollEvent(&evt))
      {
        if(evt.type == SDL_QUIT || (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE))
        {
          keepGoing = false;
          break;
        }
      }

      // transfer current picture from pipeline to the OpenGL texture
      func_sub_copy_video(handle, (void*)(uintptr_t)texture);

      // do the actual drawing
      glClearColor(0, 1, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);

      glBindTexture(GL_TEXTURE_2D, texture);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(vertices) / sizeof(*vertices));

      // flush
      SDL_GL_SwapWindow(window);

      SDL_Delay(10);
    }

    SDL_PauseAudio(1);
    func_sub_destroy(handle);
  }

  SDL_CloseAudio();

  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
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

