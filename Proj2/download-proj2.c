#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdbool.h>



#include <sys/socket.h> 
#include <netinet/in.h>




#define MAX_LENGTH 5000

#define SERVER_PORT 21




char username[100], password[100], host[100], path[200], ip[100], file[100];




int parse_data(char *url) {

    
    int result = sscanf(url, "ftp://%99[^:]:%99[^@]@%99[^/]/%99s", username, password, host, path);
    strcpy(file, strrchr(url, '/') + 1);


    if (result == 4) {
        printf("Username: %s\n", username);
        printf("Password: %s\n", password);
        printf("Host: %s\n", host);
        printf("Resource: %s\n", path);
        printf("File: %s\n", file);
    } else {
        printf("Failed to parse the URL.\n");
        return 1;
    }

    
    struct hostent *h;

    if (strlen(host) == 0) return -1;
    if ((h = gethostbyname(host)) == NULL) {
        printf("Invalid hostname '%s'\n", host);
        exit(-1);
    }
    strcpy(ip, inet_ntoa(*((struct in_addr *) h->h_addr)));


    printf("Ip: %s\n", ip);
    return 0;

}



int create_socket(char *ip_p, int port) {

    int sockfd;
    struct sockaddr_in server_addr;

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip_p);  
    server_addr.sin_port = htons(port); 

    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }

    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }


    

    return sockfd;
}


int authenticate(const int socket, const char* user, const char* pass) {

    char userCommand[5+strlen(username)+1]; sprintf(userCommand, "user %s\n", user);
    char passCommand[5+strlen(password)+1]; sprintf(passCommand, "pass %s\n", pass);

    if (write(socket, userCommand, strlen(userCommand)) == -1){
        printf("Error in writing to socket username\n");
        return 1;
    }

    sleep(1);
    char buffer1[MAX_LENGTH];
    int bytes = 0;
    
    bytes = read(socket, buffer1, MAX_LENGTH);
    printf("%s",buffer1);    

    if (write(socket, passCommand, strlen(passCommand)) == -1){
        printf("Error in writing to socket password\n");
        return 1;
    }
    
    char buffer2[MAX_LENGTH];
    bytes = read(socket, buffer2, MAX_LENGTH);
    printf("%s",buffer2);

    return 0;
}




int activate_passiveMode(int socket, char *ip, int *port) {

    char buffer[MAX_LENGTH];


    write(socket, "pasv\n", 5);



    int bytes = 0;
    bytes = read(socket, buffer, MAX_LENGTH);

    printf("%s",buffer);
    sleep(2);




    int ip1, ip2, ip3, ip4, port1, port2;


    int result = sscanf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).\n",
                        &ip1, &ip2, &ip3, &ip4, &port1, &port2);

 
    if (result == 6) {
        *port = port1 * 256 + port2;
    } else {
        printf("Error: Unable to extract values.\n");
        printf("%i\n",ip1);
        printf("%i\n",ip2);
        printf("%i\n",ip3);
        printf("%i\n",ip4);
        return -1;
    }


    sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);


    return 0;
}



int request_resource(const int socket, char *resource) {
    char fileCommand[5+strlen(resource)+1];

    sprintf(fileCommand, "retr %s\n", resource);
    if (write(socket, fileCommand, sizeof(fileCommand)) == -1){
        return -1;
    }

    char byte;
    while (read(socket, &byte, 1) > 0){
        printf("%c",byte);
        if (byte == '\n') break;
    }

    while (read(socket, &byte, 1) > 0){
        printf("%c",byte);
        if (byte == '\n') break;
    }

    return 0;
}

int retrieve_resource(const int socketB, char *filename) {

    FILE *fd = fopen(filename, "wb");
    if (fd == NULL) {
        printf("Error opening or creating file '%s'\n", filename);
        return -1;
    }

    

    char buffer[MAX_LENGTH];
    int bytes = 0;

    while (true) {
        bytes = read(socketB, buffer, MAX_LENGTH);
        if (fwrite(buffer, bytes, 1, fd) < 0) return -1;
        if (bytes == 0) break;
    }

    

    fclose(fd);
    return 0;
}


int main(int argc, char *argv[]) {


    if (argc != 2) {
        printf("Invalid Input.\n");
        printf("Usage: ./download ftp://<user>:<password>@<host>/<url-path>\n");
        exit(-1);
    }

    //"ftp://[<username>:<password>@]<ftp.example.com>/<path/to/file.txt>"

    char *input = argv[1];


    if (parse_data(input) != 0){
        printf("Error in parsing data\n");
        exit(-1);
    }

    printf("\nSocketA: %s, %i\n\n",ip, SERVER_PORT);
    int socketA;
    if ((socketA = create_socket(ip, SERVER_PORT)) < 0){
        printf("Error in creating socketA\n");
        exit(-1);
    }

    char byte;
    while (read(socketA, &byte, 1) > 0){
        printf("%c",byte);
        if (byte == '\n') break;
    }


    if (authenticate(socketA, username, password) != 0){
        printf("Error in authentication\n");
        exit(-1);
    }


    int port;
    char ip_server[100];


    if (activate_passiveMode(socketA, ip_server, &port) != 0) {
        printf("Error in setting passive mode\n");
        exit(-1);
    }

    
    printf("\nSocketB: %s, %i\n\n",ip_server,port);
    int socketB = create_socket(ip_server, port);
    if (socketB < 0) {
        printf("Socket to '%s:%d' failed\n", ip_server, port);
        exit(-1);
    }





    if (request_resource(socketA, path) != 0){
        printf("Error in requesting resource\n");
        exit(-1);
    }


    if (retrieve_resource(socketB, file) != 0){
        printf("Error in retrieving resource\n");
        exit(-1);
    }

    
    write(socketA, "quit\n", 5);
    write(socketB, "quit\n", 5);
    close(socketA);
    close(socketB);


    return 0;
}


