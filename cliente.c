#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

// Função envelopadora para criar o socket
int Socket(int family, int type, int protocol) {
    int sockfd;
    if ((sockfd = socket(family, type, protocol)) < 0) {
        perror("socket");
        exit(1);
    }
    return sockfd;
}

// Função envelopadora para connect
void Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if (connect(sockfd, addr, addrlen) < 0) {
        perror("connect");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <IP do servidor> <porta>\n", argv[0]);
        exit(1);
    }

    int client_socket; // Socket do cliente
    struct sockaddr_in server_addr, local_addr; // Endereços do servidor e local
    socklen_t addr_size; // Tamanho da estrutura de endereço
    char buffer[BUFFER_SIZE]; // Buffer para mensagens
    char *response = "TAREFA_LIMPEZA CONCLUÍDA"; // Resposta do cliente
    char *server_ip = argv[1]; // IP do servidor
    int server_port = atoi(argv[2]); // Porta do servidor

    // Criar socket do cliente
    client_socket = Socket(AF_INET, SOCK_STREAM, 0);

    // Configurar o endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    // Conectar ao servidor
    Connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // Obter informações sobre a conexão local
    addr_size = sizeof(local_addr);
    if (getsockname(client_socket, (struct sockaddr *)&local_addr, &addr_size) == -1) { // Obter informações sobre a conexão local
        perror("getsockname");
        exit(1);
    }

    char client_ip[INET_ADDRSTRLEN]; // IP do cliente
    inet_ntop(AF_INET, &local_addr.sin_addr, client_ip, INET_ADDRSTRLEN); // Converter endereço IP para string
    int client_port = ntohs(local_addr.sin_port); // Porta do cliente

    // Imprimir informações sobre a conexão
    printf("Conectado ao servidor %s:%d\n", server_ip, server_port);
    printf("Conexão local: %s:%d\n", client_ip, client_port);

    // Loop principal para receber e processar tarefas
    while (1) {
        // Receber tarefa do servidor
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("Conexão fechada pelo servidor.\n");
            } else {
                perror("recv");
            }
            break;
        }

        buffer[bytes_received] = '\0'; // Assegurar terminação da string

        // Verificar se é a instrução de encerrar
        if (strcmp(buffer, "ENCERRAR") == 0) {
            printf("Instrução de encerramento recebida. Fechando conexão.\n");
            break;
        }

        printf("Tarefa recebida: %s\n", buffer);

        // Simular execução da tarefa (sleep de 5 segundos)
        printf("Executando tarefa...\n");
        sleep(2);

        // Enviar resposta ao servidor
        send(client_socket, response, strlen(response), 0);
        printf("Resposta enviada ao servidor: %s\n", response);
    }

    // Fechar conexão com o servidor
    close(client_socket);

    return 0;
}