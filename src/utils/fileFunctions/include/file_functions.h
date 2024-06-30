#include <stdlib.h>
// Function to read SPV files (char** with uint32_t align to be used for vulkan shader objects)
int readSPVBinaryFile(const char* filename, char** data, size_t* length);
