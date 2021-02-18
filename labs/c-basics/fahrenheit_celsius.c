#include <stdlib.h>
#include <stdio.h>

#define   LOWER  0       /* lower limit of table */
#define   UPPER  300     /* upper limit */
#define   STEP   20      /* step size */

/* print Fahrenheit-Celsius table */

int main(int argc, char**argv)
{
    //simple conversion cas e
    if(argc==2){
        printf("%c",argv[1]);
        int fahr=atoi(argv[1]);
        printf("Fahrenheit: %3d, Celcius: %6.1f\n", fahr, (5.0/9.0)*(fahr-32));    
    }//range conversion case 
    else if(argc>2){
        int fahr=atoi(argv[1]);
        for (fahr = fahr; fahr <= atoi(argv[2]); fahr = fahr + atoi(argv[3])){
            printf("Fahrenheit: %3d, Celcius: %6.1f\n", fahr, (5.0/9.0)*(fahr-32));
        }
    }
    
    return 0;
} 
