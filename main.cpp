#include <iostream>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>

/*
    Chatclient door Midas van Veen
    usage: chatclient <mode> <ip (only if sending)> <port>
    modes: recieve, send
    exit command: exit

    FIXME: client verstuurt max 8 characters
*/

using namespace std;

class Listener {
public:
  Listener(int port) {
    this->listening = socket(AF_INET, SOCK_STREAM, 0);
    this->hint.sin_family = AF_INET;
    this->hint.sin_port = htons(port);
    inet_pton(AF_INET, "0.0.0.0", &this->hint.sin_addr);
    bind(this->listening, (sockaddr*)&this->hint, sizeof(this->hint));
    listen(this->listening, SOMAXCONN);

    this->clientSize = sizeof(this->client);
    this->clientSocket = accept(this->listening,(sockaddr*)&this->client,&this->clientSize);

    close(this->listening);
    memset(this->host,0,NI_MAXHOST);
    memset(this->svc,0,NI_MAXSERV);

    int result = getnameinfo((sockaddr*)&this->client,sizeof(this->client),host,NI_MAXHOST,svc,NI_MAXSERV,0);
    if (result) {
      cout << this->host << " connected on " << svc << endl;
    } else {
      inet_ntop(AF_INET, &this->client.sin_addr, host, NI_MAXHOST);
      cout << this->host << " connected on " << ntohs(this->client.sin_port) << endl;
    }
  }

  ~Listener() {
    close(this->clientSocket);
  }

  int serverSend(char* data, int size) {
    send(this->clientSocket,data, size,0);
    return 0;
  }

  int getBytesRecv() {
    int bytesRecv = recv(this->clientSocket,this->buffer,4096,0);
    return bytesRecv;
  }

  char* getBuffer() {
    return this->buffer;
  }

  char* getHost() {
    return this->host;
  }

private:
  int listening;
  sockaddr_in hint;
  int clientSocket;
  sockaddr_in client;
  socklen_t clientSize;
  char host[NI_MAXHOST];
  char svc[NI_MAXSERV];
  char buffer[4096];
};

class Client {
public:
  Client(char* ip, int port) {
    this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->sockfd == -1) {
      cout << "socket connection failed" << endl;
      exit(0);
    }
    bzero(&this->servaddr, sizeof(servaddr));

    this->servaddr.sin_family = AF_INET;
    this->servaddr.sin_addr.s_addr = inet_addr(ip);
    this->servaddr.sin_port = htons(port);

    if (connect(this->sockfd, (sockaddr*)&this->servaddr, sizeof(servaddr)) != 0) {
      cout << "cant connect" << endl;
    } else {
      cout << "connected to server" << endl;
    }
  }

  ~Client() {
    close(this->sockfd);
  }

  void sendMsg(char* msg) {
    write(this->sockfd, msg, sizeof(msg));
  }

  void readMsg(char* str) {
    read(this->sockfd, str, sizeof(str));
  }

private:
  int sockfd, connfd;
  sockaddr_in servaddr, cli;
};

int main(int argc, char *argv[]) {
  if ((string)argv[1] == "recieve") {
    Listener listener = Listener(atoi(argv[2]));
    while (true) {
      int bytesRecv = listener.getBytesRecv();
      char* buffer = listener.getBuffer();
      char* host = listener.getHost();
      if (bytesRecv == 0) {
        cout << host << " disconnected" << endl;
        break;
      }
      for (int i = 0, j = 0; buffer[i] != '\0'; i++) {
        if(buffer[i] != '\n')
          buffer[j++] = buffer[i];
      }
      for (int i = 0, j = 0; buffer[i] != '\0'; i++) {
        if(buffer[i] != '\r')
          buffer[j++] = buffer[i];
      }

      cout << host << ": " << string(buffer,0,bytesRecv+1) << endl;
      listener.serverSend(buffer,bytesRecv+1);
    }
  } else if ((string)argv[1] == "send") {
    Client client = Client((char*)argv[2],atoi(argv[3]));
    char buffer[4096];
    int n;
    for (;;) {
      bzero(buffer, sizeof(buffer));
      cout << "(Connected:" << argv[2] << ") ";
      n = 0;
      while ((buffer[n++] = getchar()) != '\n')
          ;
      client.sendMsg(buffer);

      if ((strncmp(buffer, "exit", 4)) == 0) {
          printf("Client Exit...\n");
          break;
      }
    }
  }

  return 0;
}
