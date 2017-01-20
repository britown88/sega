#pragma once

typedef struct Texture_t Texture;
typedef struct WorldView_t WorldView;

typedef struct Weather_t Weather;
Weather *createWeather(WorldView *view);

void testRain(Weather *self);

void weatherDestroy(Weather *self);
void weatherRender(Weather *self, Texture *tex);
