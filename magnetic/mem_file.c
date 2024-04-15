#include "mem_file.h"
#include <string.h>
#include <stdlib.h>
#include "wmm-2020.h"

FILE * mem_fopen(const char * restrict path, const char * restrict mode){
    if ( strcmp(path,"WMM.COF" ) != 0){
        return NULL;
    }

    FILE * f = malloc(sizeof(FILE));
    f->_p = (unsigned char *)wmm_2020;
    f->_cookie = f->_p + strlen(wmm_2020);
    return f;
}

int mem_fclose(FILE *stream){
    free(stream);
    return 0;
}

char * mem_fgets(char * restrict str, int size, FILE * restrict f){
    for( int i=0; i < size; i++){
        if( f->_p == f->_cookie)
            return NULL;
        if( *f->_p == '\n'){
            str[i] = '\0';
            f->_p ++;
            return str;
        }
        str[i] = (char )(*f->_p);
        f->_p++;
    }
    return str;
}

