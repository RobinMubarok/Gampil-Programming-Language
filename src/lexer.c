#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "diddiblead.h"



int main(int argc, char* argv[]){
    FILE* program = fopen(argv[1], "r");
    char code[1024], *reader;
    while(fgets(code, 1024, program)){

        reader = code;
        while(*reader != '\0'){
            if(!(*reader == '+')){
                printf("%c", *reader);
            }
            reader++;
        }

    }


    return 0;
}
