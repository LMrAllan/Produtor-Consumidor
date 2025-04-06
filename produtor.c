#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "shm_com.h"

// Função para "fechar" o semáforo 
void down(int semid, int sem_num) {
    struct sembuf op = {sem_num, -1, 0};
    semop(semid, &op, 1);
}

// Função para "abrir" o semáforo 
void up(int semid, int sem_num) {
    struct sembuf op = {sem_num, 1, 0};
    semop(semid, &op, 1);
}

int main() {
    int executando = 1;  // Controla o loop principal
    void *memoria_compartilhada;  // Ponteiro para a memória compartilhada
    struct estrutura_compartilhada *dados;  // Dados compartilhados
    char buffer[BUFSIZ];  // Buffer para ler mensagens do teclado
    int id_shm, id_sem;   // Identificadores da memória e semáforos

    printf("\n=== PRODUTOR ===\n");
    printf("Digite mensagens para o consumidor.\n");
    printf("Comandos: 'fim' para terminar, 'status' para ver contagem\n\n");

    // Cria ou acessa a memória compartilhada
    id_shm = shmget(CHAVE_SHM, sizeof(struct estrutura_compartilhada), IPC_CREAT | 0666);
    if (id_shm == -1) {
        perror("Erro ao acessar memória compartilhada");
        exit(1);
    }

    // Conecta a memória compartilhada ao nosso programa
    memoria_compartilhada = shmat(id_shm, NULL, 0);
    dados = (struct estrutura_compartilhada *)memoria_compartilhada;

    // Cria ou acessa os semáforos
    id_sem = semget(CHAVE_SEM, 2, IPC_CREAT | 0666);
    if (id_sem == -1) {
        perror("Erro ao acessar semáforos");
        exit(1);
    }

    // Configura os semáforos iniciais se for a primeira execução
    if (semctl(id_sem, 0, GETVAL) == 0) {
        semctl(id_sem, 0, SETVAL, 1);  // Semáforo 0 começa em 1 (mutex)
        semctl(id_sem, 1, SETVAL, 0);  // Semáforo 1 começa em 0 (itens)
    }

    // Loop principal
    while(executando) {
        printf("Digite uma mensagem: ");
        fgets(buffer, BUFSIZ, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove o enter

        // Comando status
        if (strcmp(buffer, "status") == 0) {
            printf("Status: %d/%d mensagens consumidas\n", 
                  dados->contador_mensagens, MAX_MENSAGENS);
            continue;
        }

        // Entra na região crítica (trava o mutex)
        down(id_sem, 0);

        // Escreve a mensagem na memória compartilhada
        strncpy(dados->texto, buffer, TAM_TEXTO);
        dados->escrito = 1;  // Avisa que tem mensagem nova

        // Sai da região crítica (destrava o mutex)
        up(id_sem, 0);
        
        // Avisa que tem um novo item disponível
        up(id_sem, 1);

        // Comando fim
        if (strcmp(buffer, "fim") == 0) {
            executando = 0;
            printf("Finalizando produtor...\n");
        }
    }

    // Desconecta da memória compartilhada
    shmdt(memoria_compartilhada);
    printf("Produtor encerrado.\n");
    return 0;
}