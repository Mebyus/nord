#include <ctype.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <SDL2/SDL.h>

#include "glad/glad.c"

#include "core.hpp"

#include "core_linux.cpp"

const i32 window_width = 800;
const i32 window_height = 600;

var SDL_Window* window_ptr = nil;
var SDL_GLContext gl_context = nil;

var u32 gl_ao_handle = 0;
var u32 gl_bo_handle = 0;

var u32 gl_prog_handle = 0;
var u32 gl_pipeline_handle = 0;

fn void print_gl_version_info() noexcept {
  const u8* vendor = glGetString(GL_VENDOR);
  const u8* renderer = glGetString(GL_RENDERER);
  const u8* version = glGetString(GL_VERSION);
  const u8* shadlang = glGetString(GL_SHADING_LANGUAGE_VERSION);

  stderr_write_all(macro_static_str("gl.vendor:   "));
  stdout_write_all(cstr(const_cast<u8*>(vendor)).as_str());
  stderr_write_all(macro_static_str("\n"));

  stderr_write_all(macro_static_str("gl.renderer: "));
  stdout_write_all(cstr(const_cast<u8*>(renderer)).as_str());
  stderr_write_all(macro_static_str("\n"));

  stderr_write_all(macro_static_str("gl.version:  "));
  stdout_write_all(cstr(const_cast<u8*>(version)).as_str());
  stderr_write_all(macro_static_str("\n"));

  stderr_write_all(macro_static_str("gl.shadlang: "));
  stdout_write_all(cstr(const_cast<u8*>(shadlang)).as_str());
  stderr_write_all(macro_static_str("\n"));
}

fn void init() noexcept {
  var i32 r = SDL_Init(SDL_INIT_VIDEO);
  if (r < 0) {
    stdout_write_all(macro_static_str("failed to init sdl"));
    exit(1);
  }

  window_ptr = SDL_CreateWindow("SDL2 window", 0, 0, window_width,
                                window_height, SDL_WINDOW_OPENGL);
  if (window_ptr == nil) {
    stdout_write_all(macro_static_str("failed to create sdl window"));
    exit(1);
  }

  r = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  if (r < 0) {
    stdout_write_all(macro_static_str("failed to set gl major version"));
    exit(1);
  }

  r = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  if (r < 0) {
    stdout_write_all(macro_static_str("failed to set gl minor version"));
    exit(1);
  }

  r = SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                          SDL_GL_CONTEXT_PROFILE_CORE);
  if (r < 0) {
    stdout_write_all(macro_static_str("failed to set gl profile mask"));
    exit(1);
  }

  r = SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  if (r < 0) {
    stdout_write_all(macro_static_str("failed to set gl double buffer"));
    exit(1);
  }

  r = SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  if (r < 0) {
    stdout_write_all(macro_static_str("failed to set gl depth size"));
    exit(1);
  }

  gl_context = SDL_GL_CreateContext(window_ptr);
  if (gl_context == nil) {
    stdout_write_all(macro_static_str("failed to create gl context"));
    exit(1);
  }

  r = gladLoadGLLoader(SDL_GL_GetProcAddress);
  if (r < 0) {
    stdout_write_all(macro_static_str("failed to setup glad"));
    exit(1);
  }

  print_gl_version_info();
}

fn void specify_vertices() noexcept {
  const f32 p[] = {
      -0.8f, -0.8f, 0.0f,  //
      0.8f,  -0.8f, 0.0f,  //
      0.0f,  0.8f,  0.0f,  //
  };

  var chunk<f32> vp = chunk<f32>(const_cast<f32*>(p), sizeof(p) / sizeof(f32));

  glGenVertexArrays(1, &gl_ao_handle);
  glBindVertexArray(gl_ao_handle);

  glGenBuffers(1, &gl_bo_handle);
  glBindBuffer(GL_ARRAY_BUFFER, gl_bo_handle);
  glBufferData(GL_ARRAY_BUFFER, vp.size(), vp.ptr, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nil);

  // TODO: cleanup ?
  glBindVertexArray(0);
  glDisableVertexAttribArray(0);
}

fn void bind_shader_to_handle(u32 handle, str src) noexcept {
  const i32 lens[] = {cast(i32, src.len)};
  glShaderSource(handle, 1, cast(char**, &src.ptr), lens);
  glCompileShader(handle);
}

fn u32 compile_vertex_shader(str src) noexcept {
  const u32 handle = glCreateShader(GL_VERTEX_SHADER);
  bind_shader_to_handle(handle, src);
  return handle;
}

fn u32 compile_fragment_shader(str src) noexcept {
  const u32 handle = glCreateShader(GL_FRAGMENT_SHADER);
  bind_shader_to_handle(handle, src);
  return handle;
}

fn void create_shader_prog() noexcept {
  gl_prog_handle = glCreateProgram();

  var str vertex_shader_filename = macro_static_str("shaders/vertex.glsl");
  var str fragment_shader_filename = macro_static_str("shaders/fragment.glsl");

  fs::FileReadResult rr = fs::read_file(vertex_shader_filename);
  if (rr.is_err()) {
    stdout_write_all(macro_static_str("failed to load vertex shader source"));
    exit(1);
  }
  var str vertex_shader_source = rr.data;

  rr = fs::read_file(fragment_shader_filename);
  if (rr.is_err()) {
    stdout_write_all(macro_static_str("failed to load fragment shader source"));
    exit(1);
  }
  var str fragment_shader_source = rr.data;

  const u32 vertex_shader = compile_vertex_shader(vertex_shader_source);
  const u32 fragment_shader = compile_fragment_shader(fragment_shader_source);

  glAttachShader(gl_prog_handle, vertex_shader);
  glAttachShader(gl_prog_handle, fragment_shader);

  glLinkProgram(gl_prog_handle);
  glValidateProgram(gl_prog_handle);
}

fn void create_gl_pipeline() noexcept {
  create_shader_prog();
}

fn void handle_input() noexcept {
  SDL_Event e;

  while (SDL_PollEvent(&e) != 0) {
    if (e.type == SDL_QUIT) {
      exit(0);
    }
  }
}

fn void setup_draw_pipeline() noexcept {
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  glViewport(0, 0, window_width, window_height);
  glClearColor(0.2f, 0.2f, 0.0f, 1.0f);

  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

  glUseProgram(gl_prog_handle);
}

fn void draw() noexcept {
  glBindVertexArray(gl_ao_handle);
  glBindBuffer(GL_ARRAY_BUFFER, gl_bo_handle);

  glDrawArrays(GL_TRIANGLES, 0, 3);
}

fn void run_main_loop() noexcept {
  while (true) {
    handle_input();
    setup_draw_pipeline();
    draw();

    SDL_GL_SwapWindow(window_ptr);
  }
}

fn i32 main() noexcept {
  init();
  specify_vertices();
  create_gl_pipeline();
  run_main_loop();
  return 0;
}
