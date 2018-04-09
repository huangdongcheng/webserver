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

int main(void)
{
	
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
