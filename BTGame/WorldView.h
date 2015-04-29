#pragma once

typedef struct BTManagers_t BTManagers;
typedef struct EntitySystem_t EntitySystem;
typedef struct ImageLibrary_t ImageLibrary;

typedef struct {
   BTManagers *managers;
   EntitySystem *entitySystem;
   ImageLibrary *imageLibrary;
}WorldView;