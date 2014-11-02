#include "GL/glew.h"
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "Defs.h"
#include "Strings.h"

#include "segalib\CheckedMemory.h"
#include "segalib\BitBuffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stddef.h>



struct Shader_t {
   int handle;
};

static int compileShader(StringView path, int type) {
   long fSize;
   char *shader = readFullFile(path, &fSize);

   unsigned int handle = glCreateShader(type);
   if(handle) {
      int shaderLength = strlen(shader);
      int compileStatus;

      glShaderSource(handle, 1, &shader, &shaderLength);
      glCompileShader(handle);
      
      glGetShaderiv(handle, GL_COMPILE_STATUS, &compileStatus);
      if(!compileStatus) {
         handle = 0;
      }
   }

   checkedFree(shader);
   return handle;
}

static int buildShaderProgram(int vertex, int fragment) {
   int handle = glCreateProgram();
   if(handle)
   {
      int i, linkStatus;
      if(vertex == -1 || fragment == -1) {
         return 0;
      }

      for(i = 0; i < saCOUNT; ++i) {
         glBindAttribLocation(handle, i, shaderAttributes[i].name);
      }

      glAttachShader(handle, vertex);
      glAttachShader(handle, fragment);
      glLinkProgram(handle);

      
      glGetProgramiv(handle, GL_LINK_STATUS, &linkStatus);
      if(!linkStatus) {
         GLsizei log_length = 0;
         GLchar message[1024];
         glGetProgramInfoLog(handle, 1024, &log_length, message);
         return 0;
      }
         

      return handle;
   }

   return 0;
}

Shader *shaderCreate(StringView vertexPath, StringView fragmentPath) {
   int vert = compileShader(vertexPath, GL_VERTEX_SHADER);
   int frag = compileShader(fragmentPath, GL_FRAGMENT_SHADER);
   int handle = buildShaderProgram(vert, frag);
   Shader *r;

   if(!handle) {
      return NULL;
   }

   r = checkedMalloc(sizeof(Shader));
   r->handle = handle;

   return r;
}

void shaderDestroy(Shader *self) {
   checkedFree(self);
}

void shaderSetActive(Shader *self) {
   glUseProgram(self->handle);
}

void shaderSet4fv(Shader *self, StringView u, Matrix *m) {
   GLint uniHandle = glGetUniformLocation(self->handle, u);
   glUniformMatrix4fv(uniHandle, 1, false, m->data);
}

void shaderSet1i(Shader *self, StringView u, int i) {
   GLint uniHandle = glGetUniformLocation(self->handle, u);
   glUniform1i(uniHandle, i);
}