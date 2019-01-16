// Interactive standalone application, for signals-unity-bridge.so testing.
#include <cstdio>
#include <exception>
#include "dynlib.h"
#include "signals_unity_bridge.h"

#define GL_GLEXT_PROTOTYPES

#include <SDL.h>
#include <epoxy/gl.h>
#include <cassert>

using namespace std;

static const char* vertex_shader =
  "#version 130\n"
  "in vec2 pos;\n"
  "in vec2 uv;\n"
  "out vec2 UV;\n"
  "\n"
  "void main() {\n"
  "    UV = uv;\n"
  "    gl_Position = vec4( pos, 0.0, 1.0 );\n"
  "}\n";

static const char* fragment_shader =
  "#version 130\n"
  "in vec2 UV;\n"
  "out vec4 color;\n"
  "uniform sampler2D mySampler;\n"
  "\n"
  "void main() {\n"
  "    color = texture(mySampler, UV);\n"
  "}\n";

enum { attrib_position, attrib_uv };

int createShader(int type, const char* code)
{
  auto vs = glCreateShader(type);

  glShaderSource(vs, 1, &code, nullptr);
  glCompileShader(vs);

  GLint status;
  glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
  assert(status);
  return vs;
}

struct Vertex
{
  float x, y;
  float u, v;
};

const Vertex vertices[] =
{
  { /* xy */ -1, -1, /* uv */ 0, 0 },
  { /* xy */ -1, +1, /* uv */ 0, 1 },
  { /* xy */ +1, +1, /* uv */ 1, 1 },

  { /* xy */ -1, -1, /* uv */ 0, 0 },
  { /* xy */ +1, +1, /* uv */ 1, 1 },
  { /* xy */ +1, -1, /* uv */ 1, 0 },
};

#define IMPORT(name) ((decltype(name)*)lib->getSymbol(# name))

void safeMain(int argc, char* argv[])
{
  if(argc != 2 && argc != 3)
  {
    throw runtime_error("Usage: app.exe <my_library> [url]");
  }

  SDL_Init(SDL_INIT_VIDEO);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE | SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  // Enable vsync
  SDL_GL_SetSwapInterval(1);

  auto window = SDL_CreateWindow("App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256, 256, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  auto context = SDL_GL_CreateContext(window);

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  auto vs = createShader(GL_VERTEX_SHADER, vertex_shader);
  auto fs = createShader(GL_FRAGMENT_SHADER, fragment_shader);

  auto program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);

  glBindAttribLocation(program, attrib_position, "pos");
  glBindAttribLocation(program, attrib_uv, "uv");
  glLinkProgram(program);

  glUseProgram(program);

  glActiveTexture(GL_TEXTURE0);

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glEnableVertexAttribArray(attrib_position);
  glVertexAttribPointer(attrib_position, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(0 * sizeof(float)));

  glEnableVertexAttribArray(attrib_uv);
  glVertexAttribPointer(attrib_uv, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(2 * sizeof(float)));

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
  GLuint texture;
  glGenTextures(1, &texture);

  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  auto libName = argv[1];
  string uri = "http://livesim.dashif.org/livesim/testpic_2s/Manifest.mpd";

  if(argc > 2)
    uri = argv[2];

  {
    auto lib = loadLibrary(libName);
    auto funcUnitySetGraphicsDevice = IMPORT(UnitySetGraphicsDevice);
    auto func_sub_create = IMPORT(sub_create);
    auto func_sub_play = IMPORT(sub_play);
    auto func_sub_destroy = IMPORT(sub_destroy);
    auto func_sub_copy_video = IMPORT(sub_copy_video);

    funcUnitySetGraphicsDevice(nullptr, 0 /* openGL */, 0);

    auto handle = func_sub_create("DecodePipeline");

    func_sub_play(handle, uri.c_str());

    bool keepGoing = true;

    while(keepGoing)
    {
      SDL_Event evt;

      while(SDL_PollEvent(&evt))
      {
        if(evt.type == SDL_QUIT)
        {
          keepGoing = false;
          break;
        }
      }

      glClearColor(0, 1, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);

      func_sub_copy_video(handle, (void*)(uintptr_t)texture);

      glBindTexture(GL_TEXTURE_2D, texture);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(vertices) / sizeof(*vertices));
      SDL_GL_SwapWindow(window);

      SDL_Delay(10);
    }

    func_sub_destroy(handle);
  }

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

