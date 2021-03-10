#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define REPORT_FILE "packages_report.txt"

typedef struct packages
{
    bool imprimir;
    char name[30];
    char installDate[30];
    char lastUpdate[30];
    int howManyUpdates;
    char removalDate[30];
    
    struct packages * next;

}packages_t;

void analizeLog(char *logFile, char *report);

bool siEsta(packages_t * head, char nombre[20]){
    packages_t * current = head;
    while (current != NULL) {
        if(strcmp(current->name,nombre)==0){
            return true;
        }
    current = current->next;
    }
    return false;
}


void push(packages_t * head, char nombre[20], char install_Date[30]){
    if(siEsta(head,nombre)){
        packages_t * current = head;    
        while (current != NULL) {
        if(strcmp(current->name,nombre)==0){
            current->imprimir=true;
            //printf("**************************************************************%s\n", current->name);    
        }
    current = current->next;
    }
    }else{
    packages_t * current = head;
    //checar si la cabeza tiene datos 
        while(current->next!=NULL){
            current=current->next;
        }
        current->next=(packages_t *)malloc(sizeof(packages_t));
        //current->next->val=val;
        strcpy(current->next->name,nombre);
        current->next->imprimir=true;
        strcpy(current->next->installDate,install_Date);
        strcpy(current->next->lastUpdate,"-");
        strcpy(current->next->removalDate,"-");
        current->next->next=NULL;       
    }
}

void print_list(packages_t * head) {
    packages_t * current = head;

    while (current != NULL) {
         printf("- Package Name:            %s\n    -Install date         %s\n    -Last update          %s\n    -How many updates      %i\n    -Removal date         %s\n",current->name,current->installDate,current->lastUpdate,current->howManyUpdates,current->removalDate);
        /*if(current->imprimir!=false){
            //printf("%d\n", current->val);
            printf("- Package Name:            %s\n    -Install date         %s\n    -Last update          %s\n    -How many updates      %i\n    -Removal date         %s\n",current->name,current->installDate,current->lastUpdate,current->howManyUpdates,current->removalDate);
        }*/
        current = current->next;
    }
}

void buscarNombre(packages_t *head, char nombre[20]){
    packages_t * current = head;

    while (current != NULL) {
        if(strcmp(current->name,nombre)==0){
            
            //if(strcmp(current->val,comparar)==0){
            printf("%s\n", current->name);    
        }
        current = current->next;
    }
}


void actualizarUpgraded(packages_t *head, char nombre[20],char last_upgraded[30]){
    packages_t * current = head;
    while (current != NULL) {
        if(strcmp(current->name,nombre)==0){
            current->howManyUpdates=current->howManyUpdates+1;
            strcpy(current->lastUpdate,last_upgraded);
        }
        current = current->next;
    }
}

void eliminarPaquete(packages_t *head, char nombre[20],char paqueteEliminado[30]){
    packages_t * current = head;

    while (current != NULL) {
        if(strcmp(current->name,nombre)==0){
            current->imprimir=false;         
            strcpy(current->removalDate,paqueteEliminado);
        }
        current = current->next;
    }
}


int main(int argc, char **argv) {
    packages_t * head= NULL;
    head = (packages_t *)malloc(sizeof(packages_t));
    
    head->howManyUpdates=0;
    
    char nombre[10]="aaa";
    strcpy(head->name,"");
    strcpy(head->installDate,"");
    strcpy(head->lastUpdate,"");
    strcpy(head->removalDate,"");
    head->imprimir=false;
    head->next=NULL;

    char nombreArchivoArg2[100];
    char nombreArchivoArg4[100];

    if (argc < 2) {
	    printf("Usage:./pacman-analizer.o \n");
	    return 1;
        }


    if(strcmp(argv[1],"-input")==0 && strcmp(argv[3],"-report")==0){
        //verificar que sean txt los dos 
        bool firstIsCorrect=false;
        bool secondIsCorrect=false;

        strcpy(nombreArchivoArg2,argv[2]);
        strcpy(nombreArchivoArg4,argv[4]);
        //printf("\n%s\n",nombreArchivoArg);

        char *archivo[100];
        
        int i=0;
        
        char *stringTokenizer = strtok(argv[2],".");
        
        while (stringTokenizer != NULL)
            {
                archivo[i]=stringTokenizer;
                i++;
                stringTokenizer=strtok(NULL,".");
        }
        char *archivo2[100];
        int j=0;
        char *stringTokenizer2 = strtok(argv[4],".");
        while (stringTokenizer2 != NULL)
            {
                archivo2[j]=stringTokenizer2;
                j++;
                stringTokenizer2=strtok(NULL,".");
        }

        if(strcmp(archivo[1],"txt")==0){
            firstIsCorrect=true;
        }else{
            printf("\nPlease verify that your using a  .txt\n\n");

        }
        if(strcmp(archivo2[1],"txt")==0){
            secondIsCorrect=true;
        }else{
            printf("\nPlease verify that your using a  .txt\n\n");
        }
        if(firstIsCorrect && secondIsCorrect){
            analizeLog(nombreArchivoArg2, nombreArchivoArg4);
        }
        memcpy(argv[2],archivo,sizeof(argv[2]));
        memcpy(argv[4],archivo2,sizeof(argv[4]));
        
        
    }else{
        printf("\nPlease verify the input command \n\n");
        return 0;
    }

    

    return 0;
}



void analizeLog(char *logFile, char *report) {

    packages_t * head = NULL;
    head=(packages_t *)malloc(sizeof(packages_t));


    int installedPackages=0;
    int deletedPackages=0;
    int upradedPackages=0;
    int currentInstalled=0;

    char oldestPackage[30];
    bool isOldestPackageAsign=false;
    char newestPackage[30];


    int PACMAN=0;
    int ALPM=0;
    int ALPMSCRIPTTLET=0;

    printf("Generating Report from: [%s] log file\n", logFile);
    
    // Implement your solution here.
    //open file 
    FILE * fp;
    char * linea = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(logFile, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    //manejo de linea 
    //stringtokenizer, i hate string tokenizer but there is not split in c 

    while (read = getline(&linea, &len, fp) != -1) {        
        //manejo de la linea 
        char *lineaChar[100];
        int i=0;

        char *stringTokenizer = strtok(linea," ");
        while (stringTokenizer != NULL)
        {
            lineaChar[i]=stringTokenizer;
            i++;
            stringTokenizer=strtok(NULL," ");
        }
       //pacman2
        //lineaChar posiciones
        //[0]= fecha
        //[1]=[ALPM-SCRIPTTLET], [ALPM],[PACMAN]
        //[2]=INSTALLED, REMOVED, UPGRADED, 
        //[3]=package name


        if(strcmp(logFile,"pacman.txt")==0){
            //installed, removed or upgraded         
            if(strcmp(lineaChar[2+1],"installed")==0){
                if(!isOldestPackageAsign){
                    strcpy(oldestPackage,lineaChar[4]);
                    isOldestPackageAsign=true;
                }
                strcpy(newestPackage,lineaChar[4]);
                
                installedPackages++;
                currentInstalled++;
                push(head,lineaChar[4],strcat(lineaChar[0],lineaChar[1]));
            }else if(strcmp(lineaChar[2+1],"reinstalled")==0){
                strcpy(newestPackage,lineaChar[4]);
                
                //installedPackages++;
                //currentInstalled++;
                push(head,lineaChar[4],strcat(lineaChar[0],lineaChar[1]));
            }else if(strcmp(lineaChar[2+1],"upgraded")==0){
                
                upradedPackages++;
                actualizarUpgraded(head,lineaChar[4],strcat(lineaChar[0],lineaChar[1]));

            }else if(strcmp(lineaChar[2+1],"removed")==0){                    
                
                eliminarPaquete(head,lineaChar[4],strcat(lineaChar[0],lineaChar[1]));
                deletedPackages++;
                currentInstalled--;
            }
            //tipo de paquete
            if(strcmp(lineaChar[1+1],"[PACMAN]")==0){
                PACMAN++;

            }else if(strcmp(lineaChar[1+1],"[ALPM]")==0){
                ALPM++;

            }else if(strcmp(lineaChar[1+1],"[ALPM-SCRIPTLET]")==0){
                ALPMSCRIPTTLET++;
            }
        }else{
            //installed, removed or upgraded         
            if(strcmp(lineaChar[2],"installed")==0){
                if(!isOldestPackageAsign){
                    strcpy(oldestPackage,lineaChar[3]);
                    isOldestPackageAsign=true;
                }
                strcpy(newestPackage,lineaChar[3]);
                
                
                installedPackages++;
                currentInstalled++;
                push(head,lineaChar[3],lineaChar[0]);
            }else if(strcmp(lineaChar[2],"reinstalled")==0){
                strcpy(newestPackage,lineaChar[3]);
                //installedPackages++;
                //currentInstalled++;
                push(head,lineaChar[3],lineaChar[0]);
            }else if(strcmp(lineaChar[2],"upgraded")==0){
                //printf("Upgraded\n");
                upradedPackages++;
                actualizarUpgraded(head,lineaChar[3],lineaChar[0]);
            }else if(strcmp(lineaChar[2],"removed")==0){
                //printf("Removed\n");
                eliminarPaquete(head,lineaChar[4],lineaChar[0]);
                deletedPackages++;
                currentInstalled--;
            }
            //tipo de paquete
            if(strcmp(lineaChar[1],"[PACMAN]")==0){
                PACMAN++;
            }else if(strcmp(lineaChar[1],"[ALPM]")==0){
                ALPM++;
            }else if(strcmp(lineaChar[1],"[ALPM-SCRIPTLET]")==0){
                ALPMSCRIPTTLET++;
            }
        }
        


        //garbage collector 
        memcpy(linea,lineaChar,sizeof(linea));
     
    }
    
    //prueba paquetes 
    /*
    packages_t * recorrido = head;
    printf("Packages with no updates: ");
    while (recorrido != NULL) {
         
        if(recorrido->howManyUpdates==0){
            
            printf("%s, ", recorrido->name);            
        }
        recorrido=recorrido->next;
    }
    printf("\n");
    */
    /*
    printf("----------------------------------------------\n");
    printf("Installed             : %i\n",installedPackages);
    printf("Deleted               : %i\n",deletedPackages);
    printf("Currentlly Installed  : %i\n",currentInstalled);
    printf("Upgraded              : %i\n",upradedPackages);
    printf("----------------------------------------------\n");
    printf("General Stats\n");
    printf("----------------------------------------------\n");
    printf("Oldest package        : %s\n",oldestPackage);
    printf("Newsest package       : %s\n",newestPackage);
    
    printf("[ALPMSCRIPLET] count  : %i\n",ALPMSCRIPTTLET);
    printf("[ALPM]                : %i\n",ALPM);
    printf("[PACMAN] count        : %i\n",PACMAN);
    printf("Upgraded              : %i\n",upradedPackages);
    printf("----------------------------------------------\n");
    printf("List of packages\n");
    printf("----------------------------------------------\n");*/
    //print_list(head);
    fclose(fp);
    if (linea)
        free(linea);
    //

    //imprimir
    printf("Report is generated at: [%s]\n\n", report);    
    FILE *fptr;
    fptr = fopen(report,"w");
    if(fptr == NULL)
    {
      printf("Error!");   
      exit(1);             
    }

    fprintf(fptr,"%s\n",logFile);
    fprintf(fptr,"----------------------------------------------\n");
    fprintf(fptr,"Installed             : %i\n",installedPackages);
    fprintf(fptr,"Deleted               : %i\n",deletedPackages);
    fprintf(fptr,"Currentlly Installed  : %i\n",currentInstalled);
    fprintf(fptr,"Upgraded              : %i\n",upradedPackages);
    fprintf(fptr,"----------------------------------------------\n");
    fprintf(fptr,"General Stats\n");
    fprintf(fptr,"----------------------------------------------\n");
    fprintf(fptr,"Oldest package        : %s\n",oldestPackage);
    fprintf(fptr,"Newsest package       : %s\n",newestPackage);
   

   packages_t * recorrido = head;
    fprintf(fptr,"Packages with no updates: ");
    while (recorrido != NULL) {
         
        if(recorrido->howManyUpdates==0){
            
            fprintf(fptr,"%s, ", recorrido->name);            
        }
        recorrido=recorrido->next;
    } 
    //231
    fprintf(fptr,"[ALPMSCRIPLET] count  : %i\n",ALPMSCRIPTTLET);
    fprintf(fptr,"[ALPM]                : %i\n",ALPM);
    fprintf(fptr,"[PACMAN] count        : %i\n",PACMAN);
    fprintf(fptr,"Upgraded              : %i\n",upradedPackages);
    fprintf(fptr,"----------------------------------------------\n");
    fprintf(fptr,"List of packages\n");
    fprintf(fptr,"----------------------------------------------\n");

    packages_t * current = head;

    while (current != NULL) {
         fprintf(fptr,"- Package Name:            %s\n    -Install date         %s\n    -Last update          %s\n    -How many updates      %i\n    -Removal date         %s\n",current->name,current->installDate,current->lastUpdate,current->howManyUpdates,current->removalDate);
        current = current->next;
    }
  
    fclose(fptr);
   
    exit(EXIT_SUCCESS);
//   return 0;
}
