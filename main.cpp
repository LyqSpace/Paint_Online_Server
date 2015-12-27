#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <vector>

using namespace std;

#include <opencv2/opencv.hpp>

using namespace cv;

const char *server_IP = "127.0.0.1";
const short server_port = 58520;
const int max_clients = 100;

struct TypeClient{
	char name[100];
	int socket;
	int id;
};

vector<TypeClient> clients;

void error(const char *msg)
{
	perror(msg);
	exit(0);
}

void sendMsgToAll(char* msg, int exceptId) {

	for (size_t i = 0; i < clients.size(); i++) {
		if (clients[i].id == exceptId) continue;
		send(clients[i].socket, msg, strlen(msg), 0);
	}
}

void* service_thread(void *param) {

	TypeClient newClient;
	newClient.socket = *(int*)param;

	char client_name[100];
	if (recv(newClient.socket, client_name, sizeof(client_name), 0) > 0) {
		strcpy(newClient.name, client_name);
	}
	newClient.id = clients.size();
	clients.push_back(newClient);

	char welcomeMsg[100] = {0};
	sprintf(welcomeMsg, "SERVER : Welcome %s come to Paint Online.", client_name);
	sendMsgToAll(welcomeMsg, -1);

	while (true) {

		char msg[100] = {0};
		if (recv(newClient.socket, msg, sizeof(msg), 0) == 0) {

			clients[newClient.id] = clients.back();
			clients.pop_back();

			char byeMsg[100];
			sprintf(byeMsg, "SERVER : %s leave.\n", client_name);
			sendMsgToAll(byeMsg, newClient.id);
			close(newClient.socket);
			return NULL;
		}

		sendMsgToAll(msg, newClient.id);
	}
}

int main() {

	cout << "INFO : Server starts..." << endl;

	sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_IP);
	server_addr.sin_port = htons(server_port);

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) {
		error("ERR : Create server socket failed.");
	}

	if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		error("ERR : Bind server socket failed.");
	}

	if (listen(server_socket, 10) < 0) {
		error("ERR : Listen failed.");
	}

	cout << "INFO : Initialize server host successful!" << endl;

	while (true) {

		sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
		if (client_socket < 0) {
			cout << "INFO : Client connection failed. Drop it." << endl;
			continue;
		}
		pthread_t pid;
		pthread_create(&pid, 0, service_thread, &client_socket);

	}

	return 0;

}

