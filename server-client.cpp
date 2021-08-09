#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <string.h>
#include <string>
#include <fstream>
#include <vector>
#include <windows.h>
#include <stdio.h>
#include <cstdio>
#include <stdexcept>
#include <algorithm>
#include <queue>

#define number_of_connections 100
#pragma warning(disable: 4996)

#if defined NDEBUG
#define TRACE( format, ... )
#else
#define TRACE( format, ... )   printf( "%s::%s(%d)" format, __FILE__, __FUNCTION__,  __LINE__, __VA_ARGS__ )
#endif

HANDLE ghMutex; // передавать оборачивающий его класс по ссылке от одной части программы к другой

class Mutex {
public:
	Mutex(const HANDLE &inmutex) : mutex(inmutex)
	{
		if (!mutex)
			throw std::runtime_error("MutexClass Error 1: mutex creation error;");
		WaitForSingleObject(
			mutex,    // handle to mutex
			INFINITE);	// no time-out interval
	}
	~Mutex()
	{
		ReleaseMutex(mutex);
	}
private:
	HANDLE mutex;
};

class Message {
	char* msg;
public:
	Message(const int msg_size)
	{
		msg = new char[msg_size + 1];
		msg[msg_size] = '\0';
	}
	~Message()
	{
		delete[] msg;
	}
	char* getMsg()
	{
		return msg;
	}
//+метод с распознаванем размера сообщения
};

class Connected_Users_Array{ //vector
public:
	Connected_Users_Array()
	{
		array_mutex = CreateMutex(
			NULL,              // default security attributes
			FALSE,             // initially not owned
			NULL);             // unnamed mutex
	}

	~Connected_Users_Array()
	{
		connected_users.clear();
	}

	void Add(SOCKET& client)
	{
		Mutex mutex(array_mutex);
		connected_users.push_back(client);
	}

	void Remove(SOCKET& client)
	{
		for (size_t i = 0; i < connected_users.size(); i++) std::cout << connected_users[i] << std::endl;
		Mutex mutex(array_mutex);
		size_t client_id;
		for (client_id = 0; client_id < connected_users.size(); client_id++)
		{
			if (connected_users[client_id] == client) break;
		}
		connected_users.erase(connected_users.begin() + client_id);
	}

	void SendMessageToAllClients(SOCKET& сlient, const int msg_size, char* msg)
	{
		size_t check;
		Mutex mutex(array_mutex);
		for (size_t i = 0; i < connected_users.size(); i++) {
			if (сlient == connected_users[i]) {
				continue;
			}

			check = send(connected_users[i], (char*)&msg_size, sizeof(int), NULL);
			if (check == -1) std::cout << "send() error in array class; (size)" << std::endl;
			check = send(connected_users[i], msg, msg_size, NULL);
			if (check == -1) std::cout << "send() error in array class; (message)" << std::endl;
		}
	}

	 void SendMessageToClient(SOCKET& сlient, const int msg_size, std::string msg)
	{
		size_t check;
		Mutex mutex(array_mutex);
		std::cout << std::endl;
		check = send(сlient, (char*)&msg_size, sizeof(int), NULL);
		if (check == -1) std::cout << "send() error" << std::endl;
		check = send(сlient, msg.c_str(), msg_size, NULL);
		if (check == -1) std::cout << "send() error" << std::endl;
	}

private:
	std::vector<SOCKET> connected_users; // локальные переменные в классах с маленькой
	HANDLE array_mutex;
};

class File {
public:
	File(const char* filename) : m_file_handle(std::fopen(filename, "w+"))
	{
		if (!m_file_handle)
			throw std::runtime_error("FileClass Error 1: file open failure;");
		file_name = filename;
	}
	~File()
	{
		fclose(m_file_handle);
	}

	void Write(std::string str)
	{
		if (std::fputs(str.c_str(), m_file_handle) == EOF)
		{
			throw std::runtime_error("FileClass Error 2: file write failure;");
		}
		std::fputs("\n", m_file_handle);

		if (lastMessages.size() < queue_size)
		{
			lastMessages.push_back(str);
		}
		else
		{
			lastMessages.pop_front();
			lastMessages.push_back(str);
		}
	}

	void GetLastLines(SOCKET& client, Connected_Users_Array* array)
	{
		if (lastMessages.empty())
		{
			std::string msg = "There is no messages yet in this chat, be the first!";
			array->SendMessageToClient(client, msg.size(), (char*)msg.c_str());
		}
		else
		{
			for (size_t i = 0; i < lastMessages.size(); i++)
			{
				array->SendMessageToClient(client, lastMessages[i].size(), lastMessages[i]);
			}
		}
	}

private:
	std::FILE* m_file_handle;
	const char* file_name;
	
	std::deque <std::string> lastMessages;
	size_t queue_size = 10;

	// Копирование и присваивание не реализовано.  Предотвратим их использование,
	// объявив соответствующие методы закрытыми.
	File(const File &);
	File & operator=(const File &);
};

struct Thread_Data {
	int id;
	Connected_Users_Array* array;
	SOCKET user_socket;
};

File file("msg.txt");

void ClientHandler(Thread_Data* Data) {
	int msg_size, result;
	int index = Data->id;
	std::string time_msg;
	SOCKET user = Data->user_socket;
	Connected_Users_Array* connected_users = Data->array;

	file.GetLastLines(user, Data->array);
	while (true) {

		result = recv(user, (char*)&msg_size, sizeof(int), NULL); // case: ... timeout ... error ... disconnected
		if (result == SOCKET_ERROR) {
			std::cout << "Socket error 1: User "<< index<<" disconnected; " << WSAGetLastError() << std::endl;
			connected_users->Remove(user);
			break;
		}
		
		Message msg(msg_size);

		result = recv(user, msg.getMsg(), msg_size, NULL);
		if (result == SOCKET_ERROR) {
			std::cout << "Socket error 2: User " << index << " disconnected; " << WSAGetLastError() << std::endl;
			connected_users->Remove(user);
			break;
		}

		connected_users->SendMessageToAllClients(user, msg_size, msg.getMsg());
		
		//mutex
		try {
			Mutex mutex(ghMutex);
			file.Write(time_msg + msg.getMsg());
		}
		catch (int j) {
			std::cout << "caught exception " << j << std::endl;
		}
	}
}

int main(int argc, char* argv[]) {
	std::cout << argc << std::endl;
	std::cout << argv << std::endl;
	int counter = 0;
	Connected_Users_Array client_array; /*socket(AF_INET, SOCK_STREAM, NULL)*/

	//WSAStartup
	WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);
	if (WSAStartup(DLLVersion, &wsaData) != 0) {
		std::cout << "Error 1: WSA init error;" << std::endl;
		exit(1);
	}

	SOCKADDR_IN addr, addr_client;
	int sizeofaddr = sizeof(addr), sizeofaddr_client = sizeof(addr_client);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (argc > 1)
	{
		std::cout << atoi(argv[1]) << std::endl;
		addr.sin_port = htons(atoi(argv[1]));
	}
	else
	{
		addr.sin_port = htons(1111);
	}
	addr.sin_family = AF_INET;

	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
	
	listen(sListen, SOMAXCONN);

	SOCKET newConnection;

	ghMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex

	if (ghMutex == NULL)
	{
		printf("Error 2: CreateMutex error %d;\n", GetLastError());
		return 1;
	}

	for (int i = 0; i < number_of_connections; i++) {
		
		std::cout << "Waiting for accept " << std::endl;
		int timeout = 2000000;

		newConnection = accept(sListen, (SOCKADDR*)&addr_client, &sizeofaddr_client);
		int temporary_opt = setsockopt(newConnection, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
		if (temporary_opt == SOCKET_ERROR) {
			std::cout << "Error 3: setsockopt error " <<  WSAGetLastError() << std::endl;
		}

		unsigned int ip = ntohl(addr_client.sin_addr.s_addr);
		std::cout << ((ip >> 24) & 0xFF) << "." << ((ip >> 16) & 0xFF) << "." << ((ip >> 8) & 0xFF) << "." << ((ip) & 0xFF) << "; " << addr_client.sin_port << std::endl;

		if (newConnection == 0) {
			std::cout << "Error 4: newConnection error \n";
		}
		else {
			std::cout << "Client Connected!\n";
			std::string msg = "Welcome to our little chat! Have a nice conversation!";
			client_array.SendMessageToClient(newConnection, msg.size(), msg);
			send(newConnection, (char*)&i, sizeof(int), NULL);

			Thread_Data Data;
			Data.id = i;
			Data.array = &client_array;
			Data.user_socket = newConnection;

			client_array.Add(newConnection);
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, (LPVOID)(&Data), NULL, NULL);
		}
	}
	system("pause");
	return 0;
}