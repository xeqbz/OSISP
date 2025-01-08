﻿#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <string>
#include <unordered_map>

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

struct ClientInfo {
	SOCKET socket;
	char name[21];
	std::vector<std::string> msgs;

	ClientInfo(SOCKET s, const char* n)
		: socket(s), msgs() {
		strncpy_s(name, n, sizeof(name) - 1);
		name[sizeof(name) - 1] = '\0'; 
	}
};

struct addrinfo* result = NULL, * ptr = NULL, hints;
std::vector<ClientInfo> clientSockets; 
std::mutex clientMutex;            


void static BroadcastMessage(const std::string& message, SOCKET senderSocket) {
	std::lock_guard<std::mutex> lock(clientMutex);
	for (const auto& client : clientSockets) {
		if (client.socket != senderSocket) {  
			send(client.socket, message.c_str(), int(message.size()), 0);
		}
	}
}

void HandleClient(SOCKET ClientSocket) {
	char recvbuf[DEFAULT_BUFLEN];
	int iResult, iSendResult;
	int recvbuflen = DEFAULT_BUFLEN;

	iResult = recv(ClientSocket, recvbuf, DEFAULT_BUFLEN, 0);
	if (iResult <= 0) {
		closesocket(ClientSocket);
		return;
	}
	recvbuf[iResult] = '\0';  
	char clientName[21];

	strncpy_s(clientName, recvbuf, sizeof(clientName) - 1);
	clientName[sizeof(clientName) - 1] = '\0';  

	{
		std::lock_guard<std::mutex> lock(clientMutex);
		clientSockets.push_back(ClientInfo(ClientSocket, clientName));
	}
	std::cout << "Client connected: " << recvbuf << std::endl;

	do {
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0); 
		printf("Bytes received: %d\n", iResult);
		if (iResult > 0) {
			recvbuf[iResult] = '\0';
			std::string message = recvbuf;

			if (strlen(recvbuf) == 0) {
				printf("Client %s disconnected.\n", clientName);
				break;
			}
			else {
				printf("Message received: %s\n", recvbuf);
			}


			size_t arrowPos = message.find("->");
			if (arrowPos != std::string::npos) {
				std::string actualMessage = message.substr(0, arrowPos); 
				std::string targetID = message.substr(arrowPos + 2);     

				{
					std::lock_guard<std::mutex> lock(clientMutex);
					for (auto& client : clientSockets) {
						if (client.socket == ClientSocket) {
							client.msgs.push_back(actualMessage); 
							break;
						}
					}
				}

				bool clientFound = false;
				{
					std::lock_guard<std::mutex> lock(clientMutex);
					for (const auto& client : clientSockets) {
						if (client.name == targetID) {
							clientFound = true;
							std::string name(clientName);
							std::string directedMessage = name + " (private): " + actualMessage;
							send(client.socket, directedMessage.c_str(), static_cast<int>(directedMessage.size()), 0);
							break;
						}
					}
				}
				if (!clientFound) {
					std::string errorMsg = "Error: Client with ID '" + targetID + "' not found.\n";
					send(ClientSocket, errorMsg.c_str(), static_cast<int>(errorMsg.size()), 0);
				}
			}
			else {
				std::string name(clientName);
				std::string broadcastMessage = name + ": " + message;
				BroadcastMessage(broadcastMessage, ClientSocket);
			}

		}
		else {
			printf("Recv failed: %d\n", WSAGetLastError());
			break;
		}

	} while (iResult > 0);

	{
		std::lock_guard<std::mutex> lock(clientMutex);
		clientSockets.erase(std::remove_if(clientSockets.begin(), clientSockets.end(),
			[ClientSocket](const ClientInfo& client) {
				return client.socket == ClientSocket;
			}),
			clientSockets.end());
	}

	printf("Client %s removed from list.\n", clientName);
	closesocket(ClientSocket);
	return;
}

int main() {
	WSADATA wsaData; 

	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	// clear hints structure
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;              
	hints.ai_socktype = SOCK_STREAM;        
	hints.ai_protocol = IPPROTO_TCP;        
	hints.ai_flags = AI_PASSIVE;            

	iResult = getaddrinfo("0.0.0.0", DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);                               
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	printf("Socket is listening\n");

	while (true) {
		SOCKET ClientSocket = INVALID_SOCKET;

		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed: %d\n", WSAGetLastError());
			continue; 
		}

		printf("New client connected.\n");

		std::thread(HandleClient, ClientSocket).detach();
	}

	closesocket(ListenSocket);
	WSACleanup();
	return 0;
}