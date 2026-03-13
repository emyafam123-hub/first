#include <iostream>
#include <string>
#include <thread>
#include <cstring>

// Заголовки Windows
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const char* SERVER_IP = "127.0.0.1";
const int PORT = 8080;

void receiveMessages(SOCKET sock) {
    char buffer[1024];
    int bytesRead;
    while ((bytesRead = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytesRead] = '\0';
        cout << "\n[CHAT] " << buffer << endl;
        cout << "> ";
        cout.flush();
    }
    cout << "\n[Система] Соединение разорвано." << endl;
    closesocket(sock);
    exit(0);
}

int main() {
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in serv_addr;

    // 1. Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "Ошибка Winsock" << endl;
        return 1;
    }

    // 2. Создание сокета
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cout << "Ошибка создания сокета" << endl;
        WSACleanup();
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Преобразование IP (inet_pton доступна в новых версиях Windows/MinGW)
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        cout << "Неверный адрес" << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // 3. Подключение
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        cout << "Ошибка подключения. Запущен ли сервер?" << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    cout << "Подключено к чату! Введите сообщение (или 'exit' для выхода):" << endl;

    thread receiver(receiveMessages, sock);
    receiver.detach();

    string message;
    while (true) {
        cout << "> ";
        getline(cin, message);
        if (message == "exit") break;
        
        send(sock, message.c_str(), (int)message.length(), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}