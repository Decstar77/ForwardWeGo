/*
    Emscripten compatibility stub for glad/glad.h
    Redirects to GLES3 headers provided by Emscripten.
    Desktop GL function signatures map to WebGL2 (OpenGL ES 3.0).
*/
#ifndef __glad_h_
#define __glad_h_

#include <GLES3/gl3.h>
#include <GLES3/gl3platform.h>

// glad loader is not needed — Emscripten binds GL functions directly.
// Provide a no-op stub so existing gladLoadGLLoader calls compile.
typedef void* (* GLADloadproc)(const char *name);
inline int gladLoadGLLoader(GLADloadproc) { return 1; }

#endif // __glad_h_
