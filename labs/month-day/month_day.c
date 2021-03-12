#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>



char* NombreDelMes(int n){
    static char* mes[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct", "Nov","Dec"};
    return mes[n-1];
}


void month_day(int year, int yearDay){    
    
    int dayOfMonth[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};

    if(!(366<yearDay||yearDay<1))
    {
        if(year%4==0)
        {
            if(year%100!=0||year%400==0)
            {//biciesto 
                dayOfMonth[2]=29;
                //printf("Año biciesto\n");
            }else
            {
                //printf("Año no biciesto\n");
            }
        }else
        {
                //printf("Año no biciesto\n");
        }
    
        bool cabeEnOtroMes=true;
    
        for(int mes=1;mes<sizeof(dayOfMonth) && cabeEnOtroMes;mes++)
        {
            //Esta en el mismo mes 
            if(yearDay<=dayOfMonth[mes])
            {
                cabeEnOtroMes=false;
                printf("\n%s %i, %i\n\n",NombreDelMes(mes),yearDay,year);

            }else
            {
                //aun falts restarle 
                yearDay=yearDay-dayOfMonth[mes];
            }   
        }
    }else
    {
        printf("------------------------------\n");
        printf("Dia invalido\n");
        printf("------------------------------\n");
        //return 1;
    }
}

int main(int args, char**argv) {
    //año
    //printf("%i\n",atoi(argv[1]));
    //dia 
    //printf("%i\n",atoi(argv[2]));
    if(args==3){
        month_day(atoi(argv[1]),atoi(argv[2]));
        return 0;
    }else{
        printf("------------------------------\n");
        printf("Please enter valid information\n");
        printf("------------------------------\n");
        return 1;
    }    
}