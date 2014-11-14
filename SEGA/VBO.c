#include "GL/glew.h"
#include <GLFW/glfw3.h>
#include "VBO.h"
#include "segashared\CheckedMemory.h"

#include <malloc.h>
#include <string.h>

static VBO *_currentVBO = NULL;

typedef struct {
   e_shaderAttributes attr;
   int offset, dataSize;
   bool persist;

   byte *data;
} VertexAttr;

struct VBO_t {
   int attrCount;
   VertexAttr attrs[saCOUNT];

   unsigned int handle, vCount;
};


void vboAddAttribute(VBO *self, e_shaderAttributes attr, bool persistAfterBind) {
   VertexAttr *a = &self->attrs[self->attrCount];   
   ++self->attrCount;

   a->attr = attr;
   a->persist = persistAfterBind;
   
}
void vboBuild(VBO *self, unsigned int vertexCount) {
   int i, offset;

   self->vCount = vertexCount;

   //allocate the data for the vertices
   offset = 0;
   for(i = 0; i < self->attrCount; ++i) {
      int size = shaderAttributes[self->attrs[i].attr].size * vertexCount;
      
      self->attrs[i].dataSize = size;
      self->attrs[i].data = checkedCalloc(1, size);
      self->attrs[i].offset = offset;
      offset += size;

   }  
}

VBO *vboCreate() {
   return checkedCalloc(1, sizeof(VBO));
}

void vboDestroy(VBO *self) {
   int i;
   for(i = 0; i < self->attrCount; ++i) {
      if(self->attrs[i].data) {
         checkedFree(self->attrs[i].data);
      }
   }

   if(self->handle) {
      glDeleteBuffers(1, &self->handle);
   }

   checkedFree(self);      
}

unsigned int vboGetVertexCount(VBO *self) {
   return self->vCount;
}

//returns the entire datachunk for all vertices for that attribute
void *vboGetAttributeChunk(VBO *self, e_shaderAttributes attr) {
   int i;
   for(i = 0; i < self->attrCount; ++i){
      if(self->attrs[i].attr == attr) {
         return self->attrs[i].data;
      }
   }

   return NULL;
}

//sets data for a single attribute of a single vertex (this doesnt bound anything)
void vboSetSingleVertex(VBO *self, e_shaderAttributes attr, unsigned int vIndex, void *data) {
   int i;
   for(i = 0; i < self->attrCount; ++i){
      if(self->attrs[i].attr == attr) {
         //attr found, copy data in
         int size = shaderAttributes[attr].size;
         memcpy(self->attrs[i].data + (vIndex * size), data, size);
         break;
      }
   }
}

unsigned int vboGetGLHandle(VBO *self) {

   if(!self->handle && self->attrCount > 0) {
      //first we create one giant array from the chunks for the buffer
      VertexAttr *last = &self->attrs[self->attrCount - 1];
      int allDataSize = last->offset + last->dataSize;
      byte *allData = checkedMalloc(allDataSize);
      int i;
      bool dynamic = false;

      //and we loop over the attrs, copy into the big one, and then free any nonpersistent chunks
      for(i = 0; i < self->attrCount; ++i) {
         VertexAttr *current = &self->attrs[i];
         memcpy(allData + current->offset, current->data, current->dataSize);
         if(!current->persist) {
            checkedFree(current->data);
            current->data = NULL;
         }  
         else {
            dynamic = true;
         }
      }

      glGenBuffers(1, (GLuint*)&self->handle);
      glBindBuffer(GL_ARRAY_BUFFER, self->handle);
      glBufferData(GL_ARRAY_BUFFER, allDataSize, allData, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

      checkedFree(allData);
   }

   return self->handle;
}

void vboPush(VBO *self) {
   int i;
   vboMakeCurrent(self);
   for(i = 0; i < self->attrCount; ++i) {
      if(self->attrs[i].data) {
         VertexAttr *current = &self->attrs[i];

         glBindBuffer(GL_ARRAY_BUFFER, self->handle);
         glBufferSubData(GL_ARRAY_BUFFER, current->offset, current->dataSize, current->data); 
		   glBindBuffer(GL_ARRAY_BUFFER, 0);
      }
   }
}

//calls the handle itself
void vboMakeCurrent(VBO *self) {
   int i, current = 0;

   if(_currentVBO != self) {
      unsigned int handle = vboGetGLHandle(self);
      glBindBuffer(GL_ARRAY_BUFFER, handle);

      for(i = 0; i < self->attrCount; ++i) {
         VertexAttr *v = &self->attrs[i];
         int idx =  v->attr;

         //disable vertices we may have skipped
         for (; current < idx; ++current ) {
            glDisableVertexAttribArray(current);
         }

         glEnableVertexAttribArray(idx);
         glVertexAttribPointer(idx, shaderAttributes[idx].itemCount, GL_FLOAT, GL_FALSE, 0, (void*)v->offset);
         ++current;
      }

      //disable any remaining after we reach out final
      for (; current < saCOUNT; ++current ) { 
         glDisableVertexAttribArray(current); 
      }

      _currentVBO = self;
   }
}