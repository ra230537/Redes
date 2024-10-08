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

// Função envelopadora para criar o socket conforme recomendado
int Socket(int family, int type, int protocol) {
    int sockfd;
    if ((sockfd = socket(family, type, protocol)) < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1; // Opção para permitir reutilização do endereço
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(1);
    }
    return sockfd;
}

// Função para registrar logs no arquivo
void log_message(const char *message) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        perror("Erro ao abrir o arquivo de log");
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
    getpeername(client_socket, (struct sockaddr *)&client_addr, &addr_len);
    
    // Gravar log de conexão com o cliente e seu endereço IP
    snprintf(log_entry, sizeof(log_entry), "Conexão estabelecida com o cliente %s:%d.", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    log_message(log_entry);

    // Loop para enviar múltiplas tarefas
    while (1) {
        pthread_mutex_lock(&task_mutex); // Bloquear o mutex
        task_number = next_task_number;
        next_task_number++;
        pthread_mutex_unlock(&task_mutex); // Desbloquear o mutex
        // Condição para encerrar a conexão após enviar 5 tarefas
        if (task_number > 200) {
            char *end_message = "ENCERRAR";
            send(client_socket, end_message, strlen(end_message), 0);

            snprintf(log_entry, sizeof(log_entry), "Instrução de encerramento enviada ao cliente.");
            log_message(log_entry);
            break;
        }
        // Definir a tarefa a ser enviada
        snprintf(task, sizeof(task), "TAREFA %d: LIMPEZA", task_number);

        // Enviar tarefa ao cliente
        send(client_socket, task, strlen(task), 0);

        // Registrar a tarefa enviada ao cliente específico no log
        snprintf(log_entry, sizeof(log_entry), "Tarefa %d enviada ao cliente %s:%d.", task_number, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        log_message(log_entry);

        // Receber resposta do cliente
        memset(buffer, 0, BUFFER_SIZE);
        int received_bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (received_bytes <= 0) {
            snprintf(log_entry, sizeof(log_entry), "Cliente %s:%d desconectou.", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            log_message(log_entry);
            break;
        }
        buffer[received_bytes] = '\0'; // Garantir terminação da string

        // Registrar a resposta recebida no log
        snprintf(log_entry, sizeof(log_entry) - 100, "Resposta do cliente %s:%d: %.900s", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
        log_message(log_entry);

        
    }

    // Gravar log de desconexão do cliente
    snprintf(log_entry, sizeof(log_entry), "Conexão encerrada com o cliente %s:%d.", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    log_message(log_entry);


    // Fechar conexão com o cliente
    close(client_socket);
    pthread_exit(NULL);

}

int main(int argc, char *argv[]) {
    // Verificar se a porta foi fornecida
    if (argc != 3) { // Modificado para 3 para aceitar o backlog como parâmetro
        fprintf(stderr, "Uso: %s <porta> <backlog>\n", argv[0]);
        exit(1);
    }

    int server_socket, *client_socket_ptr; // Ponteiro para o socket do cliente
    struct sockaddr_in server_addr, client_addr; // Estruturas para endereços do servidor e do cliente
    socklen_t client_len = sizeof(client_addr); // Tamanho da estrutura do cliente
    int port = atoi(argv[1]); // Porta do servidor
    int backlog = atoi(argv[2]); // Tamanho da fila de conexões pendentes

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

    printf("Servidor iniciado com backlog %d. Aguardando conexões na porta %d...\n", backlog, port);

    sleep(40);

    // Loop para aceitar múltiplos clientes
    while (1) {
        client_socket_ptr = malloc(sizeof(int));  // Alocar memória para o ponteiro do cliente
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
        }

        pthread_detach(thread_id);  // Desanexar a thread para limpar seus recursos automaticamente
    }

    sleep(40);

    close(server_socket);
    pthread_mutex_destroy(&task_mutex);  // Destruir o mutex
    return 0;
}