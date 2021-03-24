

int mystrlen(char *str){
    int tamano=0;
    char *c;

    for(c=str;*c != '\0';c++){
        tamano++;
    }
    
    return tamano;
}

char *mystradd(char *origin, char *addition){
    char *añadido=origin+mystrlen(origin);

    while(*addition != '\0'){
        *añadido++= *addition++;

    }
    
    *añadido='\0';
    return origin;

}

int mystrfind(char *origin, char *substr){
    int onta = 0,
        PosOriginal = 0,
        i = 0,
        tamaño = mystrlen(substr);
    while(origin[PosOriginal] != '\0') {
        onta = PosOriginal;
        while(origin[PosOriginal] != '\0' && origin[PosOriginal]==substr[i] ){
            i++;
            PosOriginal++;
        }
        if(i == tamaño){return onta;}
        i = 0;
        PosOriginal++;   
    }
    return -1;
}
