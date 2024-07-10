#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdalign.h>

int readSPVBinaryFile(const char* filename, char** data, size_t* length) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("%s\n", filename);
        perror("Failed to open file");
        return -1;
    }

    // Determine the file size
    fseek(file, 0, SEEK_END);
    uint32_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize == -1) {
        perror("Failed to determine file size");
        fclose(file);
        return -1;
    }
    size_t alignment = alignof(uint32_t);
    // Allocate memory for the file contents
    *data = aligned_alloc(alignment, sizeof(char)*fileSize);
    if (data == NULL) {
        perror("Failed to allocate memory");
        fclose(file);
        return -1;
    }

    // Read the file into the allocated memory
    size_t bytesRead = fread(*data, 1, fileSize, file);
    printf("%s: %u\n", filename, bytesRead);
    if (bytesRead != fileSize) {
        perror("Failed to read file");
        free(*data);
        fclose(file);
        return -1;
    }

    // Set the length of the data
    *length = fileSize;

    fclose(file);
    return 0;
}
