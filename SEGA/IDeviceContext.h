#pragma once


typedef struct IDeviceContext_t IDeviceContext;

typedef struct {
   void(*init)(IDeviceContext*);
   void(*destroy)(IDeviceContext*);
} IDeviceContextVTable;

struct IDeviceContext_t{
   IDeviceContextVTable *vTable;
};