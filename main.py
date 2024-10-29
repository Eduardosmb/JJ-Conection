# cliente.py
import socket

HOST = '169.254.0.13'  # Substitua pelo endereço IP do servidor
PORTA = 8080

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORTA))
    mensagem = 'Olá do cliente Python!'
    s.sendall(mensagem.encode())
    data = s.recv(1024)

print('Resposta do servidor:', data.decode())