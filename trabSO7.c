//Primeiro Trabalho Pratico de Sistemas Operacionais I (2023) - USP/ICMC
//Integrantes do Grupo 03:
//  - Caio Abner Soares Araujo (4822220)
//  - Heitor Pupim Assunção Toledo (11372858)
//  - Pedro Henrique Cruz da Silva (11833236)
//  - Pedro Santos Souza (12567502)
//  - Pedro Zambrozi Brunhara (12608664)

//Para compilar o programa: gcc trabSO7.c -o trabSO7 -pthread
//Para executar o programa: ./trabSO7

//Para executar o programa, digite o número de pontos de ônibus (S), o número de ônibus (C), o número de passageiros (P)
//e o número de assentos em cada ônibus (A) no formato "S C P A"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#define MAX 1000000

typedef struct psg{
    int id ;                         //ondeEstou : 
    int pontoVaiSubir;              //    -1 = ponto de ônibus inicial
    int pontoVaiDescer;             //    0  = ônibus 0
    int ondeEstou;                  //    1  = ônibus 1
    int flagEspera;                 //    n  = ônibus n
    int flagCriado;                 //    -2 = chegou no destino
    int onibus;
    time_t tempoChegadaPonto;
    time_t tempoEntradaBus;
    time_t tempoDescidaBus;
}passageiro;

typedef struct obs{                 //OndeEstou:
    int id;                         //    0  = ponto de ônibus 0
    int ondeEstou;                  //    n  = ponto de ônibus n
    int acetosLivres;              //    -1 = se locomovendo entre pontos 
}onibusSTC;
/*
Como os exemplos do livro Sistemas Operacionais Modernos (presentes nas figuras
2.28 e 2.32) e o exemplo "prodcons_1_thread_sem.c" fornecido pelo professor possuem
variáveis globais, optamos por seguir o mesmo padrão.
*/
int s,c,p,a;
int idPassag    = 0;
int processados = 0;
int idOnibus    = 0;

sem_t semaforoPontoDeOnibus[MAX];
sem_t multex1,multex2,multex3,multex4,multex5;
passageiro* listaPassageiros;
onibusSTC* onibus; 
time_t inicio;

//função pra gerar arquivo trace de cada passageiro, contendo o horário de chegada no ponto,
//horário de entrada/saida do ônibus e o ponto de destino
void produzOutput(passageiro psg){
    sem_wait(&multex2);
    sem_wait(&multex1);
    sem_wait(&multex3);
    sem_wait(&multex5);

    FILE *tracePsg;
    double tChegada,tEmbarq,TDesembarq;
    tChegada = difftime(psg.tempoChegadaPonto,inicio);
    tEmbarq = difftime(psg.tempoEntradaBus,inicio);
    TDesembarq = difftime(psg.tempoDescidaBus,inicio);
    
    char nomeArquivo[50];
    sprintf(nomeArquivo, "passageiro%d.trace", psg.id);
    
    tracePsg = fopen(nomeArquivo,"w");
    if (tracePsg == NULL){
      exit (0);
    }
    fprintf(tracePsg, 
        "Horário que chegou no ponto de origem %d: %.4f\nHorário que entrou no ônibus %d: %.4f\nHorário que desceu do ônibus %d: %.4f\nPonto de destino que desceu: %d\n",
        psg.pontoVaiSubir,tChegada,psg.onibus,tEmbarq,psg.onibus,TDesembarq,psg.pontoVaiDescer);
    fclose(tracePsg);

    sem_post(&multex5);
    sem_post(&multex3);
    sem_post(&multex1);
    sem_post(&multex2);
}

void* funcAnimacao(void* arg){
    int count =0;
    while (1)
    {
        //pega os semáforos para parar o código e imprimir o estado
        //dos passageiros e dos ônibus
        sem_wait(&multex2); 
        sem_wait(&multex1);
        sem_wait(&multex3);
        sem_wait(&multex5);

        printf("Passageiros--------------------------------------------------------\n");
        for(int i=0;i<idPassag;i++){
            
            if(listaPassageiros[i].flagCriado == 0)continue;
            printf("Passageiro %d ",listaPassageiros[i].id);
            if(listaPassageiros[i].ondeEstou == -1) 
            printf("esta esperando o onibus no ponto %d  "
            ,listaPassageiros[i].pontoVaiSubir);
            if(listaPassageiros[i].ondeEstou == -2)
            printf("desembarcou no ponto de onibus   %d  "
            ,listaPassageiros[i].pontoVaiDescer);
            if(listaPassageiros[i].ondeEstou > -1)
            printf("esta dentro do onibus            %d  "
            ,listaPassageiros[i].ondeEstou);
            count++;
            if(count % 3 == 0){
                printf("\n");
                usleep(90000);
            }
        }
        printf("\n-------------------------------------------------------------------\n");
        printf("Onibus--------------------------------------------------------\n");
        for(int i=0;i<idOnibus;i++){
            printf("Onibus %d esta passando no ponto %d\n",
            onibus[i].id,onibus[i].ondeEstou);
            usleep(90000);
        }
        printf("\n-------------------------------------------------------------------\n");
        
        //libera os semáforos para o código voltar a executar
        sem_post(&multex5);
        sem_post(&multex3);
        sem_post(&multex1);
        sem_post(&multex2);
        usleep(70000);
        if(processados >= p)break; //condição de saída do loop
    }
    pthread_exit(NULL);
}

//função responsável por produzir os passageiros
void* funcPassageiro(void *arg){
    //passageiro é criado e inserido nos pontos
    int id;
    time_t tempo;
    //garante que 2 passageiros não tenham o mesmo id
    sem_wait(&multex2);  
    id = idPassag;
    idPassag++;
    sem_post(&multex2);

    //protege o array de passageiros
    sem_wait(&multex1);
    listaPassageiros[id].id = id;
    listaPassageiros[id].pontoVaiSubir = rand() % s;
    time(&listaPassageiros[id].tempoChegadaPonto);
    listaPassageiros[id].pontoVaiDescer= rand() % s;
    listaPassageiros[id].ondeEstou = -1;
    listaPassageiros[id].flagEspera = 0;
    listaPassageiros[id].flagCriado = 1;
    sem_post(&multex1);
    
    while (1) //espera ocupada
    {
        if(listaPassageiros[id].flagEspera) break;
    }
    
    int aux;
    while (1)
    {   
        //verifica se o ônibus esta onde o passageiro vai descer
        if(listaPassageiros[id].pontoVaiDescer == onibus[listaPassageiros[id].ondeEstou].ondeEstou){
            aux = listaPassageiros[id].ondeEstou;
            sem_wait(&multex1); //protege o array de passageiros
            listaPassageiros[id].ondeEstou = -2; //flag indicando que o passageiro desceu do ônibus
            time(&listaPassageiros[id].tempoDescidaBus);
            sem_wait(&multex4);//protege o número de assentos livres do ônibus
            onibus[aux].acetosLivres++;
            processados++; //número de passageiros que desceram do ônibus
            sem_post(&multex4);
            sem_post(&multex1);
            break;
        }
    }
    
    produzOutput(listaPassageiros[id]);
    usleep(rand());
    pthread_exit(NULL);
}

void* funcaoOnibus(void* arg){
    int aux = rand();
    int esseOnibus;
    clock_t tempo;
    //garante que 2 ônibus não tenham o mesmo id
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
        if(sem_trywait(&semaforoPontoDeOnibus[(aux%s)])){//tenta entrar no ponto de ônibus
            for(int i=0; i<p ;i++){
                //garante que apenas um passageiro por vez irá tentar embarcar
                sem_wait(&multex5);
                if(listaPassageiros[i].pontoVaiSubir == onibus[esseOnibus].ondeEstou &&
                listaPassageiros[i].ondeEstou == -1 &&
                onibus[esseOnibus].acetosLivres > 0){
                    listaPassageiros[i].flagEspera = 1;
                    listaPassageiros[i].ondeEstou = esseOnibus;
                    listaPassageiros[i].onibus = esseOnibus;
                    time(&listaPassageiros[i].tempoEntradaBus);
                    onibus[esseOnibus].acetosLivres --;
                }
                sem_post(&multex5);
            }
            sem_post(&semaforoPontoDeOnibus[aux%s]);
            //libera o semáforo do ponto de ônibus
        } 
        if(processados >= p)break;
    }
    pthread_exit(NULL);
}

int main(){
    srand((unsigned int)time(NULL));
    time(&inicio);
    //Declarando Variáveis---------------
    int aux; 
    aux = scanf("%d %d %d %d", &s,&c,&p,&a);
    if (aux != 4){  //testando se a entrada é valida
      printf("Erro ao ler valores!");
      return 0;
    }
    if (!(p>a) && !(a>c)){//testando se a entrada é valida
      printf("Erro ao ler valores!");
      return 0;
    }

    //inicializando os multex
    sem_init(&multex1,0,1);
    sem_init(&multex2,0,1);
    sem_init(&multex3,0,1);
    sem_init(&multex4,0,1);
    sem_init(&multex5,0,1);

    listaPassageiros = (passageiro*) malloc(p*sizeof(passageiro));
    onibus =(onibusSTC*)malloc(c*sizeof(onibus));
    pthread_t onibus[c];
    pthread_t passageiros[p];
    pthread_t animacao;

    for(int i=0;i<s;i++){
        sem_init(&semaforoPontoDeOnibus[i],0,1);
    }

    int status;             
    //------------------------------------------------------------------------------------
    //Inicializando as Threads do Passageiro

    for(int i=0;i<p;i++){
        status =pthread_create(&passageiros[i], NULL, (void*)funcPassageiro, NULL);
        if (status != 0) {
            printf("Oops. pthread create returned error code %d\n", status);
            exit(-1);
        }
    }

   //------------------------------------------------------------------------------------
   //Inicializando as Threads do Ônibus

    for(int i=0;i<c;i++){
        status =pthread_create(&onibus[i], NULL, (void*)funcaoOnibus,(void*) &semaforoPontoDeOnibus );
        if (status != 0) {
            printf("Oops. pthread create returned error code %d\n", status);
            exit(-1);
        }
    }

    //------------------------------------------------------------------------------------
    //Inicializando as Threads da animação--------------------------------------

    status =pthread_create(&animacao, NULL, (void*)funcAnimacao,(void*) &semaforoPontoDeOnibus );
    if (status != 0) {
        printf("Oops. pthread create returned error code %d\n", status);
        exit(-1);
    }

   //------------------------------------------------------------------------------------


    for(int i=0;i<c;i++){
        pthread_join(onibus[i], NULL);
    }

    return 0;
}