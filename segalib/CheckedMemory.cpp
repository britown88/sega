#include "CheckedMemory.h"

#include <unordered_map>



//in a .cpp file
struct AllocData
{
   AllocData(){}
   AllocData(std::string file, size_t line, size_t bytes):file(file), line(line), bytes(bytes){}
   std::string file;
   size_t line, bytes;
};

static std::unordered_map<void*, AllocData> _memMap;

void* checkedMallocImpl(size_t sz, char* file, size_t line){
   auto a = malloc(sz);
   _memMap[a] = AllocData(file, line, sz);
   return a;
}
void* checkedCallocImpl(size_t count, size_t sz, char* file, size_t line){
   auto a = calloc(count, sz);
   _memMap[a] = AllocData(file, line, count*sz);
   return a;
}
void checkedFreeImpl(void* mem){
   _memMap.erase(mem);
   free(mem);
}
void printMemoryLeaks(){
   FILE *output = fopen("memleak.txt", "wt");
   for(auto &pair : _memMap) {
      auto &data = pair.second;
      fprintf(output, "%i bytes in %s:%i\n", data.bytes,data.file.c_str(), data.line);
   }

   fclose(output);
}