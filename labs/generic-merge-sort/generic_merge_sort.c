#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *datos[300]; 

int *datosInt[300];
char *datosString[300];
int cantidadLineas=0;

void merge_sort(int,int,int (*comparar)(void *, void *));

void leerArchivo(char *nombreArchivo,int tipe){
    FILE * archivo;
    archivo=fopen(nombreArchivo,"r");
    //tipe 0 = string 
    //tipe 1 = int
    if(archivo==0){
        //return(0);
    }else{
        char * linea = NULL;
        size_t len= 0;
        ssize_t read;    
        while (read = getline(&linea, &len, archivo) != -1) {    
            if(tipe ==1){
                datosString[cantidadLineas]=malloc(strlen(linea));
                strcpy(datosString[cantidadLineas++],linea);
                //datosInt[cantidadLineas]=calloc(4,sizeof(int));
                //datosInt[cantidadLineas++]=atoi(linea);

            }else{
                //string
                datosString[cantidadLineas]=malloc(strlen(linea));
                strcpy(datosString[cantidadLineas++],linea);
            }
        }
        fclose(archivo);
    }    

}

int comparacionGenerica(char *s1, char *s2){
    double v1,v2;
    v1 = atof(s1);
    v2 = atof(s2);
    if(v1 < v2) 
        return -1;
    if(v1 > v2)
        return 1;
    return 0;    
}

void merge_sort(int izquierdo, int derecho, int (*comp)(void *, void *))
{
    //buscar hasta que el izquierdo y el derecho se encuentren 
    if (izquierdo >= derecho)
    {
        return;
    }
    //sacar el punto medio 
    int medio = (izquierdo + derecho) / 2;
    int izq = izquierdo;
    int med = medio + 1;
    int k=izquierdo;
    //llamar a si mismo 
    merge_sort(izquierdo, medio, comp);
    merge_sort(medio + 1, derecho, comp);
    //recorrido 
    while (k<=derecho)
    {
        //cuando el izquierdo llego al medio +1 se le tiene que sumar uno al med
        //y asignarle los datos que tenemos al datos 
        if (izq == medio + 1)
        {
            //printf("D:%s\n",datosString[med]);
            //reservar espacio
            //printf("CASO1\n");
            datos[k] = malloc(strlen(datosString[med]));


            strcpy(datos[k], datosString[med]);
            med++;
        }
        else if (med == derecho + 1)
        {
            //printf("CASO2\n");
            //printf("D:%s\n",datosString[izq]);
            //reservar espacio
            datos[k] = malloc(strlen(datosString[izq]));

            strcpy(datos[k], datosString[izq]);
            izq++;
        }
        else if ((*comp)(datosString[izq], datosString[med]) < 0)
        {
            //printf("CASO3\n");
            //printf("D:%s\n",datosString[izq]);
            //reservar espacio

            datos[k] = malloc(strlen(datosString[izq]));
            strcpy(datos[k], datosString[izq]);
            izq++;
        }
        else if((*comp)(datosString[izq], datosString[med]) > 0)
        {
            //printf("CASO4\n");
            //printf("D:%s\n",datosString[med]);
            //reservar espacio
            datos[k] = malloc(strlen(datosString[med]));
            strcpy(datos[k], datosString[med]);


            med++;
        }

        
        k++;
    }
    
    /*
    for (int z = 0;  z<= medio; z++)
    {
        printf("%s",datos[z]);
    }*/

    //rellenar datos faltante 
    k=izquierdo;
    while (k<=derecho)
    {
        strcpy(datosString[k], datos[k]);
        k++;
    } 
}

int main(int argc, char **argv) 
{
    for(int i =0; i<argc;i++){
        printf("%i:  %s \t",argc, argv[i]);
    }
    printf("\n\n");

    //checar si tiene más de un argumento 
    if(argc>1){
        //checar int 
        if(argc==2 && strcmp(argv[1],"-n")==0){
            printf("\nPlease insert complete parameters :)\n");    
            return 0;
        }else if(argc==2){
            printf("\nManejando strings :)\n");    
            leerArchivo(argv[1],0);        
            printf("Ya lo lei\n Numero De lineas:%i\n",cantidadLineas);
            //merge
            merge_sort(0, cantidadLineas - 1, (0 ? (int (*)(void *, void *))comparacionGenerica : (int (*)(void *, void *))strcmp));
            char salida[100];
            strcat(salida,"Sorted_");
            strcat(salida,argv[1]);
            printf("Archivo de salida %s\n",salida);

            FILE *subir;
            subir=fopen(salida,"w");

            if(subir==NULL){
                printf("Error con el documento \n");
                exit(0);
            }

            //for para subir datos
            
            for(int i =0;i<cantidadLineas;i++){
                //printf("%s",datosString[i]);
                fprintf(subir,"%s",datosString[i]);
            }
            printf("Ya subi los datos\n");
            fclose(subir);
                
        }else{
    
            printf("\nManejando ints :)\n");    
            leerArchivo(argv[2],1);
            
            printf("Ya lo lei\n Numero De lineas:%i\n",cantidadLineas);
                        
            //merge

            merge_sort(0, cantidadLineas - 1, (1 ? (int (*)(void *, void *))comparacionGenerica : (int (*)(void *, void *))strcmp));


            char salida[100];
            strcat(salida,"Sorted_");
            strcat(salida,argv[2]);
            printf("Archivo de salida %s\n",salida);

            FILE *subir;
            subir=fopen(salida,"w");

            if(subir==NULL){
                printf("Error con el documento \n");
                exit(0);
            }

            //for para subir datos
            
            for(int i =0;i<cantidadLineas;i++){
                //printf("%s",datosString[i]);
                fprintf(subir,"%s",datosString[i]);
            }
            printf("Ya subi los datos\n");
            fclose(subir);
        }
    }else{
        printf("\nPlease insert valid parameters :)\n");
        return 0;
    }
    return 0;
}