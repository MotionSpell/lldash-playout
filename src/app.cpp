// Interactive standalone application, for signals-unity-bridge.so testing.
//
// Don't introduce direct dependencies to signals here, keep this program standalone,
// as it serves as an example on how to use the DLL.
#include <cstdio>
#include <exception>
#include "dynlib.h"
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
  { /* xy */ -1, -1, /* uv */ 0, 0 },
  { /* xy */ -1, +1, /* uv */ 0, 1 },
  { /* xy */ +1, +1, /* uv */ 1, 1 },

  { /* xy */ -1, -1, /* uv */ 0, 0 },
  { /* xy */ +1, +1, /* uv */ 1, 1 },
  { /* xy */ +1, -1, /* uv */ 1, 0 },
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

void main() {
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

#define IMPORT(name) ((decltype(name)*)lib->getSymbol(# name))

void safeMain(int argc, char* argv[])
{
  if(argc != 2 && argc != 3)
    throw runtime_error("Usage: app.exe <signals-unity-bridge.dll> [media url]");

  SDL_Init(SDL_INIT_VIDEO);

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
    auto lib = loadLibrary(libName);
    auto func_UnitySetGraphicsDevice = IMPORT(UnitySetGraphicsDevice);
    auto func_sub_create = IMPORT(sub_create);
    auto func_sub_play = IMPORT(sub_play);
    auto func_sub_destroy = IMPORT(sub_destroy);
    auto func_sub_copy_video = IMPORT(sub_copy_video);

    func_UnitySetGraphicsDevice(nullptr, 0 /* openGL */, 0);

    auto handle = func_sub_create("DecodePipeline");

    func_sub_play(handle, uri.c_str());

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

