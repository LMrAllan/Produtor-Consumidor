#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "shm_com.h"

void down(int semid, int sem_num) {
    struct sembuf op = {sem_num, -1, 0};
    semop(semid, &op, 1);
}

void up(int semid, int sem_num) {
    struct sembuf op = {sem_num, 1, 0};
    semop(semid, &op, 1);
}

int main() {
    int executando = 1;
    void *memoria_compartilhada;
    struct estrutura_compartilhada *dados;
    int id_shm, id_sem;

    printf("\n=== CONSUMIDOR ===\n");
    printf("Vai consumir até %d mensagens.\n", MAX_MENSAGENS);
    printf("Aguardando mensagens do produtor...\n\n");

    id_shm = shmget(CHAVE_SHM, sizeof(struct estrutura_compartilhada), 0666 | IPC_CREAT);
    if (id_shm == -1) {
        perror("Erro ao acessar memória compartilhada");
        exit(1);
    }

    memoria_compartilhada = shmat(id_shm, NULL, 0);
    dados = (struct estrutura_compartilhada *)memoria_compartilhada;

    id_sem = semget(CHAVE_SEM, 2, 0666 | IPC_CREAT);
    if (id_sem == -1) {
        perror("Erro ao acessar semáforos");
        exit(1);
    }

    while(executando && dados->contador_mensagens < MAX_MENSAGENS) {
        down(id_sem, 1);  // Espera por itens no buffer
        down(id_sem, 0);  // Bloqueia acesso ao buffer

        // Verifica se há mensagens para consumir
        if (dados->mensagens_no_buffer == 0) {
            up(id_sem, 0);  // Libera acesso ao buffer
            continue;
        }

        // Remove mensagem do buffer
        char mensagem[TAM_TEXTO];
        strncpy(mensagem, dados->buffer[dados->indice_consumidor], TAM_TEXTO);
        dados->indice_consumidor = (dados->indice_consumidor + 1) % TAM_BUFFER;
        dados->mensagens_no_buffer--;
        
        up(id_sem, 0);  // Libera acesso ao buffer

        // Processa a mensagem
        if (mensagem[0] == '\x01') { // Comando 'fim'
            executando = 0;
            printf("Consumidor encerrado pelo produtor.\n");
        } 
        else if (mensagem[0] == '\x02') { // Comando 'limpar'
            executando = 0;
            shmdt(memoria_compartilhada);
            shmctl(id_shm, IPC_RMID, NULL);
            semctl(id_sem, 0, IPC_RMID);
            printf("Consumidor encerrado (recursos liberados).\n");
        } 
        else { // Mensagem normal
            printf("Mensagem recebida: %s\n", mensagem);
            dados->contador_mensagens++;
            printf("Esperando mensagem... \n");
            sleep(1);
        }

        if (dados->contador_mensagens >= MAX_MENSAGENS) {
            printf("Limite de mensagens atingido!\n");
            executando = 0;
        }
    }

    shmdt(memoria_compartilhada);
    return 0;
}