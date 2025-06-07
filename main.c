#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "compiler.h"
#define NOB_IMPLEMENTATION
#include "nob.h"

char* read_file(const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if(!file)
    {
        perror("Error");
        exit(69);
    }
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);
    char* buffer = (char*) malloc(file_size + 1);
    if(!buffer)
    {
        perror("Error");
        fclose(file);
        exit(69);
    }
    size_t num_of_readb = fread(buffer, sizeof(char), file_size, file);
    if(num_of_readb != file_size)
    {
        free(buffer);
        fclose(file);
        fprintf(stderr, "Error: Could not read file. Expected %zu got %zu\n",
                file_size, num_of_readb);
        exit(69);
    }
    buffer[file_size] = '\0';
    fclose(file);
    return buffer;
}

void usage(void)
{
    nob_log(NOB_INFO, "Usage: forth <source> <output>");
}

int main(int argc, char** argv)
{
    int result = 0;
    char* output = "a.out";
    if(argc < 2) {
        usage();
        exit(1);
    } else if(argc == 3) 
    {
        output = argv[2];
    }

    char* source = read_file(argv[1]);
    if(!compile(source, output)) result = 1;
    free(source);
    return result;
}
