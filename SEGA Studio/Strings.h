#pragma once

#include <string>
#include < stdio.h >
#include < stdlib.h >
#include < vcclr.h >

using namespace System;


inline std::string toNative(String^in) {

   // Pin memory so GC can't move it while native function is called
   pin_ptr<const wchar_t> wch = PtrToStringChars(in);

   size_t convertedChars = 0;
   size_t  sizeInBytes = ((in->Length + 1) * 2);
   errno_t err = 0;
   std::string out(sizeInBytes, ' ');


   err = wcstombs_s(&convertedChars, 
                     &out.front(), sizeInBytes,
                     wch, sizeInBytes);
   out.resize(convertedChars);

   return out;
}