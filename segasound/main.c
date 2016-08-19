#include "segautils/IncludeWindows.h"
#include "segautils/Defs.h"

#include <math.h>
#include <malloc.h>

#define SAMPLES 23000
#define PI  3.14159265358979323846


WAVEFORMATEX buildWaveFormat() {
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

typedef struct  {
   byte *samples;
   double freq;
   size_t samplePeriod;
}Note;

Note buildNote(double freq) {
   Note out = { .samples = calloc(1, SAMPLES),.freq = freq,.samplePeriod = 0 };

   int i = 0;
   double period = 1.0 / freq;
   double sinScale = 2.0 * (PI / period);
   out.samplePeriod = (int)(period * SAMPLES) + 1;
   double t = 0.0;
   double tStep = (1.0 / SAMPLES);
   double v;
   for (i = 0; i < SAMPLES; ++i) {      
      v = sin(sinScale * t);
      v = (v + 1.0) * 0.5;
      v = MAX(0.0, MIN(1.0, v));

      out.samples[i] = (byte)(255.0 * v);
      t += tStep;
   }

   return out;
}

WAVEHDR hdr = { 0 };

void playNote(HWAVEOUT *hwo, Note *n, int len) {
   int err;

   hdr.lpData = n->samples;
   hdr.dwBufferLength = SAMPLES;
   hdr.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
   hdr.dwLoops = 2;
   
   

   err = waveOutPrepareHeader(*hwo, &hdr, sizeof(WAVEHDR));
   err = waveOutWrite(*hwo, &hdr, sizeof(WAVEHDR));

   while (waveOutUnprepareHeader(*hwo, &hdr, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING) {
      Sleep(0);
   }
}



int main() {
   WAVEFORMATEX fmt = buildWaveFormat();
   HWAVEOUT hwo = { 0 };
   
   int err = 0;

   MMRESULT result = waveOutOpen(&hwo, WAVE_MAPPER, &fmt, NULL, NULL, CALLBACK_NULL);


   int i = 0;

   Note G = buildNote(392.00);
   Note A = buildNote(440.00);
   Note B = buildNote(494.00);
   Note C = buildNote(523.25);
   Note D = buildNote(587.33);
   Note E = buildNote(659.25);
   Note F = buildNote(698.46);
   

   
   hdr.dwLoops = 0;/**/
   

   for (i = 0; i < 10; ++i) {
      
      playNote(&hwo, &G, 1000);
      playNote(&hwo, &A, 1000);
      playNote(&hwo, &B, 1000);
      playNote(&hwo, &C, 1000);
      playNote(&hwo, &D, 1000);
      playNote(&hwo, &E, 1000);
      playNote(&hwo, &F, 1000);
   }
   

   

   

   //while (waveOutUnprepareHeader(hwo, &hdr, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING) {      
      
   //}

   waveOutClose(hwo);
   return 0;





}

//#define M_PI  3.14159265358979323846
//#define FREQUENCY 144.0 // that extra ".0" is important!
//#define SAMPLES 44100
//
//int open16PCM(LPHWAVEOUT phwo) {
//   // open a wave-out device, for 16-bit PCM playback
//   // most of this code was copied from the net
//   WAVEFORMATEX fmt;
//   fmt.cbSize = 0;
//   fmt.nChannels = 1;
//   fmt.nSamplesPerSec = 44100;
//   fmt.wBitsPerSample = 16;
//
//   fmt.nBlockAlign = (fmt.wBitsPerSample >> 3) * fmt.nChannels;
//   fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;
//
//   fmt.wFormatTag = WAVE_FORMAT_PCM;
//   return waveOutOpen(phwo, WAVE_MAPPER, &fmt, NULL, NULL, CALLBACK_NULL) == MMSYSERR_NOERROR;
//}
//
//int main(int argc, char *argv[]) {
//   HWAVEOUT hwo;
//
//   if (open16PCM(&hwo)) {
//      const double PERIOD = 1 / FREQUENCY;
//      const double TIME_STEP = 1 / (double)SAMPLES;
//
//      short data[SAMPLES];
//      double time = 0;
//      for (int i = 0; i < SAMPLES; ++i) {
//         // expand out the equation above in steps
//         double angle = time;
//         double factor = 0.5 * (sin(angle) + 1); // convert range that sin returns from [-1, 1] to [0, 1]
//
//                                                 //
//                                                 // factor is in the range [0, 1]
//                                                 // set the current sample to 2^(16-1) * factor
//                                                 // (since we're dealing with 16-bit PCM)
//                                                 // for a quieter wave, change 32768 to some
//                                                 // other maximum amplitude.
//                                                 //
//         short x = (short)(65535 * factor);
//         data[i] = x;
//         time += TIME_STEP;
//      }
//
//      //
//      // now populate the wave-header to send to Windows
//      //
//      WAVEHDR hdr;
//      hdr.dwBufferLength = 44100;
//      hdr.dwBytesRecorded = 0;
//      hdr.dwLoops = 5; // <-- change this to a higher value to make the wav loop for a number of times
//      hdr.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
//      hdr.lpData = (char *)data;
//
//      int err;
//      if ((err = waveOutPrepareHeader(hwo, &hdr, sizeof(WAVEHDR))) != MMSYSERR_NOERROR) {}
//
//      if ((err = waveOutWrite(hwo, &hdr, sizeof(WAVEHDR))) != MMSYSERR_NOERROR) {}
//
//      // now wait for the sound to finish playing
//      while (waveOutUnprepareHeader(hwo, &hdr, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING) {
//
//         Sleep(100);
//      }
//
//      waveOutClose(hwo);
//   }
//
//   return 0;
//}