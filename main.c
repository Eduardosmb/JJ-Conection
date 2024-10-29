// servidor.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORTA 8080

int main() {
    int servidor_fd, novo_socket;
    struct sockaddr_in endereco;
    int opt = 1;
    int addrlen = sizeof(endereco);
    char buffer[1024];

    // Criando o socket
    if ((servidor_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Falha ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configurando o socket
    if (setsockopt(servidor_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Falha ao configurar socket");
        exit(EXIT_FAILURE);
    }

    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = INADDR_ANY;
    endereco.sin_port = htons(PORTA);

    // Vinculando o socket ao endereço e porta especificados
    if (bind(servidor_fd, (struct sockaddr *)&endereco, sizeof(endereco)) < 0) {
        perror("Falha ao vincular");
        exit(EXIT_FAILURE);
    }

    // Escutando por conexões
    if (listen(servidor_fd, 3) < 0) {
        perror("Falha ao escutar");
        exit(EXIT_FAILURE);
    }

    printf("Servidor em execução. Aguardando conexões na porta %d...\n", PORTA);

    while (1) {  // Loop principal do servidor
        printf("Aguardando conexão...\n");

        // Aceitando conexão
        if ((novo_socket = accept(servidor_fd, (struct sockaddr *)&endereco, (socklen_t*)&addrlen)) < 0) {
            perror("Falha ao aceitar conexão");
            continue;  // Continua para a próxima iteração do loop
        }

        printf("Conexão estabelecida com um cliente.\n");

        // Lendo dados do cliente
        memset(buffer, 0, sizeof(buffer));  // Limpa o buffer
        int bytes_lidos = read(novo_socket, buffer, sizeof(buffer) - 1);
        if (bytes_lidos > 0) {
            printf("Mensagem recebida: %s\n", buffer);

            // Enviando resposta ao cliente
            char *resposta = "Mensagem recebida pelo servidor em C";
            send(novo_socket, resposta, strlen(resposta), 0);
            printf("Resposta enviada ao cliente.\n");
        } else {
            printf("Nenhuma mensagem recebida do cliente.\n");
        }

        close(novo_socket);  // Fecha o socket do cliente
        printf("Conexão com o cliente encerrada.\n");
    }

    close(servidor_fd);  // Fecha o socket do servidor (nunca alcançado neste exemplo)
    return 0;
}