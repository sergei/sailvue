#ifndef SRC_MEM_FILE_H
#define SRC_MEM_FILE_H

#include <stdio.h>

FILE * mem_fopen(const char * restrict path, const char * restrict mode);
int mem_fclose(FILE *stream);
char * mem_fgets(char * restrict str, int size, FILE * restrict stream);

#define fopen mem_fopen
#define fclose mem_fclose
#define fgets mem_fgets

#endif //SRC_MEM_FILE_H
