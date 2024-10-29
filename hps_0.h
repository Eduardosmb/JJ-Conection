// hps_0.h

#ifndef _ALTERA_HPS_0_H_
#define _ALTERA_HPS_0_H_

/*
 * Este arquivo foi automaticamente gerado pela ferramenta swinfo2header.
 * 
 * Criado a partir do sistema SOPC Builder 'soc_system' no
 * arquivo './soc_system.sopcinfo'.
 *
 * Contém macros para o módulo 'hps_0' e dispositivos conectados a ele.
 * Certifique-se de que os valores dos offsets estão corretos conforme sua configuração de hardware.
 */

/* Definições de offsets de memória */
#define ALT_STM_OFST             0xFF200000  /* Offset base para registros do sistema */
#define ALT_LWFPGASLVS_OFST      0xFFC00000  /* Offset base para Lightweight FPGA Slaves */

/*
 * Macros para os GPIOs
 * Prefixo 'GPIO_'
 */
#define GPIO0_BASE               0xFF708000
#define GPIO1_BASE               0xFF709000
#define GPIO2_BASE               0xFF70A000

#endif /* _ALTERA_HPS_0_H_ */
