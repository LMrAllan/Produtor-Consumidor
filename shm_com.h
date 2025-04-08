#ifndef SHM_COM_H
#define SHM_COM_H

#include <sys/types.h>
#include <sys/ipc.h>

// Tamanho máximo de cada mensagem
#define TAM_TEXTO 2048     

// Capacidade do buffer circular
#define TAM_BUFFER 15 

// Limite total de mensagens a serem processadas
#define MAX_MENSAGENS 15  

// Chave para a memória compartilhada
#define CHAVE_SHM 0x1234    

// Chave para os semáforos
#define CHAVE_SEM 0x1235     

// Estrutura compartilhada entre produtor e consumidor
struct estrutura_compartilhada {
    int indice_produtor;      // Posição onde o produtor irá inserir a próxima mensagem
    int indice_consumidor;    // Posição de onde o consumidor irá ler a próxima mensagem

    int contador_mensagens;   // Total de mensagens já consumidas 
    int mensagens_no_buffer;  // Quantidade atual de mensagens no buffer
    char buffer[TAM_BUFFER][TAM_TEXTO]; // Buffer circular para armazenar mensagens
};

#endif