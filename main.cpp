#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include "threadpool.h" 
#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
using namespace std;

void setNonBlocking(SOCKET& socket) {
    u_long mode = 1;
    ioctlsocket(socket, FIONBIO, &mode);  
}

void interactwithclient(SOCKET clientSocket, vector<SOCKET>& clients, mutex& clients_mtx) {
    cout << "Connection accepted" << endl;
    char buffer[1024] = {0};
    
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            cout << "Client: " << buffer << endl;

            lock_guard<mutex> lock(clients_mtx);
            for (auto it : clients) {
                if (it != clientSocket) {
                    send(it, buffer, bytesReceived, 0);
                }
            }
        } else if (bytesReceived == 0) {
            cout << "Client disconnected" << endl;
            break;
        } else if (WSAGetLastError() != WSAEWOULDBLOCK) {
        
            cout << "recv error: " << WSAGetLastError() << endl;
            break;
        }

    
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    {
        lock_guard<mutex> lock(clients_mtx);
        auto it = find(clients.begin(), clients.end(), clientSocket);
        if (it != clients.end()) {
            clients.erase(it);
        }
    }

    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int addrLen = sizeof(clientAddr);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return 1;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Socket creation failed" << endl;
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Bind failed" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, 3) < 0) {
        cerr << "Listen failed" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Waiting for connections..." << endl;

    vector<SOCKET> clients;
    mutex clients_mtx;

    ThreadPool pool(4); 

    setNonBlocking(serverSocket);  

    while (true) {
       
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);

        {
            lock_guard<mutex> lock(clients_mtx);
            for (auto& client : clients) {
                FD_SET(client, &readfds);
            }
        }

        timeval timeout = {0, 100000};  
        int activity = select(0, &readfds, NULL, NULL, &timeout);
        if (activity == SOCKET_ERROR) {
            cerr << "Select error: " << WSAGetLastError() << endl;
            break;
        }

        
        if (FD_ISSET(serverSocket, &readfds)) {
            clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
            if (clientSocket == INVALID_SOCKET) {
                if (WSAGetLastError() != WSAEWOULDBLOCK) {
                    cerr << "Accept failed: " << WSAGetLastError() << endl;
                }
            } else {
                setNonBlocking(clientSocket);  
                {
                    lock_guard<mutex> lock(clients_mtx);
                    clients.push_back(clientSocket);
                }
                pool.ExecuteTask(interactwithclient, clientSocket, ref(clients), ref(clients_mtx));
            }
        }


        {
            lock_guard<mutex> lock(clients_mtx);
            for (auto& client : clients) {
                if (FD_ISSET(client, &readfds)) {
                    pool.ExecuteTask(interactwithclient, client, ref(clients), ref(clients_mtx));
                }
            }
        }
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
