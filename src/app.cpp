#include <cstdio>
#include <exception>
#include "dynlib.h"
#include "signals_unity_bridge.h"

#define GL_GLEXT_PROTOTYPES

#include <SDL.h>
#include <SDL_opengl.h>
#include <cassert>

using namespace std;

static const char* vertex_shader =
  "#version 130\n"
  "in vec2 pos;\n"
  "out vec2 UV;\n"
  "\n"
  "void main() {\n"
  "    UV = vec2(pos.x, pos.y);\n"
  "    gl_Position = vec4( pos * 0.5, 0.0, 1.0 );\n"
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

enum { attrib_position };

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

void onError(GUBPipeline* userData, char* message)
{
  (void)userData;
  printf("Error: '%s'\n", message);
}

#define IMPORT(name) ((decltype(name)*)lib->getSymbol(# name))

void safeMain(int argc, char* argv[])
{
  if(argc != 2)
  {
    throw runtime_error("Usage: app.exe <my_library>");
  }

  SDL_Init(SDL_INIT_VIDEO);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE | SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  auto window = SDL_CreateWindow("App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
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
  glLinkProgram(program);

  glUseProgram(program);

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glEnableVertexAttribArray(attrib_position);
  glVertexAttribPointer(attrib_position, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)(0 * sizeof(float)));

  struct Vertex
  {
    float x, y;
  };

  const Vertex vertices[] =
  {
    { -1, -1 },
    { -1, +1 },
    { +1, +1 },

    { -1, -1 },
    { +1, +1 },
    { +1, -1 },
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  auto libName = argv[1];

  {
    auto lib = loadLibrary(libName);
    auto func_gub_ref = IMPORT(gub_ref);
    auto func_gub_unref = IMPORT(gub_unref);
    auto func_gub_pipeline_create = IMPORT(gub_pipeline_create);
    auto func_gub_pipeline_play = IMPORT(gub_pipeline_play);
    auto func_gub_pipeline_setup_decoding = IMPORT(gub_pipeline_setup_decoding);
    auto func_gub_pipeline_destroy = IMPORT(gub_pipeline_destroy);
    auto func_gub_pipeline_blit_image = IMPORT(gub_pipeline_blit_image);
    auto func_gub_pipeline_grab_frame_with_info = IMPORT(gub_pipeline_grab_frame_with_info);
    auto funcUnitySetGraphicsDevice = IMPORT(UnitySetGraphicsDevice);

    funcUnitySetGraphicsDevice(nullptr, 0 /* openGL */, 0);

    func_gub_ref("TheApp");

    auto handle = func_gub_pipeline_create("name", nullptr, &onError, nullptr, nullptr);

    GUBPipelineVars vars {};
    vars.uri = "http://livesim.dashif.org/livesim/testpic_2s/Manifest.mpd";
    func_gub_pipeline_setup_decoding(handle, &vars);

    func_gub_pipeline_play(handle);

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

      GUBPFrameInfo info {};
      int ret = func_gub_pipeline_grab_frame_with_info(handle, &info);
      printf("%dx%d (%d)\n", info.width, info.height, ret);
      glClearColor(0, 1, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);

      func_gub_pipeline_blit_image(handle, (void*)(uintptr_t)texture);

      glDrawArrays(GL_TRIANGLES, 0, sizeof(vertices) / sizeof(*vertices));
      SDL_GL_SwapWindow(window);

      SDL_Delay(10);
    }

    func_gub_pipeline_destroy(handle);

    func_gub_unref();
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

