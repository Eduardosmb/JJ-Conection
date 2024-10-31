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

#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

#define USER_IO_DIR     (0x01000000)
#define BIT_LED         (0x01000000)
#define BUTTON_MASK     (0x02000000)


int servidor_fd_global = -1;
void *virtual_base;
int fd;

void handle_signal(int signal) {
    printf("\nRecebido sinal de término. Limpando recursos...\n");

    /* Limpar mapeamento de memória */
    if (munmap(virtual_base, HW_REGS_SPAN) != 0) {
        perror("ERROR: munmap() failed");
    }
    close(fd);

    /* Fechar socket servidor */
    if (servidor_fd_global != -1) {
        close(servidor_fd_global);
    }

    exit(0);
}


int main() {
    void *virtual_base;
	int fd;
	uint32_t  scan_input;
	int i;		
	// map the address space for the LED registers into user space so we can interact with them.
	// we'll actually map in the entire CSR span of the HPS since we want to access various registers within that span
	if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 1 );
	}

	virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );
	
	if( virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap() failed...\n" );
		close( fd );
		return( 1 );
	}
	// initialize the pio controller
	// led: set the direction of the HPS GPIO1 bits attached to LEDs to output
	alt_setbits_word( ( virtual_base + ( ( uint32_t )( ALT_GPIO1_SWPORTA_DDR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), USER_IO_DIR );
	printf("led test\r\n");
	printf("the led flash 2 times\r\n");
	for(i=0;i<2;i++)
	{
		alt_setbits_word( ( virtual_base + ( ( uint32_t )( ALT_GPIO1_SWPORTA_DR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), BIT_LED );
		usleep(500*1000);
		alt_clrbits_word( ( virtual_base + ( ( uint32_t )( ALT_GPIO1_SWPORTA_DR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), BIT_LED );
		usleep(500*1000);
	}

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
            else if (strncmp(buffer, "led_on", 6) == 0) {
		        alt_setbits_word( ( virtual_base + ( ( uint32_t )( ALT_GPIO1_SWPORTA_DR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), BIT_LED );
                snprintf(resposta, sizeof(resposta), "LED aceso.");
            }
            else if (strncmp(buffer, "led_off", 7) == 0) {
		        alt_clrbits_word( ( virtual_base + ( ( uint32_t )( ALT_GPIO1_SWPORTA_DR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), BIT_LED );
                snprintf(resposta, sizeof(resposta), "LED apagado.");
            }

            else if (strncmp(buffer, "button_status", 7) == 0) {
		        scan_input = alt_read_word( ( virtual_base + ( ( uint32_t )(  ALT_GPIO1_EXT_PORTA_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ) );		
                if(~scan_input&BUTTON_MASK){
                    snprintf(resposta, sizeof(resposta), "Botão pressionado");
                }
                else{
                    snprintf(resposta, sizeof(resposta), "Botão não pressionado");
                }
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
	// clean up our memory mapping and exit
	if( munmap( virtual_base, HW_REGS_SPAN ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( fd );
		return( 1 );
	}	
	close( fd );
	return( 0 );
}
