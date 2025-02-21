//Lab 4: Fakemake
//Madelyn Gross

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>//to use the stat functions on files
#include "fields.h"
#include "jval.h"
#include "dllist.h"

//this just checks that the last two chars are consistant with that of a c file
int isACFile(char *fileName){
    int stringLength = strlen(fileName);

    if(stringLength > 2){
        if(fileName[stringLength-2] == '.' && fileName[stringLength-1] == 'c'){
            return 1;
        }
    }
    return 0;
}

//converts the string for the cFile into what it would be in a oFile
char *getOFileName(char *fileName){
    int stringLength = strlen(fileName);

    char *copy = strdup(fileName);
    copy[stringLength-1] = 'o';
    copy[stringLength] = '\0';

    return copy;
}

int main(int argc, char const *argv[])
{
    //All of the things we need befre we parse the file;
    char *fileName = NULL;
    char *executable = NULL;
    Dllist cHeaderFiles = new_dllist();
    Dllist cSourceFiles = new_dllist();
    Dllist flags = new_dllist();
    Dllist libraries = new_dllist();

    //this is for the whitespace 
    //setting the file we are going be looking at
    if(argc == 1){
        fileName = strdup("fmakefile");
    }else if(argc == 2){
        fileName = strdup(argv[1]);
    }else{
        fprintf(stderr, "Command line arguments not in the correct format");
        exit(1);
    }

    IS is = new_inputstruct(fileName);
    //if we cannot acces the file
    if(is == NULL){
        fprintf(stderr,"fakemake: %s No such file or directory\n", fileName);
        exit(1);
    }

    int flagSize = 0;
    int librariesSize = 0;
    while(get_line(is) >= 0){
        if(is->NF > 0){
            //if the line is E set as executable
            //if executable is already set return that there is too many
            if(strcmp(is->fields[0], "E") == 0){
                if(executable == NULL){
                    executable = strdup(is->fields[1]);
                }else{
                    fprintf(stderr, "fmakefile (%i) cannot have more than one E line\n", is->line);
                    exit(1);
                }
            //add the cFiles to the list, make sure it ends in c
            }else if(strcmp(is->fields[0], "C") == 0){
                for(int i = 1; i < is->NF; i++){
                    if(isACFile(is->fields[i]) > 0){
                        dll_append(cSourceFiles, new_jval_s(strdup(is->fields[i])));
                    }else{
                        fprintf(stderr, "file does not end in .c");
                        exit(1);
                    }
                }
            }else if(strcmp(is->fields[0], "H") == 0){
                for(int i = 1; i < is->NF; i++){
                    dll_append(cHeaderFiles, new_jval_s(strdup(is->fields[i])));
                }
            }else if(strcmp(is->fields[0], "F") == 0){
                for(int i = 1; i < is->NF; i++){
                    dll_append(flags, new_jval_s(strdup(is->fields[i])));
                    flagSize += strlen(is->fields[i]) + 1;
                }
            }else if(strcmp(is->fields[0], "L") == 0){
                for(int i = 1; i < is->NF; i++){
                    dll_append(libraries, new_jval_s(strdup(is->fields[i])));
                    librariesSize += strlen(is->fields[i]) + 1;
                }
            //this just means this line does not contain any of this stuff
            }else{
                fprintf(stderr, "I do not know what to do for this line");
                exit(1);
            }
        }
    }
    //get rid of this bad boy cause we dont need him anymore
    jettison_inputstruct(is);

    //if no executable has been told we exit the program
    if(executable == NULL){
        fprintf(stderr, "No executable specified\n");
        exit(1);
    }

    Dllist ptr;
    
    //we are trying to determine the time of the last h file was touched 
    long newestChange = 0;
    dll_traverse(ptr, cHeaderFiles){
        char *fileName = (char *) ptr->val.s;

        struct stat fileStat;

        if(stat(fileName, &fileStat) != 0){
            fprintf(stderr, "header file does not exist");
            exit(1);
        }

        if(fileStat.st_mtime > newestChange){
            newestChange = fileStat.st_mtime;
        }

        free(fileName);
    }

    //creating a string with all the flags in it so that we can use them to compile
    char *flagsString = malloc(flagSize + 1);
    flagsString[0] = '\0'; 

    dll_traverse(ptr, flags){
        char *flag = (char *) ptr->val.s;
        strcat(flagsString, " "); // Add space separator
        strcat(flagsString, flag);

        free(flag);
    }

    flagsString[flagSize] = '\0';

    Dllist oFiles = new_dllist();
    int oFileLength = 0;
    int executableNeedsMade = 0;
    long newestOFile = 0;
    //running through all of the c files to determine if they need compiled
    //using stat to determine the last time they were touched
    dll_traverse(ptr, cSourceFiles){
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
        }else{
            oLastUpdate = oFileStat.st_mtime;
            if(oLastUpdate > newestOFile){
                newestOFile = oLastUpdate;
            }
        }

        free(oFile);

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

            free(command);
        }
        free(cFile);
    }
    //getting the executable stat if it exists
    struct stat executableStat;
    long executableLastUpdate;
    if(stat(executable, &executableStat) != 0){
        executableLastUpdate = 0;
    }else{
        executableLastUpdate = executableStat.st_mtime;
    }
    //if it does not exist of is less recent than the c files and or header files we will remake it
    if(executableNeedsMade == 1 || executableLastUpdate < newestOFile){
        //creating the Ofile string to put in the compilation of the execuatble
        char *oFileString = malloc(oFileLength+1);
        oFileString[0] = '\0';
        dll_traverse(ptr, oFiles){
            char *file = (char *) ptr->val.s;
            strcat(oFileString, " "); // Add space separator
            strcat(oFileString, file);

            free(file);
        }
        oFileString[oFileLength] = '\0';

        char *librariesString = malloc(librariesSize + 1);
        librariesString[0] = '\0';
        //creating the library string to put in the compilation of the exectuable
        dll_traverse(ptr, libraries){
            char *lib = (char *) ptr->val.s;
            strcat(librariesString, " "); // Add space separator
            strcat(librariesString, lib);

            free(lib);
        }
        librariesString[librariesSize] = '\0';

        //creating the command string this was really mean but we finally got it all to work
        const char *fmt = "gcc -o %s%s%s%s";

        int commandSize = snprintf(NULL, 0, fmt,executable, flagsString, oFileString, librariesString);
        char *command = malloc(commandSize+1);
        command[commandSize]='\0';

        snprintf(command, commandSize+1, fmt,executable,flagsString, oFileString, librariesString);

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
        //if nothing needs to happen we still need to free all of these lists since that is done
        //in the above if statement but still needs to happen anyway
        dll_traverse(ptr, oFiles){
            char *file = (char *) ptr->val.s;
            
            free(file);
        }
        dll_traverse(ptr, libraries){
            char *lib = (char *) ptr->val.s;

            free(lib);
        }
        printf("%s up to date\n", executable);
    }

    //freeing all the stuff we are finally done!!
    free_dllist(cHeaderFiles);
    free_dllist(cSourceFiles);
    free_dllist(flags);
    free_dllist(libraries);
    free_dllist(oFiles);

    free(fileName);
    free(executable);
    free(flagsString);

    return 0;
}
