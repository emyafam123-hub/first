#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <cstring>

// Специфичные заголовки для Windows
#include <winsock2.h>
#include <ws2tcpip.h>

// Подключаем библиотеку сокетов для линковщика
#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int PORT = 8080;
vector<SOCKET> clients; // В Windows тип сокета - SOCKET, а не int
mutex clientsMutex;

// Функция отправки сообщения всем, кроме отправителя
void broadcastMessage(const string& message, SOCKET senderSocket) {
    lock_guard<mutex> lock(clientsMutex);
    for (SOCKET client : clients) {
        if (client != senderSocket) {
            send(client, message.c_str(), (int)message.length(), 0);
        }
    }
}

void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesRead;

    cout << "[Server] Новый клиент подключился." << endl;

    {
        lock_guard<mutex> lock(clientsMutex);
        clients.push_back(clientSocket);
    }

    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytesRead] = '\0';
        string message(buffer);
        
        // Добавляем префикс (в реальном чате лучше использовать имена)
        string fullMessage = "Client: " + message;
        broadcastMessage(fullMessage, clientSocket);
    }

    cout << "[Server] Клиент отключился." << endl;
    closesocket(clientSocket); // В Windows используется closesocket

    lock_guard<mutex> lock(clientsMutex);
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (*it == clientSocket) {
            clients.erase(it);
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket, newSocket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 1. Инициализация Winsock (Обязательно для Windows!)
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Ошибка инициализации Winsock" << endl;
        return 1;
    }

    // 2. Создание сокета
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        cerr << "Ошибка создания сокета: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 3. Привязка
    if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        cerr << "Ошибка bind: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // 4. Прослушивание
    if (listen(serverSocket, 3) == SOCKET_ERROR) {
        cerr << "Ошибка listen: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "[Server] Сервер запущен на порту " << PORT << "..." << endl;

    while (true) {
        newSocket = accept(serverSocket, (struct sockaddr *)&address, &addrlen);
        if (newSocket == INVALID_SOCKET) {
            cerr << "Ошибка accept" << endl;
            continue;
        }

        thread t(handleClient, newSocket);
        t.detach();
    }

    // Очистка (никогда не достигнется в этом примере, но хороша для практики)
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
