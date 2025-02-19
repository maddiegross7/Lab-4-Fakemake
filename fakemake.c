#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>//this is to see if the fakemake file exists
#include "fields.h"
#include "jval.h"//unsure if needed
#include "dllist.h"//unsure if needed
#include "jrb.h"//unsure if needed

typedef struct definition{
    char *fileName;
    Dllist cSourceFiles;
    Dllist cHeaderFiles;
    char *executable;
    Dllist flags;
    Dllist libraries;
} Definition;

int isACFile(char *fileName){
    int stringLength = strlen(fileName);

    if(stringLength > 2){
        if(fileName[stringLength-2] == '.' && fileName[stringLength-1] == 'c'){
            return 1;
        }
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    //char *fileName;

    Definition *file =(Definition *)malloc(sizeof(Definition));
    file->fileName = NULL;
    file->executable = NULL;
    file->cHeaderFiles = new_dllist();
    file->cSourceFiles = new_dllist();
    file->flags = new_dllist();
    file->libraries = new_dllist();

    //this is for the whitespace 
    if(argc == 1){
        file->fileName = strdup("fmakefile");
    }else if(argc == 2){
        file->fileName = strdup(argv[1]);
    }else{
        fprintf(stderr, "Command line arguments not in the correct format");
        exit(1);
    }
    printf("fileName: %s\n", file->fileName);

    IS is = new_inputstruct(file->fileName);

    if(is == NULL){
        fprintf(stderr,"fakemake: %s No such file or directory\n", file->fileName);
    }

    while(get_line(is) >= 0){
        printf("NF: %i, line: %s", is->NF, is->text1);

        if(is->NF > 0){
            printf("NF greater than 0\n");

            if(strcmp(is->fields[0], "E") == 0){
                printf("this is the executable\n");
                if(file->executable == NULL){
                    file->executable = strdup(is->fields[1]);
                }else{
                    fprintf(stderr, "the executable file has already been declared");
                    exit(1);
                }
            }else if(strcmp(is->fields[0], "C") == 0){
                printf("this is the source files\n");
                for(int i = 1; i < is->NF; i++){
                    if(isACFile(is->fields[i]) > 0){
                        dll_append(file->cSourceFiles, new_jval_s(is->fields[i]));
                    }else{
                        fprintf(stderr, "file is not of type .c");
                    }
                }
            }else if(strcmp(is->fields[0], "H") == 0){
                printf("this is the header files\n");
                for(int i = 1; i < is->NF; i++){
                    if(access(is->fields[i], F_OK) == 0){
                        dll_append(file->cHeaderFiles, new_jval_s(is->fields[i]));
                    }
                }
            }else if(strcmp(is->fields[0], "F") == 0){
                printf("this is the flags\n");
            }else if(strcmp(is->fields[0], "L") == 0){
                printf("this is the libraries\n");
            }
        }

    }

    return 0;
}
