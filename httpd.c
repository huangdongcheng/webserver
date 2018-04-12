#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int startup(u_short *);
int get_line(int, char *, int);
void error_die(const char *);


int main(void)
{
	int server_socket = -1, client_socket = -1;
	u_short port = 0;
	struct sockaddr_in client_name;
	int client_name_len = sizeof(client_name);

	server_socket = startup(&port);
	printf("httpd running on port %d\n", port);

	while(1) {
		client_socket = accept(server_soket, (struct sockaddr *)&client_name, &client_name_len);

		if (client_socket == -1) {
			error_die("accept");
		}

		accept_request(client_socket);
	}
	
	close(server_socket);

	return 0 ;
}

int get_line(int socket, char *buf, int size)
{
	int i = 0, n = 0;
	char c = '\0';

	while((i < size - 1) && (c != '\n')) {
		n = recv(socket, &c, 1, 0);

		if (n > 0) {
			if (c == '\r') {
				n = recv(socket, &c, 1, MSG_PEEK);
				if (n > 0 && c == '\n') {
					recv(socket, &c, 1, 0);
				} else {
					n = '\n';
				}
			}
		} else {
			c = '\n';
		}
		buf[i]= c;
		i++;
	}

	buf[i] = '\0';

	return n;
}

int startup(u_short *port)
{
	int httpd = 0;
	struct sockaddr_in address;

	httpd = socket(AF_INET, SOCK_STREAM, 0);
	if (httpd == -1) {
		error_die("socket");
	}

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(*port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(httpd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		error_die("bind");
	}

	if (*port == 0) {
		int addresslen = sizeof(address);

		if (getsockname(httpd, (struct sockaddr *)&address, &addresslen) == -1) {
			error_die("getsockname");
		}

		*port = ntohs(address.sin_port);
	}

	if (listen(httpd, 5) < 0) 
	{
		error_die("listen");
	}

	return httpd;
}

void error_die(const char *message) {
	perror(message);
	exit(1);
}

void accept_request(int socketfd)
{
	char buf[1024] = "";
	char method[1024] = "";
	char url[1024] = "";
	char path[512] = "";
	int numchars = 0;

	size_t i = 0, j = 0;

	struct stat st;

	int cgi = 0;
	char *query_string = NULL;

	numchars = get_line(socketfd, buf, sizeof(buf));

	while(!isspace(buf[j]) && (i < sizeof(method) - 1)){
		method[i] = buf[j];
		i++;j++;
	}
	method[i] = '\0';

	if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
		unimplemented(socketfd);
		return;
	}

	if (strcasecmp(method, "POST")) {
		cgi = 1;
	}

	i = 0;

	while(isspace(buf[j]) && (j < sizeof(buf))){
		j++;
	}

	while(!isspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))) {
		url[i] = buf[j];
		i++;j++;
	}
	url[i] = '\0';

	if (strcasecmp(method, "GET") == 0) {
		query_string = url;

		while((*query_string != '?') && (*query_string != '\0')) {
			query_string++;
		}

		if (*query_string == '?') {
			cgi = 1;
			*query_string = '\0';
			query_string++;
		}
	}

	sprintf(path, "htdoc%s", url);

	if (path[strlen(path) - 1] == '/') {
		strcat(path, "index.html");
	}

	if (stat(path, &st) == -1) {
		while ((numchars > 0) && strcmp("\n", buf)) {
			numchars = get_line(socketfd, buf, sizeof(buf));
		}

		not_found(socketfd);
		return;
	}

	if ((st.st_mode & S_IFMT) == S_IFDIR) {
		strcat(path, "/index.html");
	}

	if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH)) {
		cgi = 1;
	}

	if (!cgi) {
		server_file(client, path);
	} else {

	}
}

void unimplemented(int client)
{
 	char buf[1024];

 	sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, SERVER_STRING);
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "Content-Type: text/html\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "</TITLE></HEAD>\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "</BODY></HTML>\r\n");
 	send(client, buf, strlen(buf), 0);
}

void not_found(int client)
{
 	char buf[1024];

 	sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "Content-Type: text/html\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "your request because the resource specified\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "is unavailable or nonexistent.\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "</BODY></HTML>\r\n");
 	send(client, buf, strlen(buf), 0);
}

void server_file(int socketfd, const char *filename)
{
	FILE *resource = NULL;
	int numchars = 1;
	
	resource = fopen(filename, "r");
	if (resource == NULL) {
		not_found(socketfd);
		return;
	}

	headers(socketfd, resource);

	cat(socketfd, resource);

	fclose(resource);
}

void not_found(int client)
{
 	char buf[1024];

 	sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "Content-Type: text/html\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "your request because the resource specified\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "is unavailable or nonexistent.\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "</BODY></HTML>\r\n");
 	send(client, buf, strlen(buf), 0);
}

void headers(int socketfd, const char *filename)
{
	char buf[1024] = "";

	strcpy(buf, "HTTP/1.0 200 OK\r\n");
	send(socketfd, buf, strlen(buf), 0);
	strcpy(buf, "Content-type: text/html\r\n");
	send(socketfd, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(socketfd, buf, strlen(buf), 0);
}

void cat(int socketfd, FILE *resource)
{
	char buf[1024] = "";
	fgets(buf, sizeof(buf), resource);
	while (!feof(resource)) {
		send(socketfd, buf, strlen(buf), 0);
		fgets(buf, sizeof(buf), resource);
	}
}
