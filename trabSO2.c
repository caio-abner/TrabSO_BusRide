#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#define MAX 10000

typedef struct psg{
    int id;                         //ondeEstou : 
    int pontoVaiSubir;              //    -1 = ponto de onibus inicial
    int pontoVaiDescer;             //    0  = onibus 0
    int ondeEstou;                  //    1  = onibus 1
                                    //    n  = onibus n
                                    //    -2 = chegou no destino
}passageiro;

typedef struct obs{                 //OndeEstou:
    int id;                         //    0  = ponto de onibus 0 
    sem_t acentosLivres;            //    n  = ponto de onibus n
    int ondeEstou;                  //    -1 = se locomovendo entre pontos                
}onibusSTC;
/*
Como nos exemplos do livro Sistemas Operacionais Modernos presentes nas figuras
2.28 e 2.32 e o exemplo "prodcons_1_thread_sem.c" fornecido pelo professor possuiem 
variaveris glabais optamos por seguir o mesmo padrao
*/
int s,c,p,a;
int idPassag    = 0;
int processados = 0;
int idOnibus    = 0;

sem_t semaforoPassageiro[MAX];
sem_t multex1,multex2,multex3;
passageiro listaPassageiros[MAX];
onibusSTC onibus[MAX]; 

passageiro produzPassageiro(){
    passageiro pas;
    pas.pontoVaiSubir = rand() % s;
    pas.pontoVaiDescer= rand() % s;
    pas.ondeEstou     = -1;
    sem_wait(&multex2); 
    pas.id = idPassag;
    idPassag++;
    sem_post(&multex2);
    //printf("pessoa id:%d inicio:%d fim:%d \n",pas.id,pas.pontoVaiSubir,pas.pontoVaiDescer);
    return pas;
}

void* funcPassageiro(void *arg){    // responsavel por produzir os passageiros
    //pessageiro é criado e inserido nos pontos
    passageiro pas;

    sem_wait(&multex1); 
    pas= produzPassageiro();        //Passageiro é produzido
    listaPassageiros[pas.id]=pas;   //Passageiro é incerido nos postos
    sem_post(&multex1);
    
    while (1)
    {
        if(sem_trywait(&semaforoPassageiro[pas.id])!= -1) break;
    }
    


    
    int aux;
    while (1)
    {
        if(listaPassageiros[pas.id].pontoVaiDescer == onibus[listaPassageiros[pas.id].ondeEstou].ondeEstou){
            aux = listaPassageiros[pas.id].ondeEstou;
            sem_wait(&multex1);
            listaPassageiros[pas.id].ondeEstou = -2;
            printf("passageiro %d desceu do onibus %d no pontpo %d\n",
            listaPassageiros[pas.id].id,
            onibus[aux].id,
            listaPassageiros[pas.id].pontoVaiDescer);
            sem_post(&multex1);
            sem_post(&onibus[aux].acentosLivres);printf("semaforo onibus %d ++\n",
            onibus[aux].id);
            break;
        }
    }
    /*
    printf("---------------------------\n");
    for(int i = 0; i<p;i++){
        printf("|id: %d local: %d|\n",listaPassageiros[i].id,listaPassageiros[i].ondeEstou);
    }printf("---------------------------\n");
    */

    printf("+++Fim thread do passageiro %d\n", pas.id);
    pthread_exit(NULL);
}

void* funcaoOnibus(void* arg){
    int aux = rand();
    int esseOnibus;
    //onibus onibus;
    sem_wait(&multex3);
    esseOnibus= idOnibus;
    idOnibus++;
    printf("*********thread do onibus %d\n", esseOnibus);
    sem_post(&multex3);  
    onibus[esseOnibus].id= esseOnibus;
    onibus[esseOnibus].ondeEstou = aux % s;
    sem_init(&onibus[esseOnibus].acentosLivres,0,a);
    
    while(processados<p){
        aux++;
        onibus[esseOnibus].ondeEstou = aux % s;
        if(sem_trywait(&arg[(aux%s)])){
            for(int i=0; i<p ;i++){
                if(listaPassageiros[i].ondeEstou == -1 && 
                    listaPassageiros[i].pontoVaiSubir == onibus[esseOnibus].ondeEstou){
                    sem_wait(&onibus[esseOnibus].acentosLivres);
                    listaPassageiros[i].ondeEstou = onibus[esseOnibus].id;
                    processados++;
                    sem_post(&semaforoPassageiro[i]);
                    printf("passageiro %d embarcou no onibus %d\n",i,esseOnibus);
                }
            }
            onibus[esseOnibus].ondeEstou = -2;
            sem_post(&arg[aux%s]);       
        }  
    }
  
    printf("Fim do onibus %d\n", onibus[esseOnibus].id);
    pthread_exit(NULL);
}

int main(){
    srand((unsigned int)time(NULL));
    //Declarando Variaveis  
    printf("Digite a quantidade de pontos de onibus: ");
    scanf("%d %d %d %d", &s,&c,&p,&a);
    printf("s:%d    c:%d    p:%d    a:%d\n",s,c,p,a);
    sem_init(&multex1,0,1);
    sem_init(&multex2,0,1);
    sem_init(&multex3,0,1);
    


//    pthread_t pontoDeOnibus[s];
    sem_t semaforoPontoDeOnibus[s];
    for(int i=0;i<s;i++){
        sem_init(&semaforoPontoDeOnibus[i],0,1);
    }

    pthread_t onibus[c];
    sem_t semaforoOnibus[c];
    for(int i=0;i<c;i++){
        sem_init(&semaforoOnibus[c],0,a);
    }

    pthread_t passageiros[p];
    //sem_t semaforoPassageiro[p];
    for(int i=0;i<p;i++){
        sem_init(&semaforoPassageiro[i],0,0);
    }

    int status;             
    //------------------------------------------------------------------------------------
    //Inicializando as Threads do Pontos de Onibus

    for(int i=0;i<p;i++){
        printf("Metodo Main. Criando passageiro %d\n", i);
        status =pthread_create(&passageiros[i], NULL, (void*)funcPassageiro, (void *) &semaforoPassageiro);
        if (status != 0) {
            printf("Oops. pthread create returned error code %d\n", status);
            exit(-1);
        }
    }   
   

    printf("-------------------------------------------------------------------------------------------\n"); 
    //---------------------------------------------
   //Inicializando as Threads do Onibus

    for(int i=0;i<c;i++){
        printf("Metodo Main. Criando onibus %d\n", i);
        status =pthread_create(&onibus[i], NULL, (void*)funcaoOnibus,(void*) &semaforoPontoDeOnibus );
        if (status != 0) {
            printf("Oops. pthread create returned error code %d\n", status);
            exit(-1);
        }
    }

    for(int i=0;i<c;i++){
        pthread_join(onibus[i], NULL);
    } 

    for(int i = 0; i<p;i++){
        printf("id: %d local: %d\n",listaPassageiros[i].id,listaPassageiros[i].ondeEstou);
    }
 

    return 0;
}