#pragma once

#include "bt-utils\Strings.h"
#include "Matrix.h"

typedef enum {
   saPosition, 
   saColor, 
   saTexCoords, 
   saCOUNT
} e_shaderAttributes;

typedef struct {
   StringView name;
   e_shaderAttributes id;
   int size, itemCount;
} ShaderAttribute;

//literally ms3d
static ShaderAttribute shaderAttributes[] = {
   {"position",   saPosition,    sizeof(float)*2, 2}, 
   {"color",      saColor,       sizeof(float), 1},
   {"texCoords",  saTexCoords,   sizeof(float)*2, 2}
};

typedef struct Shader_t Shader;

Shader *shaderCreate(StringView vertexPath, StringView fragmentPath);
void shaderDestroy(Shader *self);

void shaderSetActive(Shader *self);

void shaderSet4fv(Shader *self, StringView u, Matrix *m);
void shaderSet1i(Shader *self, StringView u, int i);

