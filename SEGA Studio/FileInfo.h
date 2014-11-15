#pragma once

using namespace System;

enum FileTypes{
   PNG,
   EGA,
   PAL,
   Folder
};

public ref class FileInfo{
   public: 
   FileTypes type;
   String ^path;
   
   FileInfo(){}
   FileInfo(FileTypes type, String ^path):type(type), path(path){}
};