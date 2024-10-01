// servidor.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define LOG_FILE "server_log.txt" // Nome do arquivo de log
#define MAX_LOG_ENTRY 2048 // Tamanho máximo da entrada de log

// Variável global para controlar o número da próxima tarefa
int next_task_number = 1;

// Mutex para proteger o acesso à variável next_task_number
pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;

// Mutex para proteger o acesso ao arquivo de log
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Função envelopadora para criar o socket conforme recomendado
int Socket(int family, int type, int protocol) {
    int sockfd;
    if ((sockfd = socket(family, type, protocol)) < 0) {
        perror("socket");
        exit(1);
    }
    return sockfd;
}

// Função para registrar logs no arquivo com proteção de mutex
void log_message(const char *message) {
    pthread_mutex_lock(&log_mutex);
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        perror("Erro ao abrir o arquivo de log");
        pthread_mutex_unlock(&log_mutex);
        exit(1);
    }

    // Obter o timestamp atual
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[22];
    strftime(timestamp, sizeof(timestamp), "[%d/%m/%Y %H:%M:%S]", t);

    // Escrever o timestamp e a mensagem no log
    fprintf(log_file, "%s %s\n", timestamp, message);
    fclose(log_file);
    pthread_mutex_unlock(&log_mutex);
}

// Função para lidar com o cliente
void* handle_client(void* client_socket_ptr) {
    int client_socket = *((int*)client_socket_ptr);
    free(client_socket_ptr);  // Liberar a memória alocada para o ponteiro do socket
    char buffer[BUFFER_SIZE];
    char task[BUFFER_SIZE];
    char log_entry[MAX_LOG_ENTRY];
    int task_number;

    // Pegar informações sobre o cliente
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    if (getpeername(client_socket, (struct sockaddr *)&client_addr, &addr_len) < 0) {
        perror("getpeername");
        close(client_socket);
        pthread_exit(NULL);
    }

    // Gravar log de conexão com o cliente e seu endereço IP
    snprintf(log_entry, sizeof(log_entry), "Conexão estabelecida com o cliente %s:%d.", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    log_message(log_entry);
    printf("Cliente conectado: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Loop para enviar múltiplas tarefas
    while (1) {
        pthread_mutex_lock(&task_mutex); // Bloquear o mutex
        task_number = next_task_number;
        next_task_number++;
        pthread_mutex_unlock(&task_mutex); // Desbloquear o mutex

        // Condição para encerrar a conexão após enviar 15 tarefas
        if (task_number > 15) {
            char *end_message = "ENCERRAR";
            if (send(client_socket, end_message, strlen(end_message), 0) < 0) {
                perror("send");
                snprintf(log_entry, sizeof(log_entry), "Falha ao enviar instrução de encerramento para o cliente %s:%d.", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                log_message(log_entry);
                break;
            }

            snprintf(log_entry, sizeof(log_entry), "Instrução de encerramento enviada ao cliente %s:%d.", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            log_message(log_entry);
            printf("Instrução de encerramento enviada para %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            break;
        }

        // Definir a tarefa a ser enviada
        snprintf(task, sizeof(task), "TAREFA %d: LIMPEZA", task_number);

        // Enviar tarefa ao cliente
        if (send(client_socket, task, strlen(task), 0) < 0) {
            perror("send");
            snprintf(log_entry, sizeof(log_entry), "Falha ao enviar tarefa %d para o cliente %s:%d.", task_number, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            log_message(log_entry);
            break;
        }

        // Registrar a tarefa enviada ao cliente específico no log
        snprintf(log_entry, sizeof(log_entry), "Tarefa %d enviada ao cliente %s:%d.", task_number, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        log_message(log_entry);
        printf("Tarefa %d enviada para %s:%d\n", task_number, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Receber resposta do cliente
        memset(buffer, 0, BUFFER_SIZE);
        int received_bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (received_bytes < 0) {
            perror("recv");
            snprintf(log_entry, sizeof(log_entry), "Erro ao receber resposta do cliente %s:%d.", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            log_message(log_entry);
            break;
        } else if (received_bytes == 0) {
            snprintf(log_entry, sizeof(log_entry), "Cliente %s:%d desconectou.", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            log_message(log_entry);
            printf("Cliente %s:%d desconectou.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            break;
        }

        buffer[received_bytes] = '\0'; // Garantir terminação da string

        // Registrar a resposta recebida no log
        snprintf(log_entry, sizeof(log_entry) - 100, "Resposta do cliente %s:%d: %.900s", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
        log_message(log_entry);
        printf("Resposta recebida de %s:%d: %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
    }

    // Gravar log de desconexão do cliente
    snprintf(log_entry, sizeof(log_entry), "Conexão encerrada com o cliente %s:%d.", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    log_message(log_entry);
    printf("Conexão encerrada com o cliente %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Fechar conexão com o cliente
    close(client_socket);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    // Verificar se a porta e o backlog foram fornecidos
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <porta> <backlog>\n", argv[0]);
        exit(1);
    }

    int server_socket, *client_socket_ptr; // Ponteiro para o socket do cliente
    struct sockaddr_in server_addr, client_addr; // Estruturas para endereços do servidor e do cliente
    socklen_t client_len = sizeof(client_addr); // Tamanho da estrutura do cliente
    int port = atoi(argv[1]); // Porta do servidor
    int backlog = atoi(argv[2]); // Backlog fornecido como parâmetro

    // Verificar se o backlog é válido
    if (backlog < 0) {
        fprintf(stderr, "Backlog inválido. Deve ser um número não negativo.\n");
        exit(1);
    }

    // Criar socket do servidor
    server_socket = Socket(AF_INET, SOCK_STREAM, 0);

    // Configurar o endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Vincular o socket à porta
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        exit(1);
    }

    // Colocar o servidor em modo de escuta
    if (listen(server_socket, backlog) < 0) {
        perror("listen");
        close(server_socket);
        exit(1);
    }

    printf("Servidor iniciado. Aguardando conexões na porta %d com backlog %d...\n", port, backlog);
    snprintf((char[256]){}, 256, "Servidor iniciado na porta %d com backlog %d.", port, backlog);
    // Opcional: Registrar no log a inicialização do servidor
    char init_log[256];
    snprintf(init_log, sizeof(init_log), "Servidor iniciado na porta %d com backlog %d.", port, backlog);
    log_message(init_log);

    // Loop para aceitar múltiplos clientes
    while (1) {
        client_socket_ptr = malloc(sizeof(int));  // Alocar memória para o ponteiro do cliente
        if (client_socket_ptr == NULL) {
            perror("malloc");
            continue;
        }

        *client_socket_ptr = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (*client_socket_ptr < 0) {
            perror("accept");
            free(client_socket_ptr);  // Liberar memória em caso de falha
            continue;
        }

        // Criar uma nova thread para lidar com o cliente
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, client_socket_ptr) != 0) {
            perror("Falha ao criar thread");
            free(client_socket_ptr);  // Liberar memória em caso de falha
            continue;
        }

        pthread_detach(thread_id);  // Desanexar a thread para limpar seus recursos automaticamente

        // Adicionar um sleep para permitir observação das conexões
        sleep(1); // Ajuste o tempo conforme necessário
    }

    close(server_socket);
    pthread_mutex_destroy(&task_mutex);  // Destruir o mutex
    pthread_mutex_destroy(&log_mutex);   // Destruir o mutex de log
    return 0;
}
