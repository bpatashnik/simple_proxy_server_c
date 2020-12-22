#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>

#define PORT 20000
#define IP "10.189.32.119"

int i = 0;
pthread_t tid[15];
void *Handle_Comm(void *);

int main(int argc, char *argv[])
{
    //Browser socket initialization
    int socket_desc, client_sock, c, read_size;
    struct sockaddr_in server, client, my_addr;
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    client_sock = socket(AF_INET, SOCK_STREAM, 0);

    printf("Sockets Created\n");

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, IP, &(server.sin_addr));

    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Bind Failed. Error");
        return 1;
    }

    printf("Bind Completed \n");

    c = sizeof(struct sockaddr_in);

    listen(socket_desc, 15);

    //Connection loop
    while (1)
    {
        printf("Waiting for incoming connections...\n\n");

        fflush(stdout);

        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
        if (client_sock < 0)
        {
            perror("Accept Failed");
            return 1;
        }

        printf("Connection accepted... Creating thread to handle communiction\n\n");

        fflush(stdout);

        //New thread to handle request
        pthread_create(&(tid[i]), NULL, Handle_Comm, &client_sock);

        i++;
        if (i == 15)
            i = 0;
    }
}

void *Handle_Comm(void *Socket)
{
    //
    int send_size, recv_size;
    int err;
    struct sockaddr_in sa;
    char browser_message[20000];
    int Client_Sock = *((int *)Socket);

    //Browser request
    err = recv(*((int *)Socket), browser_message, sizeof(browser_message), 0);
    if (err < 0)
    {
        perror("Browser");
        exit(0);
    }

    printf("Request from Browser:\n%s\n", browser_message);

    //Parse request type
    char *request = strtok(browser_message, " ");

    //If not GET, exit
    int check_request = strcmp(request, "GET");
    if (check_request != 0)
    {
        printf("Request Rejected\n\nThread Exiting\n\n");
        return 0;
    }

    //Parse URL and version
    char *url = strtok(NULL, "\n");

    //Parse host
    char *host = strtok(NULL, "\n");

    //Host name without "Host: " or ending carriage return
    char *host_name = host + 6;
    host_name[strlen(host_name) - 1] = '\0';

    printf("Requested Server: %s\n\n", host_name);

    //Retrieve IP based on host name
    char *IPbuffer;
    struct hostent *host_entry;
    host_entry = gethostbyname(host_name);
    IPbuffer = inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0]));

    //Request to be forwarded to server
    char msgToServer[65535];
    err = sprintf(msgToServer, "%s %s\n%s\r\n\r\n", request, url, host);

    //Create new socket with server
    int my_sock = socket(AF_INET, SOCK_STREAM, 0);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(80);
    inet_pton(AF_INET, IPbuffer, &(sa.sin_addr));

    printf("Attempting to Connect to Server...\n\n");

    int x = connect(my_sock, (struct sockaddr *)&sa, sizeof(sa));
    if (x != 0)
    {
        perror("Connect");
        exit(0);
    }

    printf("Connect Successful\n\n");

    //Send request to server
    err = send(my_sock, msgToServer, 1024, 0);
    if (err < 0)
    {
        perror("Server");
        exit(0);
    }

    //Receive response from server
    err = recv(my_sock, msgToServer, 65535, MSG_WAITALL);
    if (err < 0)
    {
        perror("Server");
        exit(0);
    }

    //Print server response header to console
    char *msgToServerCopy[strlen(msgToServer)];
    strcpy(msgToServerCopy, msgToServer);
    char *serverHeader = strtok(msgToServerCopy, "\r\n\r\n");

    printf("Server Response Header: \n%s\n\n", serverHeader);

    //Forward server response to browser
    err = send(*((int *)Socket), msgToServer, sizeof(msgToServer), 0);
    if (err < 0)
    {
        perror("Browser");
        exit(0);
    }

    printf("Object Sent Back to Browser\n\n");

    printf("Thread Exiting\n\n");

    fflush(stdout);
}
