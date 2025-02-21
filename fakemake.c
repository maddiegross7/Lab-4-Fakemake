#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>//this is to see if the fakemake file exists
#include <sys/stat.h>//to use the stat functions on files
#include "fields.h"
#include "jval.h"
#include "dllist.h"
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

char *getOFileName(char *fileName){
    int stringLength = strlen(fileName);

    char *copy = strdup(fileName);

    copy[stringLength-1] = 'o';
    copy[stringLength] = '\0';
    
    return copy;
}

int isOOlderThanCFile(char *cFile, char *oFile){
    struct stat cFileStat, oFileStat;
    long cLastUpdate, oLastUpdate;

    if(stat(cFile, &cFileStat) != 0){
        fprintf(stderr, "c file does not exist");
    }else{
        cLastUpdate = cFileStat.st_mtime;
    }

    if(stat(oFile, &oFileStat) != 0){
        oLastUpdate = 0;
        //printf("file does not exist\n");
    }else{
        oLastUpdate = oFileStat.st_mtime;
    }

    printf("oLast: %ul, cLast: %ul\n", oLastUpdate, cLastUpdate);
    printf("o > c = %i\n", (oLastUpdate > cLastUpdate) ? 1 : 0);
    return oLastUpdate < cLastUpdate;
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

    IS is = new_inputstruct(file->fileName);

    if(is == NULL){
        fprintf(stderr,"fakemake: %s No such file or directory\n", file->fileName);
    }

    int flagSize = 0;
    int librariesSize = 0;
    while(get_line(is) >= 0){
        if(is->NF > 0){
            if(strcmp(is->fields[0], "E") == 0){
                if(file->executable == NULL){
                    file->executable = strdup(is->fields[1]);
                }else{
                    fprintf(stderr, "fmakefile (%i) cannot have more than one E line\n", is->line);
                    exit(1);
                }
            }else if(strcmp(is->fields[0], "C") == 0){
                for(int i = 1; i < is->NF; i++){
                    if(isACFile(is->fields[i]) > 0){
                        dll_append(file->cSourceFiles, new_jval_s(strdup(is->fields[i])));
                    }else{
                        fprintf(stderr, "file does not end in .c");
                    }
                }
            }else if(strcmp(is->fields[0], "H") == 0){
                for(int i = 1; i < is->NF; i++){
                    dll_append(file->cHeaderFiles, new_jval_s(strdup(is->fields[i])));
                }
            }else if(strcmp(is->fields[0], "F") == 0){
                for(int i = 1; i < is->NF; i++){
                    dll_append(file->flags, new_jval_s(strdup(is->fields[i])));
                    flagSize += strlen(is->fields[i]) + 1;
                }
            }else if(strcmp(is->fields[0], "L") == 0){
                for(int i = 1; i < is->NF; i++){
                    dll_append(file->libraries, new_jval_s(strdup(is->fields[i])));
                    librariesSize += strlen(is->fields[i]) + 1;
                }
            }else{
                fprintf(stderr, "I do not know what to do for this line");
            }
        }

    }

    if(file->executable == NULL){
        fprintf(stderr, "No executable specified\n");
        exit(1);
    }

    Dllist ptr;
    long newestChange = 0;
    dll_traverse(ptr, file->cHeaderFiles){
        char *fileName = (char *) ptr->val.s;

        struct stat fileStat;

        if(stat(fileName, &fileStat) != 0){
            fprintf(stderr, "header file does not exist");
        }

        if(fileStat.st_mtime > newestChange){
            newestChange = fileStat.st_mtime;
        }
    }

    char *flagsString = malloc(flagSize);

    dll_traverse(ptr, file->flags){
        char *flag = (char *) ptr->val.s;
        strcat(flagsString, " "); // Add space separator
        strcat(flagsString, flag);
    }

    Dllist oFiles = new_dllist();
    int oFileLength = 0;
    int executableNeedsMade = 0;
    long newestOFile = 0;
    dll_traverse(ptr, file->cSourceFiles){
        char *cFile = (char *) ptr->val.s;
        char *oFile = getOFileName(cFile);
        dll_append(oFiles, new_jval_s(strdup(oFile)));
        oFileLength += (strlen(oFile) + 1);

        struct stat cFileStat, oFileStat;
        long cLastUpdate, oLastUpdate;

        if(stat(cFile, &cFileStat) != 0){
            fprintf(stderr, "fmakefile: %s: No such file or directory\n", cFile);
            exit(1);
        }else{
            cLastUpdate = cFileStat.st_mtime;
        }

        if(stat(oFile, &oFileStat) != 0){
            oLastUpdate = 0;
            //printf("file does not exist\n");
        }else{
            oLastUpdate = oFileStat.st_mtime;
            if(oLastUpdate > newestOFile){
                newestOFile = oLastUpdate;
            }
        }

        if(oLastUpdate < cLastUpdate || oLastUpdate < newestChange){
            int commandSize = snprintf(NULL, 0, "gcc -c%s %s",flagsString, cFile) + 1;

            char *command = malloc(commandSize);

            snprintf(command, commandSize, "gcc -c%s %s",flagsString, cFile);
            printf("%s\n",command);
            int result = system(command);
            if(result != 0){
                fprintf(stderr, "Command failed.  Exiting\n");
                exit(1);
            }
            executableNeedsMade = 1;
        }

    }

    struct stat executableStat;
    long executableLastUpdate;
    if(stat(file->executable, &executableStat) != 0){
        executableLastUpdate = 0;
        //printf("file does not exist\n");
    }else{
        executableLastUpdate = executableStat.st_mtime;
    }

    if(executableNeedsMade == 1 || executableLastUpdate < newestOFile){
        char *oFileString = malloc(oFileLength);

        dll_traverse(ptr, oFiles){
            char *file = (char *) ptr->val.s;
            strcat(oFileString, " "); // Add space separator
            strcat(oFileString, file);
        }
        oFileString[oFileLength] = '\0';

        char *librariesString = malloc(librariesSize);

        dll_traverse(ptr, file->libraries){
            char *lib = (char *) ptr->val.s;
            strcat(librariesString, " "); // Add space separator
            strcat(librariesString, lib);
        }

        //printf("libs: %s", librariesString);
        int commandSize = snprintf(NULL, 0, "gcc -o %s%s%s%s",file->executable, flagsString, oFileString, librariesString) + 1;

        char *command = malloc(commandSize);

        snprintf(command, commandSize, "gcc -o %s%s%s%s",file->executable,flagsString, oFileString, librariesString);

        printf("%s\n", command);
        int result = system(command);

        if(result != 0){
            fprintf(stderr, "Command failed.  Fakemake exiting\n");
            exit(1);
        }

        free(librariesString);
        free(oFileString);
        free(command);
    }else{
        printf("%s up to date\n", file->executable);
    }

    return 0;
}
