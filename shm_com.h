#ifndef SHM_COM_H
#define SHM_COM_H

#include <sys/types.h>
#include <sys/ipc.h>

// Tamanho máximo para o texto das mensagens
#define TAM_TEXTO 2048

// Número máximo de mensagens que o consumidor vai processar
#define MAX_MENSAGENS 15

// Chaves fixas para identificar a memória compartilhada e os semáforos
#define CHAVE_SHM 0x1234  // Memória compartilhada
#define CHAVE_SEM 0x1235  // Semáforos

// Comandos para gerenciamento:
// Ver recursos IPC: ipcs -a
// Limpar recursos: ipcrm -a

// Estrutura que será compartilhada entre os processos
struct estrutura_compartilhada {
    int escrito;    // 1 = tem mensagem nova, 0 = mensagem já lida
    int contador_mensagens;  // Quantas mensagens já foram processadas
    char texto[TAM_TEXTO];   // Onde a mensagem é armazenada
};

#endif