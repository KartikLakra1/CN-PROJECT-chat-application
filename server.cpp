#include <iostream>
#include <winsock2.h>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080

struct ClientInfo
{
    SOCKET socket;
    std::string name;
};

std::vector<ClientInfo> clients;
std::mutex clients_mutex;

void handleClient(SOCKET clientSocket)
{
    char buffer[1024];

    // Step 1: Receive client's name
    memset(buffer, 0, sizeof(buffer));
    int nameBytes = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (nameBytes <= 0)
    {
        std::cout << "Failed to receive client name.\n";
        closesocket(clientSocket);
        return;
    }

    std::string clientName(buffer, nameBytes);

    // Add client to global list
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back({clientSocket, clientName});
    }

    std::cout << clientName << " connected.\n";

    // Step 2: Handle incoming messages and broadcast
    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0)
        {
            std::cout << clientName << " disconnected.\n";
            break;
        }

        std::string message = clientName + " says: " + std::string(buffer, bytesReceived);
        std::cout << message << "\n";

        // Broadcast to all other clients
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (const auto &client : clients)
        {
            if (client.socket != clientSocket)
            {
                send(client.socket, message.c_str(), message.size(), 0);
            }
        }
    }

    // Remove client on disconnect
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(std::remove_if(clients.begin(), clients.end(),
                                     [clientSocket](const ClientInfo &ci)
                                     { return ci.socket == clientSocket; }),
                      clients.end());
    }

    closesocket(clientSocket);
}

int main()
{
    WSADATA wsaData;
    SOCKET server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    // Bind
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR)
    {
        std::cerr << "Bind failed\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Listen
    if (listen(server_fd, 5) == SOCKET_ERROR)
    {
        std::cerr << "Listen failed\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    // Accept clients in a loop
    while (true)
    {
        SOCKET clientSocket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "Failed to accept client\n";
            continue;
        }

        std::cout << "New client connected!\n";

        // Launch new thread for this client
        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach(); // Run independently
    }

    // Cleanup (never reached here in this server loop)
    closesocket(server_fd);
    WSACleanup();

    return 0;
}
