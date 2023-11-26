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
    int flagEspera;                 //    n  = onibus n
                                    //    -2 = chegou no destino
}passageiro;

typedef struct obs{                 //OndeEstou:
    int id;                         //    0  = ponto de onibus 0 
    int ondeEstou;                  //    n  = ponto de onibus n
    int acetosLivres;              //    -1 = se locomovendo entre pontos 
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

sem_t semaforoPontoDeOnibus[MAX];
sem_t multex1,multex2,multex3,multex4,multex5;
passageiro listaPassageiros[MAX];
onibusSTC onibus[MAX]; 

void* funcPassageiro(void *arg){    // responsavel por produzir os passageiros
    //pessageiro é criado e inserido nos pontos
    int id;

    sem_wait(&multex2); 
    id = idPassag;
    idPassag++;
    sem_post(&multex2);

    sem_wait(&multex1);
    listaPassageiros[id].id = id;
    listaPassageiros[id].pontoVaiSubir = rand() % s;
    listaPassageiros[id].pontoVaiDescer= rand() % s;
    listaPassageiros[id].ondeEstou = -1;
    listaPassageiros[id].flagEspera = 0;
    sem_post(&multex1);
    
    while (1)
    {
        if(listaPassageiros[id].flagEspera) break;
    }
    
    int aux;
    while (1)
    {
        if(listaPassageiros[id].pontoVaiDescer == onibus[listaPassageiros[id].ondeEstou].ondeEstou){
            aux = listaPassageiros[id].ondeEstou;
            sem_wait(&multex1);
            listaPassageiros[id].ondeEstou = -2;
            sem_wait(&multex4);
            onibus[aux].acetosLivres++;
            processados++;
            printf("processados: %d\n",processados);
            printf("+++Fim thread do passageiro %d\n", id);
            sem_post(&multex4);
            sem_post(&multex1);
            break;
        }
    }
    pthread_exit(NULL);
}

void* funcaoOnibus(void* arg){
    int aux = rand();
    int esseOnibus;
    
    sem_wait(&multex3);
    esseOnibus= idOnibus;
    idOnibus++;
    sem_post(&multex3);  
    
    onibus[esseOnibus].id= esseOnibus;
    onibus[esseOnibus].ondeEstou = aux % s;
    onibus[esseOnibus].acetosLivres=a;
    
    while(1){
        aux++;
        onibus[esseOnibus].ondeEstou = aux % s;
        if(sem_trywait(&semaforoPontoDeOnibus[(aux%s)])){
            for(int i=0; i<p ;i++){
                sem_wait(&multex5);
                if(listaPassageiros[i].pontoVaiSubir == onibus[esseOnibus].ondeEstou &&
                listaPassageiros[i].ondeEstou == -1 &&
                onibus[esseOnibus].acetosLivres > 0){
                    sem_wait(&multex1);
                    listaPassageiros[i].flagEspera = 1;
                    sem_post(&multex1);
                    listaPassageiros[i].ondeEstou = esseOnibus;
                    onibus[esseOnibus].acetosLivres --;
                    printf("passageiro %d embarcou no onibus %d\n",i,esseOnibus);
                }
                sem_post(&multex5);
            }
            sem_post(&semaforoPontoDeOnibus[aux%s]);       
        } 
        if(processados >= p)break;
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
    sem_init(&multex4,0,1);
    sem_init(&multex5,0,1);
    pthread_t onibus[c];
    pthread_t passageiros[p];


//    pthread_t pontoDeOnibus[s];
    sem_t semaforoPontoDeOnibus[s];
    for(int i=0;i<s;i++){
        sem_init(&semaforoPontoDeOnibus[i],0,1);
    }

    int status;             
    //------------------------------------------------------------------------------------
    //Inicializando as Threads do Pontos de Onibus

    for(int i=0;i<p;i++){
        status =pthread_create(&passageiros[i], NULL, (void*)funcPassageiro, NULL);
        if (status != 0) {
            printf("Oops. pthread create returned error code %d\n", status);
            exit(-1);
        }
    }   
   

    printf("-------------------------------------------------------------------------------------------\n"); 
    //---------------------------------------------
   //Inicializando as Threads do Onibus

    for(int i=0;i<c;i++){
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