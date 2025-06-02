#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

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
    fprintf(stderr, "Usage: forth <file>\n");
    exit(0);
}

int main(int argc, char** argv)
{
    if(argc < 2) {
        usage();
    }
    char* source = read_file(argv[1]);
    compile(source);
    free(source);
}
