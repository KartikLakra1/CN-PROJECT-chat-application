// client.cpp (Windows)
#include <iostream>
#include <winsock2.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define SERVER_IP "127.0.0.1"
using namespace std;

void receiveMessages(SOCKET client_socket)
{
    char buffer[1024];
    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0)
        {
            std::cout << "\n"
                      << std::string(buffer, bytesReceived) << "\nYou: ";
        }
        else if (bytesReceived == 0)
        {
            std::cout << "\nServer disconnected.\n";
            break;
        }
        else
        {
            std::cerr << "\nReceive failed.\n";
            break;
        }
    }
}

int main()
{
    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_address;

    // Step 1: Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    // Step 2: Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    // Step 3: Define server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Step 4: Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR)
    {
        std::cerr << "Connection to server failed\n";
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server!\n";

    // Send name to server
    std::string name;
    std::cout << "Enter your name: ";
    std::getline(std::cin, name);
    send(client_socket, name.c_str(), name.length(), 0);

    // Start receiver thread
    std::thread receiverThread(receiveMessages, client_socket);
    receiverThread.detach();

    // Step 5: Send message
    std::string message;
    while (true)
    {
        std::cout << "You: ";
        std::getline(std::cin, message);

        if (message == "exit")
        {
            std::cout << "Exiting...\n";
            break;
        }

        send(client_socket, message.c_str(), message.length(), 0);
    }
    // Step 6: Cleanup
    closesocket(client_socket);
    WSACleanup();

    return 0;
}
