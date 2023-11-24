#include <stdlib.h>
#include <stdio.h>
#include <time.h>
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
/*
Como nos exemplos do livro Sistemas Operacionais Modernos presentes nas figuras
2.28 e 2.32 e o exemplo "prodcons_1_thread_sem.c" fornecido pelo professor possuiem 
variaveris glabais optamos por seguir o mesmo padrao
*/
int s,c,p,a;
int idPassag    = 0;
int processados = 0;
int idOnibus    = 0;

sem_t multex1,multex2,multex3;
passageiro listaPassageiros[MAX]; 

passageiro produzPassageiro(){
    printf("produzindo passagerio %d\n",idPassag);
    passageiro pas;
    pas.pontoVaiSubir = rand() % s;
    pas.pontoVaiDescer= rand() % s;
    pas.ondeEstou     = -1;
    sem_wait(&multex2); 
    pas.id = idPassag;
    idPassag++;
    sem_post(&multex2);
    printf("pessoa id:%d inicio:%d fim:%d \n",pas.id,pas.pontoVaiSubir,pas.pontoVaiDescer);
    return pas;
}

void* ponto(void *arg){ // responsavel por produzir os passageiros
    printf("thread do ponto %d\n", arg);
    passageiro pas;

    while ( idPassag<p ) {
        pas= produzPassageiro();
        sem_wait(&multex1); 
        listaPassageiros[pas.id]=pas;
        sem_post(&multex1);
    }

    pthread_exit(NULL);
}

void* funcaoOnibus(void* arg){
    int ponto,aux = rand();
    sem_wait(&multex3);
    int idDesseOnibus = idOnibus;
    idOnibus++;
    printf("thread do onibus %d\n", idDesseOnibus);
    sem_post(&multex3);
    int acentos = a;
    while(processados<p){
        ponto = aux % s;
        if(sem_trywait(&arg[ponto])){
            sem_wait(&multex1); 
            for(int i=0; i<p && acentos>0;i++){
                if(listaPassageiros[i].ondeEstou == -1 && listaPassageiros[i].pontoVaiSubir == ponto){
                    listaPassageiros[i].ondeEstou = idDesseOnibus;
                    processados++;
                    printf("passageiro %d embarcou no onibus %d\n",i,idDesseOnibus);
                }
            }
            sem_post(&multex1);
            sem_post(&arg[ponto]);       
        }
        aux++;
    }
    printf("Fim do onibus %d\n", idDesseOnibus);
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


    pthread_t pontoDeOnibus[s];
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
    sem_t semaforoPassageiro;
    sem_init(&semaforoPassageiro,0,p);
    

    int status;             
    //------------------------------------------------------------------------------------
    //Inicializando as Threads do Pontos de Onibus

    for(int i=0;i<s;i++){
        printf("Metodo Main. Criando ponto de onibus %d\n", i);
        status =pthread_create(&pontoDeOnibus[i], NULL, (void*)ponto, (void *) i);
        if (status != 0) {
            printf("Oops. pthread create returned error code %d\n", status);
            exit(-1);
        }
    }
    
    for(int i=0;i<s;i++){
        pthread_join(pontoDeOnibus[i], NULL);
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