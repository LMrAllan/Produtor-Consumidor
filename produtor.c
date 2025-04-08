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
    char buffer[BUFSIZ];
    int id_shm, id_sem;

    printf("\n=== PRODUTOR ===\n");
    printf("Digite mensagens para o consumidor.\n");
    printf("Comandos:\n");
    printf("  'status' - Mostra quantas mensagens foram consumidas\n");
    printf("  'fim'    - Encerra ambos os processos\n");
    printf("  'limpar' - Encerra ambos e libera recursos\n\n");

    id_shm = shmget(CHAVE_SHM, sizeof(struct estrutura_compartilhada), IPC_CREAT | 0666);
    if (id_shm == -1) {
        perror("Erro ao acessar memória compartilhada");
        exit(1);
    }

    memoria_compartilhada = shmat(id_shm, NULL, 0);
    dados = (struct estrutura_compartilhada *)memoria_compartilhada;

    id_sem = semget(CHAVE_SEM, 2, IPC_CREAT | 0666);
    if (id_sem == -1) {
        perror("Erro ao acessar semáforos");
        exit(1);
    }

    // Inicialização dos semáforos
    if (semctl(id_sem, 0, GETVAL) == 0) {  // Semáforo 0: controle de acesso
        semctl(id_sem, 0, SETVAL, 1);
    }
    if (semctl(id_sem, 1, GETVAL) == 0) {  // Semáforo 1: itens no buffer
        semctl(id_sem, 1, SETVAL, 0);
    }

    // Inicialização dos dados compartilhados
    if (dados->contador_mensagens == 0) {
        dados->indice_produtor = 0;
        dados->indice_consumidor = 0;
        dados->contador_mensagens = 0;
        dados->mensagens_no_buffer = 0;
    }

    while(executando) {
        printf("Digite uma mensagem: ");
        fgets(buffer, BUFSIZ, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcmp(buffer, "status") == 0) {
            printf("Status: %d/%d mensagens consumidas (Buffer: %d/%d)\n", 
                  dados->contador_mensagens, MAX_MENSAGENS, 
                  dados->mensagens_no_buffer, TAM_BUFFER);
            continue;
        }

        down(id_sem, 0);  // Bloqueia acesso ao buffer

        if (strcmp(buffer, "fim") == 0 || strcmp(buffer, "limpar") == 0) {
            // Insere comando especial no buffer
            strncpy(dados->buffer[dados->indice_produtor], 
                   (strcmp(buffer, "fim") == 0) ? "\x01" : "\x02", TAM_TEXTO);
            dados->indice_produtor = (dados->indice_produtor + 1) % TAM_BUFFER;
            dados->mensagens_no_buffer++;
            
            up(id_sem, 0);  // Libera acesso ao buffer
            up(id_sem, 1);  // Sinaliza que há novo item

            if (strcmp(buffer, "limpar") == 0) {
                shmdt(memoria_compartilhada);
                shmctl(id_shm, IPC_RMID, NULL);
                semctl(id_sem, 0, IPC_RMID);
                printf("Recursos liberados.\n");
            }

            executando = 0;
            continue;
        }

        if (dados->mensagens_no_buffer == TAM_BUFFER) {
            printf("Buffer cheio! Aguarde o consumidor processar algumas mensagens.\n");
            up(id_sem, 0);  // Libera acesso ao buffer antes de esperar
            sleep(1);
            continue;
        }

        // Insere mensagem no buffer
        strncpy(dados->buffer[dados->indice_produtor], buffer, TAM_TEXTO);
        dados->indice_produtor = (dados->indice_produtor + 1) % TAM_BUFFER;
        dados->mensagens_no_buffer++;
        
        up(id_sem, 0);  // Libera acesso ao buffer
        up(id_sem, 1);  // Sinaliza que há novo item
    }

    shmdt(memoria_compartilhada);
    printf("Produtor encerrado.\n");
    return 0;
}