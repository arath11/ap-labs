#include <stdio.h>
#include<stdlib.h>


int infof(const char *format, ...);
int warnf(const char *format, ...);
int errorf(const char *format, ...);
int panicf(const char *format, ...);


int main() {
    char *info = "Info generica ";
    infof("INFOF: %s\n",info);
    
    char *warning = "Warning generico";
    warnf("WARNF: %s\n",warning);
    
    char *error = "Error generico";
    errorf("ERRORF: %s\n",error);
    
    
    char *panic = "CORE DUMP GENERICO";
    panicf("PANICF: %s\n",panic);
    
    
    
    


    return 0;
}
