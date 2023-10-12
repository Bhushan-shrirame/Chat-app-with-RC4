#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <fstream>
#include <vector>
#include <netinet/in.h>
#include <unistd.h>
#include <unordered_map>

const int PORT = 10069;
const int MAX_CONNECTIONS = 15;

struct ClientInfo
{
    int socket;
    std::string name;
};

std::vector<ClientInfo> clients;
std::unordered_map<int, int> clientConnection;

void sendMessage(std::string message, const int clientSocket)
{
    send(clientSocket, message.c_str(), message.length(), 0);
}

int decodeMessage(std::string &message, const std::string &clientName, const int clientSocket)
{
    std::string decode = "";
    for (int i = 0; i < message.size(); i++)
    {
        if (message[i] == ' ')
        {
            break;
        }
        decode += message[i];
    }
    if (decode == "active-clients")
    {
        if (clients.size() == 1)
        {
            sendMessage("Server : No users Conneted right now", clientSocket);
            return 0;
        }
        for (ClientInfo &client : clients)
        {
            if (client.name != clientName)
            {
                std::string name = client.name + "\n";
                send(clientSocket, name.c_str(), name.length(), 0);
            }
        }
        return 0;
    }
    else if (decode == "establish-connection")
    {
        std::string name = "";
        int otherSocketId = -1;
        for (int i = decode.size() + 1; i < message.size(); i++)
        {
            if (message[i] == ' ')
            {
                break;
            }
            name += message[i];
        }
        for (auto &client : clients)
        {
            if (client.name == name)
            {
                otherSocketId = client.socket;
                break;
            }
        }
        if (clientConnection[clientSocket] != -1)
        {
            sendMessage("Server : You can not connect until you disconnected your current Connection", clientSocket);
            return 0;
        }
        if (otherSocketId == clientSocket)
        {
            sendMessage("Server : You can not connect to yourself", clientSocket);
            return 0;
        }
        else
        {
            if (clientConnection[otherSocketId] != -1)
            {
                sendMessage("Server : Connection can't be established as other user is connected to someone else", clientSocket);
                return 0;
            }
            else
            {
                clientConnection[otherSocketId] = clientSocket;
                clientConnection[clientSocket] = otherSocketId;
                std::string msg1 = "Server : You are now connected to " + name;
                std::string msg2 = "Server : You are now connected to " + clientName;
                sendMessage(msg1, clientSocket);
                sendMessage(msg2, otherSocketId);
            }
        }
        return 0;
    }
    else if (decode == "terminate")
    {
        int connectedTo = clientConnection[clientSocket];
        if (clientConnection[connectedTo] == clientSocket)
        {
            std::string msg1 = "Server : " + clientName + " disconnected";
            sendMessage(msg1, connectedTo);
            sendMessage("Server : Connection Closed", clientSocket);
            clientConnection[clientSocket] = -1;
            clientConnection[connectedTo] = -1;
        }
        else
            sendMessage("You are not connected to anyone", clientSocket);
        return 0;
    }
    else
        return 1;
}

void handleClient(int clientSocket, const std::string &clientName)
{
    char buffer[1024];
    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0)
        {
            std::cerr << "000 Server : Client " << clientName << " disconnected." << std::endl;
            if (clientConnection[clientSocket] != -1)
            {
                std::string msg = "000 Server : Client " + clientName + " disconnected";
                sendMessage(msg, clientConnection[clientSocket]);
                clientConnection[clientConnection[clientSocket]] = -1;
            }
            clientConnection[clientSocket] = -1;
            close(clientSocket);

            for (auto it = clients.begin(); it != clients.end(); ++it)
            {
                if (it->socket == clientSocket)
                {
                    clients.erase(it);
                    break;
                }
            }
            return;
        }
        std::string message(buffer);
        if (clientConnection[clientSocket] != -1)
        {
            std::fstream logFile;
            std::string fileName = std::to_string(clientSocket) + std::to_string(clientConnection[clientSocket]) + ".log";
            logFile.open(fileName, std::fstream::in | std::fstream::out | std::fstream::app);
            logFile << message << "\n";
        }
        if (decodeMessage(message, clientName, clientSocket))
        {
            std::string to_send = clientName + " : " + message;
            if (clientConnection[clientSocket] != -1)
                sendMessage(to_send, clientConnection[clientSocket]);
            else
            {
                sendMessage("Server : You are not connected to anyone right now\n", clientSocket);
            }
        }
    }
}

int main()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    sockaddr_in serverAddress, clientAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        std::cerr << "Error binding socket" << std::endl;
        return -1;
    }

    if (listen(serverSocket, MAX_CONNECTIONS) == -1)
    {
        std::cerr << "Error listening on socket" << std::endl;
        return -1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true)
    {
        socklen_t clientAddressSize = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressSize);
        if (clientSocket == -1)
        {
            std::cerr << "Error accepting client connection" << std::endl;
            continue;
        }

        char nameBuffer[256];
        memset(nameBuffer, 0, sizeof(nameBuffer));
        int nameReceived = recv(clientSocket, nameBuffer, sizeof(nameBuffer), 0);
        if (nameReceived <= 0)
        {
            std::cerr << "Error receiving client name." << std::endl;
            close(clientSocket);
            continue;
        }
        std::string clientName(nameBuffer);

        clients.push_back({clientSocket, clientName});
        clientConnection[clientSocket] = -1;
        std::cout << "Client " << clientName << " connected." << std::endl;

        std::thread clientThread(handleClient, clientSocket, clientName);
        clientThread.detach();
    }

    close(serverSocket);

    return 0;
}
