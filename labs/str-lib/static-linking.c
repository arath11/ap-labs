#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <stdbool.h>


extern int mystrlen(char *str);
extern char *mystradd(char *origin, char *addition);
extern int mystrfind(char *origin, char *substr);


int main(int argc, char **argv) {
    
    if(strcmp(argv[1],"-add")==0 && argc==4){

        printf("\nInitial Lenght      : %d\n", mystrlen(argv[2]));
        char *newString = mystradd(argv[2], argv[3]);
        printf("New String          : %s\n", newString );
        printf("New String          : %d\n", mystrlen(newString));


        return 0;
    }else if(strcmp(argv[1],"-find")==0 && argc==4){


        int pos = mystrfind(argv[2], argv[3]);
        printf("\n[%s] string was found at [%d] position\n", argv[3], pos);

    }else{    
        printf("\nPlease enter valid comands\n\n");
        return 0;
    }


}
