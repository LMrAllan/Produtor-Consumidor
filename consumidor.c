#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "shm_com.h"

// Funções down e up 
void down(int semid, int sem_num) {
    struct sembuf op = {sem_num, -1, 0};
    semop(semid, &op, 1);
}

void up(int semid, int sem_num) {
    struct sembuf op = {sem_num, 1, 0};
    semop(semid, &op, 1);
}

int main() {
    int executando = 1;  // Controla o loop principal
    void *memoria_compartilhada;  // Ponteiro para a memória compartilhada
    struct estrutura_compartilhada *dados;  // Dados compartilhados
    int id_shm, id_sem;  // Identificadores da memória e semáforos

    printf("\n=== CONSUMIDOR ===\n");
    printf("Vai consumir até %d mensagens.\n", MAX_MENSAGENS);
    printf("Aguardando mensagens do produtor...\n\n");

    // Acessa a memória compartilhada
    id_shm = shmget(CHAVE_SHM, sizeof(struct estrutura_compartilhada), 0666 | IPC_CREAT);
    if (id_shm == -1) {
        perror("Erro ao acessar memória compartilhada");
        exit(1);
    }

    // Conecta a memória compartilhada
    memoria_compartilhada = shmat(id_shm, NULL, 0);
    dados = (struct estrutura_compartilhada *)memoria_compartilhada;

    // Acessa os semáforos 
    id_sem = semget(CHAVE_SEM, 2, 0666 | IPC_CREAT);
    if (id_sem == -1) {
        perror("Erro ao acessar semáforos");
        exit(1);
    }

    // Loop principal
    while(executando && dados->contador_mensagens < MAX_MENSAGENS) {
        // Espera até ter uma mensagem disponível
        down(id_sem, 1);
        
        // Entra na região crítica (trava o mutex)
        down(id_sem, 0);

        // Mostra a mensagem recebida
        printf("Mensagem recebida: %s\n", dados->texto);
        dados->escrito = 0;  // Marca como lida
        dados->contador_mensagens++;  // Conta mais uma mensagem

        // Sai da região crítica (destrava o mutex)
        up(id_sem, 0);

        // Verifica se recebeu comando para terminar
        if (strcmp(dados->texto, "fim") == 0) {
            executando = 0;
        }

        // Verifica se atingiu o limite de mensagens
        if (dados->contador_mensagens >= MAX_MENSAGENS) {
            printf("Limite de %d mensagens atingido!\n", MAX_MENSAGENS);
            executando = 0;
        }
    }

    // Desconecta da memória compartilhada
    shmdt(memoria_compartilhada);
    
    // Se for o último consumidor, limpa os recursos
    if (dados->contador_mensagens >= MAX_MENSAGENS || 
        strcmp(dados->texto, "fim") == 0) {
    }

    printf("Consumidor encerrado.\n");
    return 0;
}