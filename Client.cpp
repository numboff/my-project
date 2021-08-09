#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#pragma warning(disable: 4996)

struct Thread_Data {
	int id;
	SOCKET user_socket;
};

class message {
	char* msg;
public:
	message(const int msg_size)
	{
		msg = new char[msg_size + 1];
		msg[msg_size] = '\0';
	}
	~message()
	{
		delete[] msg;
	}
	char* getMsg()
	{
		return msg;
	}
};

void ClientHandler(Thread_Data* Data) {
	int msg_size, index, result;
	result = recv(Data->user_socket, (char*)&msg_size, sizeof(int), NULL);
	if (result == SOCKET_ERROR) {
		std::cout << "Server closed the connection;" << WSAGetLastError() << std::endl;
		closesocket(Data->user_socket);
	}
	message msg(msg_size);
	result = recv(Data->user_socket, msg.getMsg(), msg_size, NULL);
	if (result == SOCKET_ERROR) {
		std::cout << "Server closed the connection;" << WSAGetLastError() << std::endl;
		closesocket(Data->user_socket);
	}
	std::cout << "Recieved message: " << msg.getMsg() << std::endl;
	result = recv(Data->user_socket, (char*)&index, sizeof(int), NULL);
	if (result == SOCKET_ERROR) {
		std::cout << "Server closed the connection;" << WSAGetLastError() << std::endl;
		closesocket(Data->user_socket);
	}
	std::cout << "Your id is: " << index << std::endl;
	Data->id = index;

	while (true) {
		result = recv(Data->user_socket, (char*)&msg_size, sizeof(int), NULL);
		if (result == SOCKET_ERROR) {
			std::cout << "Server closed the connection; Error code: " << WSAGetLastError() << std::endl;
			closesocket(Data->user_socket);
			break;
		}
		message msg(msg_size);
		result = recv(Data->user_socket, msg.getMsg(), msg_size, NULL);
		if (result == SOCKET_ERROR) {
			std::cout << "Server closed the connection; Error code: " << WSAGetLastError() << std::endl;
			closesocket(Data->user_socket);
			break;
		}
		std::cout << "Recieved message: " <<msg.getMsg() << std::endl;
	}
}

const std::string TimeRightNow() {
	time_t     client_time = time(NULL);
	struct tm  tstruct;
	char       time_char[80];
	tstruct = *localtime(&client_time);
	strftime(time_char, sizeof(time_char), "%Y-%m-%d.%X", &tstruct);

	return time_char;
}

int main(int argc, char* argv[]) {
	//WSAStartup
	WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);
	if (WSAStartup(DLLVersion, &wsaData) != 0) {
		std::cout << "Error" << std::endl;
		exit(1);
	}

	SOCKADDR_IN addr;
	int sizeofaddr = sizeof(addr);
	if (argc > 2)
	{
		addr.sin_addr.s_addr = inet_addr(argv[1]);
		addr.sin_port = htons(atoi(argv[2]));
	}
	else
	{
		addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		addr.sin_port = htons(1111);
	}
	addr.sin_family = AF_INET;

	SOCKET newConnection;
	newConnection = socket(AF_INET, SOCK_STREAM, NULL);
	if (argc > 2)
	{
		std::cout << "Connecting to " << argv[1] << std::endl;
		std::cout << "Port " << argv[2] << std::endl;
	}
	else
	{
		std::cout << "Connecting to 127.0.0.1" << std::endl;
		std::cout << "Port 1111" << std::endl;
	}
	if (connect(newConnection, (SOCKADDR*)&addr, sizeof(addr)) != 0) {
		std::cout << "Error: failed connect to server.\n";
		return 1;
	}
	std::cout << "Connected!\n";

	
	Thread_Data Data;
	Data.user_socket = newConnection;
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, (LPVOID)(&Data), NULL, NULL);

	time_t client_time = time(NULL);
	int msg_size, index = Data.id;
	while (true)
	{
		std::string msg1, user_id_beginning = " User ", user_id_ending = ": ";
		std::getline(std::cin, msg1);
		user_id_beginning = TimeRightNow() + strcat((char*)user_id_beginning.c_str(), (std::to_string(Data.id)).c_str()) + user_id_ending + msg1;
		msg1 = user_id_beginning;
		msg_size = msg1.size();
		send(Data.user_socket, (char*)&msg_size, sizeof(int), NULL);
		send(Data.user_socket, msg1.c_str(), msg_size, NULL);
	}

	system("pause");
	WSACleanup();
	return 0;
}