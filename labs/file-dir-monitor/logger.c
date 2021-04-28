#include <stdio.h>
#include "logger.h"
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <signal.h>

//0 Reset All Attributes (return to normal mode)
//1 Bright (usually turns on BOLD)
//2 Dim
//3 Underline
//5 Blink
//7 Reverse
//8 Hidden

//40 Black
//41 Red
//42 Green
//43 Yellow
//44 Blue
//45 Magenta
//46 Cyan
//47 White

#define colorFondoNegro 40

//30 Black
//31 Red
//32 Green
//33 Yellow
//34 Blue
//35 Magenta
//36 Cyan
//37 White

int isSTDOUT=0;
//0= stdout
//1= syslog


int initLogger(char *logType) {
    //check if it's stdout 
    
    if(strcmp(logType, "stdout") == 0 || strcmp(logType,"")==0) {
        printf("Initializing Logger on: %s\n", logType);
        isSTDOUT=0;
    } 
    else if(strcmp(logType, "syslog") == 0) {
        printf("Initializing Logger on: %s\n", logType);
        isSTDOUT=1;
    }else{
        printf("Opcion invalida");
        return 0;
    }
    return 0;
}

int imprimir(const char *format, va_list args){
    //stdout
    if(isSTDOUT==0){
        vprintf(format, args); 
        fflush(stdout);
    //syslog
    }else if(isSTDOUT==1){
        vsyslog(LOG_INFO, format, args);
        closelog();
    }
}

int infof(const char *format, ...) {
     //argumentos
    va_list valist;
    va_start(valist,format);
    
    //color
    printf("%c[0;%d;%dm",0x1B, 36, colorFondoNegro);
    
    //vprintf(format,valist);
    imprimir(format,valist);
    va_end(valist);
    
    //reset
    printf("%c[0;%d;%dm",0x1B, 37, colorFondoNegro);
}

int warnf(const char *format, ...) {
    //argumentos
    va_list valist;
    va_start(valist,format);

     //color
    printf("%c[0;%d;%dm",0x1B, 35, colorFondoNegro);    

    //vprintf(format,valist);
    imprimir(format,valist);
    va_end(valist);
    //reset
    printf("%c[0;%d;%dm",0x1B, 37, colorFondoNegro);

}

int errorf(const char *format, ...) {
    //argumentos
    va_list valist;
    va_start(valist,format);

     //color
    printf("%c[0;%d;%dm",0x1B, 31, colorFondoNegro);

    //vprintf(format,valist);
    imprimir(format,valist);
    va_end(valist);

    //reset
    printf("%c[0;%d;%dm",0x1B, 37, colorFondoNegro); 
}

int panicf(const char *format, ...) {
     //argumentos
    va_list valist;
    va_start(valist,format);

    //color
    printf("%c[1;%d;%dm",0x1B, 34, colorFondoNegro);
 
    //vprintf(format,valist);
    imprimir(format,valist);
    va_end(valist);
    //reset
    printf("%c[0;%d;%dm",0x1B, 37, colorFondoNegro);
}