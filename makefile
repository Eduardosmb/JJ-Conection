# Makefile

TARGET = main
ALT_DEVICE_FAMILY ?= soc_cv_av
SOCEDS_ROOT ?= $(SOCEDS_DEST_ROOT)
HWLIBS_ROOT = $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib
CROSS_COMPILE = /home/eduba/Downloads/soc/gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
CFLAGS = -g -Wall -D$(ALT_DEVICE_FAMILY) -I$(HWLIBS_ROOT)/include/$(ALT_DEVICE_FAMILY) -I$(HWLIBS_ROOT)/include/
LDFLAGS = -g -Wall 
CC = $(CROSS_COMPILE)gcc
ARCH= arm

# Alvo padrão
all: build

# Regra para compilar o alvo principal
build: $(TARGET)

# Regra para vincular o executável a partir do objeto
$(TARGET): main.o
	$(CC) $(LDFLAGS) $^ -o $@

# Regra para compilar arquivos .c em .o
%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Alvo para limpeza
.PHONY: clean
clean:
	rm -f $(TARGET) *.a *.o *~
