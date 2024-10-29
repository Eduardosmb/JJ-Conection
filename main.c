/* main.c */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"

#define PORTA 8080
#define BUFFER_SIZE 1024

/* Definições de mapeamento de memória para GPIOs */
#define GPIO0_PHYS_ADDR 0xFF708000
#define GPIO1_PHYS_ADDR 0xFF709000
#define GPIO2_PHYS_ADDR 0xFF70A000
#define GPIO_SPAN       0x1000  // Span para um GPIO (cada GPIO ocupa 0x1000 bytes)

/* Definições para configuração do LED */
#define LED_GPIO         1  // Número do GPIO ao qual o LED está conectado (0, 1 ou 2)
#define LED_BIT          24 // Número do bit dentro do GPIO ao qual o LED está conectado (0 a 31)

/* Configuração do LED: Defina como 1 se for ativo alto, 0 se for ativo baixo */
#define LED_ATIVO_ALTO   1

/* Variáveis globais para mapeamento de memória */
void *virtual_base;
int fd;
volatile unsigned int *gpio_direction;  // Endereço de direção do GPIO selecionado
volatile unsigned int *gpio_data;       // Endereço de dados do GPIO selecionado

/* Variável global para o socket servidor */
int servidor_fd_global = -1;

/* Função de tratamento de sinal para limpar recursos */
void handle_signal(int signal) {
    printf("\nRecebido sinal de término. Limpando recursos...\n");

    /* Limpar mapeamento de memória */
    if (munmap(virtual_base, GPIO_SPAN) != 0) {
        perror("ERROR: munmap() failed");
    }
    close(fd);

    /* Fechar socket servidor */
    if (servidor_fd_global != -1) {
        close(servidor_fd_global);
    }

    exit(0);
}

/* Função para configurar o GPIO selecionado como saída */
void configurar_gpio_saida() {
    /* Supondo que o registro de direção está no offset 0x0 */
    // Define o bit como saída (1)
    *gpio_direction |= (1 << LED_BIT);
    printf("GPIO%d configurado como saída. Valor de direção: 0x%X\n", LED_GPIO, *gpio_direction);
}

/* Função para ligar o LED */
void led_on_func() {
    /*
     * Dependendo da configuração do hardware:
     * - Ativo baixo: Limpar o bit para ligar o LED.
     * - Ativo alto: Definir o bit para ligar o LED.
     *
     * Ajuste a lógica abaixo conforme a configuração do seu hardware.
     */

    if (LED_ATIVO_ALTO) {
        *gpio_data |= (1 << LED_BIT); // Define apenas o bit do LED
        printf("LED ligado (Ativo Alto). Valor escrito no GPIO%d: 0x%X\n", LED_GPIO, *gpio_data);
    } else {
        *gpio_data &= ~(1 << LED_BIT); // Limpa apenas o bit do LED
        printf("LED ligado (Ativo Baixo). Valor escrito no GPIO%d: 0x%X\n", LED_GPIO, *gpio_data);
    }

    /* Leitura e exibição do valor atual do GPIO */
    unsigned int current_value = *gpio_data;
    printf("Valor atual do GPIO%d após ligar: 0x%X\n", LED_GPIO, current_value);
}

/* Função para desligar o LED */
void led_off_func() {
    /*
     * Dependendo da configuração do hardware:
     * - Ativo baixo: Definir o bit para desligar o LED.
     * - Ativo alto: Limpar o bit para desligar o LED.
     *
     * Ajuste a lógica abaixo conforme a configuração do seu hardware.
     */

    if (LED_ATIVO_ALTO) {
        *gpio_data &= ~(1 << LED_BIT); // Limpa apenas o bit do LED
        printf("LED desligado (Ativo Alto). Valor escrito no GPIO%d: 0x%X\n", LED_GPIO, *gpio_data);
    } else {
        *gpio_data |= (1 << LED_BIT); // Define apenas o bit do LED
        printf("LED desligado (Ativo Baixo). Valor escrito no GPIO%d: 0x%X\n", LED_GPIO, *gpio_data);
    }

    /* Leitura e exibição do valor atual do GPIO */
    unsigned int current_value = *gpio_data;
    printf("Valor atual do GPIO%d após desligar: 0x%X\n", LED_GPIO, current_value);
}

/* Função para fazer o LED piscar */
void blink_led(int vezes, int intervalo_ms) {
    printf("Iniciando piscar o LED %d vezes com intervalo de %d ms.\n", vezes, intervalo_ms);
    for(int i = 0; i < vezes; i++) {
        led_on_func();
        usleep(intervalo_ms * 1000);
        led_off_func();
        usleep(intervalo_ms * 1000);
    }
    printf("Piscar do LED concluído.\n");
}

int main() {
    /* Configuração do tratamento de sinal */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    /* Verificar se o LED_GPIO está dentro do intervalo válido */
    if (LED_GPIO < 0 || LED_GPIO > 2) {
        fprintf(stderr, "Erro: LED_GPIO deve ser 0, 1 ou 2.\n");
        return 1;
    }

    /* Verificar se o LED_BIT está dentro do intervalo válido */
    if (LED_BIT < 0 || LED_BIT > 31) {
        fprintf(stderr, "Erro: LED_BIT deve estar entre 0 e 31.\n");
        return 1;
    }

    /* Definir o endereço físico base do GPIO selecionado */
    uint32_t gpio_base_phys;
    switch (LED_GPIO) {
        case 0:
            gpio_base_phys = GPIO0_PHYS_ADDR;
            break;
        case 1:
            gpio_base_phys = GPIO1_PHYS_ADDR;
            break;
        case 2:
            gpio_base_phys = GPIO2_PHYS_ADDR;
            break;
        default:
            fprintf(stderr, "Erro: GPIO inválido.\n");
            return 1;
    }

    /* Abrir o arquivo /dev/mem para mapeamento de memória */
    if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
        perror("ERROR: could not open \"/dev/mem\"");
        return( 1 );
    }

    /* Mapeamento de memória para o GPIO selecionado */
    virtual_base = mmap(NULL, GPIO_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, gpio_base_phys);

    if( virtual_base == MAP_FAILED ) {
        perror("ERROR: mmap() failed");
        close( fd );
        return( 1 );
    }

    printf("Memória mapeada para GPIO%d. Base virtual: %p\n", LED_GPIO, virtual_base);

    /* Definir os endereços de direção e dados do GPIO selecionado */
    gpio_direction = (volatile unsigned int *)(virtual_base + 0x0);  // Offset 0x0 para direção
    gpio_data = (volatile unsigned int *)(virtual_base + 0x4);       // Offset 0x4 para dados

    printf("Endereço de direção do GPIO%d mapeado para: %p\n", LED_GPIO, gpio_direction);
    printf("Endereço de dados do GPIO%d mapeado para: %p\n", LED_GPIO, gpio_data);

    /* Configurar GPIO como saída */
    configurar_gpio_saida();

    /* Inicialmente, desligar o LED */
    led_off_func();

    /* Configuração do socket TCP */
    int novo_socket;
    struct sockaddr_in endereco_sock;
    int opt = 1;
    int addrlen = sizeof(endereco_sock);
    char buffer[BUFFER_SIZE];

    /* Criando o socket */
    if ((servidor_fd_global = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Falha ao criar socket");
        handle_signal(SIGINT);
    }

    /* Configurando o socket */
    if (setsockopt(servidor_fd_global, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        perror("Falha ao configurar socket");
        handle_signal(SIGINT);
    }

    endereco_sock.sin_family = AF_INET;
    endereco_sock.sin_addr.s_addr = INADDR_ANY;
    endereco_sock.sin_port = htons(PORTA);

    /* Vinculando o socket ao endereço e porta especificados */
    if (bind(servidor_fd_global, (struct sockaddr *)&endereco_sock, sizeof(endereco_sock)) == -1) {
        perror("Falha ao vincular");
        handle_signal(SIGINT);
    }

    /* Escutando por conexões */
    if (listen(servidor_fd_global, 3) == -1) {
        perror("Falha ao escutar");
        handle_signal(SIGINT);
    }

    printf("Servidor em execução. Aguardando conexões na porta %d...\n", PORTA);

    while (1) {  // Loop principal do servidor
        printf("Aguardando conexão...\n");

        /* Aceitando conexão */
        if ((novo_socket = accept(servidor_fd_global, (struct sockaddr *)&endereco_sock, (socklen_t*)&addrlen)) == -1) {
            perror("Falha ao aceitar conexão");
            continue;  // Continua para a próxima iteração do loop
        }

        printf("Conexão estabelecida com um cliente.\n");

        /* Lendo dados do cliente */
        memset(buffer, 0, sizeof(buffer));  // Limpa o buffer
        int bytes_lidos = read(novo_socket, buffer, sizeof(buffer) - 1);
        if (bytes_lidos > 0) {
            buffer[bytes_lidos] = '\0';  // Garantir terminação da string
            printf("Mensagem recebida: %s\n", buffer);

            /* Processando comandos */
            char resposta[BUFFER_SIZE];
            if (strncmp(buffer, "status", 6) == 0) {
                // Implementação futura para status
                snprintf(resposta, sizeof(resposta), "Comando 'status' recebido. Status do LED não implementado.");
            }
            else if (strncmp(buffer, "blink", 5) == 0) {
                blink_led(10, 500);  // Pisca 10 vezes com intervalo de 500ms
                snprintf(resposta, sizeof(resposta), "LED piscando 10 vezes com intervalo de 500ms.");
            }
            else if (strncmp(buffer, "led_on", 6) == 0) {
                led_on_func();
                snprintf(resposta, sizeof(resposta), "LED aceso.");
            }
            else if (strncmp(buffer, "led_off", 7) == 0) {
                led_off_func();
                snprintf(resposta, sizeof(resposta), "LED apagado.");
            }
            else {
                snprintf(resposta, sizeof(resposta), "Comando desconhecido.");
            }

            /* Enviando resposta ao cliente */
            if (send(novo_socket, resposta, strlen(resposta), 0) == -1) {
                perror("Falha ao enviar resposta ao cliente");
            }
            printf("Resposta enviada ao cliente: %s\n", resposta);
        } else {
            if (bytes_lidos == 0) {
                printf("Conexão fechada pelo cliente.\n");
            } else {
                perror("Erro ao ler dados do cliente");
            }
        }

        close(novo_socket);  // Fecha o socket do cliente
        printf("Conexão com o cliente encerrada.\n");
    }

    /* Limpando mapeamento de memória e fechando arquivo (nunca alcançado neste exemplo) */
    munmap(virtual_base, GPIO_SPAN);
    close(fd);
    return 0;
}
