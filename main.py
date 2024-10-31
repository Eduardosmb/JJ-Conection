# main.py
import socket

HOST = '169.254.0.13'  # Substitua pelo endereço IP do servidor
PORTA = 8080

def enviar_comando(comando):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.connect((HOST, PORTA))
            s.sendall(comando.encode())
            data = s.recv(1024)
            print('Resposta do servidor:', data.decode())
        except ConnectionRefusedError:
            print("Erro: Não foi possível conectar ao servidor.")
        except Exception as e:
            print(f"Erro: {e}")

def menu():
    print("\n--- Cliente de Controle de LED ---")
    print("1. Acender o LED")
    print("2. Apagar o LED")
    print("3. Fazer o LED piscar")
    print("4. Status do botão")
    print("5. Sair")

    escolha = input("Escolha uma opção: ")
    return escolha

def main():
    while True:
        escolha = menu()
        if escolha == '1':
            enviar_comando("led_on")
        elif escolha == '2':
            enviar_comando("led_off")
        elif escolha == '3':
            enviar_comando("button_status")
        elif escolha == '4':
            print("Encerrando cliente.")
            break
        else:
            print("Opção inválida. Tente novamente.")

if __name__ == "__main__":
    main()
