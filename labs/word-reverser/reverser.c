#include <stdio.h>

#define IN   1   /* inside a word */
#define OUT  0   /* outside a word */

/* count lines, words, and characters in input */

int main()

{
    int i, state;
    char c, word[100];
    state = OUT;

    i = 0;

    //eof= end of file 
    while ((c = getchar()) != EOF) {
        //when we recive one of these we are in the same word 

        //se encuentra con un espacio o un salto de linea o tab
        if (c == ' ' || c == '\n' || c == '\t') {
            word[i]=' ';
            word[i]=c;
            i++;
           
        //manejo de caracteres 
        }else{
            word[i] = c;
            //numero de caracteres
            i++;
        }

    }

    printf("\n");

    //just prints
/*    for(int j=i-1;j>=0;j--){
        printf("%c",word[j]);    
    }*/

    //reverses
    char tmp;
    for(int j=0;j<i/2;j++){
        tmp=word[j];
        word[j]=word[i-j-1];
        word[i-j-1]=tmp;
    }
    printf("%s",word);
    
    printf("\n");

    return 0;



}

