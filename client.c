#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define MAXDATASIZE 512 // max number of bytes we can get at once 
#define PORT 5555


int main()
{
    struct hostent *host_entity;
    struct sockaddr_in serv_addr;
    char input_client[MAXDATASIZE], input_x_client[MAXDATASIZE];
    int sock, maxfd, addrlen = sizeof(serv_addr), numbytes;
    fd_set master, read_fds;

    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT);
    host_entity = gethostbyname("localhost");
    bcopy((char*)host_entity -> h_addr, (char*)&serv_addr.sin_addr.s_addr, host_entity ->h_length);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    
    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    FD_ZERO(&master);
    FD_SET(sock, &master);
    FD_SET(0, &master); // 0 for keyboard
    maxfd = sock;

    while(true) // client loop
    {
        read_fds = master;
        select(maxfd + 1, &read_fds, NULL, NULL, NULL);
        if (FD_ISSET(0, &read_fds))
        {
            fgets(input_client, MAXDATASIZE, stdin);
            send(sock, input_client, strlen(input_client) + 1, 0);
        } else {
            numbytes = recv(sock, input_x_client, MAXDATASIZE, 0);
            input_x_client[numbytes-1] = 0;
            puts(input_x_client);
            // close(socket); can't close socket here
        }
        
    }
	return 0;
}
