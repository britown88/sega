#include "segautils/IncludeWindows.h"
#include "segautils/Math.h"

#include "segashared/CheckedMemory.h"

#include <math.h>
#include <fenv.h>
#include <malloc.h>
#include <stdio.h>>

#include "MusicBox.h"

#define TWELFTH_ROOT_OF_TWO 1.059463094
#define MIDDLE_A 440.0
#define MIDDLE_OCTAVE 3

typedef enum { A, As, B, C, Cs, D, Ds, E, F, Fs, G, Gs } Notes;

double noteFrequency(int octave, Notes note) {
   //int steps = (octave - MIDDLE_OCTAVE) * 12;
   //steps += (note - A) * 2;
   //switch (shape) {
   //case Sharp: ++steps; break;
   //case Flat: --steps; break;
   //}

   int steps = octave * 12 + (note - A);
   int middleA = MIDDLE_OCTAVE * 12;

   return MIDDLE_A * pow(TWELFTH_ROOT_OF_TWO, steps - middleA);
}



#define SAMPLES 22050
#define SEGMENT (SAMPLES>>4)

#define BLOCK_COUNT 200
#define BLOCK_SIZE SEGMENT

static WAVEFORMATEX _buildWaveFormat() {
   WAVEFORMATEX fmt = { 0 };
   fmt.wFormatTag = WAVE_FORMAT_PCM;
   fmt.nChannels = 1;
   fmt.nSamplesPerSec = SAMPLES;
   fmt.wBitsPerSample = 8;
   fmt.cbSize = 0;

   fmt.nBlockAlign = (fmt.nChannels * fmt.wBitsPerSample) >> 3;
   fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

   return fmt;
}

static void CALLBACK _waveOutProc(HWAVEOUT, UINT, DWORD, DWORD, DWORD);//callback for end of block


typedef struct {
   WAVEHDR waves[BLOCK_COUNT];
   byte blocks[BLOCK_SIZE * BLOCK_COUNT];

   CRITICAL_SECTION critSection;
   int current;
   volatile int freeCount;
}WaveBlock;

static void _buildWaveBlock(WaveBlock *self) {
   int i = 0;
   byte *blocks = self->blocks;

   memset(self, 0, sizeof(WaveBlock));

   self->current = 0;
   self->freeCount = BLOCK_COUNT;
   InitializeCriticalSection(&self->critSection);

   for (i = 0; i < BLOCK_COUNT; ++i) {
      self->waves[i].dwBufferLength = BLOCK_SIZE;
      self->waves[i].lpData = blocks;
      blocks += BLOCK_SIZE;
   }
}

static void _destroyWaveBlock(HWAVEOUT hwo, WaveBlock *self) {
   int i;
   for (i = 0; i < self->freeCount; i++) {
      if (self->waves[i].dwFlags & WHDR_PREPARED) {
         waveOutUnprepareHeader(hwo, &self->waves[i], sizeof(WAVEHDR));
      }
   }

   DeleteCriticalSection(&self->critSection);
}

static void CALLBACK _waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {
   WaveBlock *wb = (WaveBlock*)dwInstance;
   if (uMsg != WOM_DONE) {
      return;
   }

   EnterCriticalSection(&wb->critSection);
   wb->freeCount++;//we're done so free ourself
   LeaveCriticalSection(&wb->critSection);
}

static void _writeBlock(HWAVEOUT hwo, WaveBlock *wb, byte *buffer) {
   WAVEHDR* current;
   current = &wb->waves[wb->current];

   if (current->dwFlags & WHDR_PREPARED) {
      waveOutUnprepareHeader(hwo, current, sizeof(WAVEHDR));
   }

   memcpy(current->lpData, buffer, BLOCK_SIZE);
   current->dwBufferLength = BLOCK_SIZE;
   waveOutPrepareHeader(hwo, current, sizeof(WAVEHDR));
   waveOutWrite(hwo, current, sizeof(WAVEHDR));

   EnterCriticalSection(&wb->critSection);
   wb->freeCount--;
   LeaveCriticalSection(&wb->critSection);

   while (!wb->freeCount) {
      Sleep(10);
   }

   wb->current++;
   wb->current %= BLOCK_COUNT;
   current = &wb->waves[wb->current];
}

typedef double(*WaveFunction)(double, int);

static double _whiteNoise(double freq, double t) {
   return (rand() % 1000) * 0.001;
}
static double _sawTooth(double freq, int t) {
   int factor = 100;
   int sampsPerCycle = round((SAMPLES / freq) * factor);
   return (t*factor % sampsPerCycle) / (double)sampsPerCycle;
}
static double _sineWave(double freq, int t) {
   double v = sin(TAU * sawTooth(freq, t));
   v = (v + 1.0) * 0.5;
   return v;
}
static double _square(double freq, int t) {
   if (freq > 4000) {
      double v = sin(TAU * freq * (t / (double)SAMPLES));
      v = (v + 1.0) * 0.5;
      return v;
   }
   else {
      int sampsPerCycle = round(SAMPLES / freq);
      return ((t) % sampsPerCycle) < (sampsPerCycle / 2.0) ? 1.0 : 0.0;
   }

   
}

struct MusicBox_t{
   WAVEFORMATEX fmt;
   HWAVEOUT hwo;
   WaveBlock wb;
};

MusicBox *musicBoxCreate() {
   MusicBox *out = checkedCalloc(1, sizeof(MusicBox));

   out->fmt = _buildWaveFormat();
   _buildWaveBlock(&out->wb);
   waveOutOpen(&out->hwo, WAVE_MAPPER, &out->fmt, (DWORD_PTR)_waveOutProc, (DWORD_PTR)&out->wb, CALLBACK_FUNCTION);

   return out;
}
void musicBoxDestroy(MusicBox *self) {
   while (!musicBoxIsDone(self)) { Sleep(10); }
   _destroyWaveBlock(self->hwo, &self->wb);
   waveOutClose(self->hwo);
   checkedFree(self);
}

bool musicBoxIsDone(MusicBox *self) {
   return self->wb.freeCount == BLOCK_COUNT;
}

void _testPlayFreq(MusicBox *self, double f, int _len) {
   byte sample[SEGMENT] = { 0 };
   int len = _len * SEGMENT;
   int pos = 0;

   while (pos < len) {
      int i;

      memset(sample, 0, SEGMENT);
      for (i = 0; i < SEGMENT && pos + i < len; ++i) {
         int npos = pos + i;
         double res = _square(f, npos);

         //scale
         res *= 0.2;

         //gate
         res = MAX(0.0, MIN(1.0, res));

         sample[i] = (byte)(255.0 * res);
      }
      pos += i;
      _writeBlock(self->hwo, &self->wb, sample);
   }
}

void _testPlayNote(MusicBox *self, int octave, Notes note, int _len) {
   double freq = noteFrequency(octave, note);
   _testPlayFreq(self, freq, _len);
}

void musicBoxTest(MusicBox *self) {
   int note;
   
   static double freqs[85] = { 0 };
   static int samps[85] = { 0 };
   int i = 1;

   for (i = 1; i < 85; ++i) {
      freqs[i] = MIDDLE_A * pow(TWELFTH_ROOT_OF_TWO, i - 1 - 33);


      //_testPlayFreq(self, freqs[i], 4);
   }


   _testPlayNote(self, 0, E, 1);
   _testPlayNote(self, 0, F, 1);
   _testPlayNote(self, 0, G, 1);
   _testPlayNote(self, 0, E, 1);
   _testPlayNote(self, 0, F, 1);
   _testPlayNote(self, 0, D, 1);
   _testPlayNote(self, 0, C, 1);

   while (!musicBoxIsDone(self)) {}Sleep(1000);

   _testPlayNote(self, 1, B, 4);
   while (!musicBoxIsDone(self)) {}Sleep(200);
   _testPlayNote(self, 1, B, 2);
   _testPlayNote(self, 1, A, 2);
   _testPlayNote(self, 1, A, 2);
   while (!musicBoxIsDone(self)) {}Sleep(200);
   _testPlayNote(self, 1, B, 2);
   while (!musicBoxIsDone(self)) {}Sleep(200);
   _testPlayNote(self, 1, B, 2);
   while (!musicBoxIsDone(self)) {}Sleep(200);
   _testPlayNote(self, 1, B, 2);
   _testPlayNote(self, 1, A, 2);
   _testPlayNote(self, 1, A, 2);
   _testPlayNote(self, 1, A, 2);


   



}