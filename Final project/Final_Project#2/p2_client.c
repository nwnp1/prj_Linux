#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h> 
#include <sys/wait.h>

#define SERVER_IP		"220.149.128.103"
#define SERVER_PORT		4675


#define LOGIN_OK		"LOGIN_OK"
#define LISTEN_CNT		10


///////////////////////////////////////////////////////////////////////////////////
// Client connection 
int client_connect();

// stdin (remove (ASCII Code)LF 
int rm_lf(char *pBuf, int len);
//////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[ ])
{
	pid_t pid;
	int sockfd;
	struct sockaddr_in dest_addr;

	int rcv_byte;
	char buf[512];
	char id[20];
	char pw[20];
	char msg[512];
	char name_tag[512];

	////////////////////////////////////////////////////////
	sockfd = client_connect(SERVER_IP, SERVER_PORT);
	if (sockfd == -1)
	{
		perror("Client Socket Fail!!");
		exit(1);
	}

	////////////////////////////////////////////////////////
	printf("----------------------------------------\n");
	printf("---------- START CHAT PROGRAM ----------\n");
	printf("----------------------------------------\n");

	////////////////////////////////////////////////////////
	printf("ID : ");
	scanf("%s", id);
	send(sockfd, id, strlen(id) + 1, 0);
	
	////////////////////////////////////////////////////////
	printf("PW : ");
	scanf("%s", pw);
	send(sockfd, pw, strlen(pw) + 1, 0);

	////////////////////////////////////////////////////////
	rcv_byte = recv(sockfd, buf, sizeof(buf), 0);

	if (strcmp(buf, LOGIN_OK) != 0)
	{
		printf("### LOGIN FAIL(%s) PROGRAM EXIT!! ###\n", buf);

		close(sockfd);
		return 0;
	}
	else
	{
		printf("Log-in Success! - Welcome to MyChat\n");
		printf("--------------------Chatting Room--------------------\n");
	}
	
	////////////////////////////////////////////////////////
	fgets(buf, sizeof(buf), stdin);

	////////////////////////////////////////////////////////
	if ((pid = fork()) == 0)
	{
		char recv_buf[512];
		while (1)
		{
			memset(recv_buf, 0, sizeof(recv_buf));
			if (read(sockfd, recv_buf, sizeof(recv_buf)) <= 0)
			{
				printf("Disconnect server\n");
				close(sockfd);
				exit(0);
			}
			else
			{
				printf("%s", recv_buf);
				printf(">>");
				fflush(stdout);
			}
		}
	}
	////////////////////////////////////////////////////////
	else
	{
		sprintf(name_tag,"[%s] : ",id);
		while (1)
		{
			////////////////////////////////////
			printf(">>");
			fgets(buf, sizeof(buf), stdin);

			////////////////////////////////////
			if (write(sockfd, buf, sizeof(buf)) > 0)
			{
				////////////////////////////////////////////////
				write(sockfd, name_tag, sizeof(name_tag));
			}
			//////////////////////////////////////////////
			else
			{
				break;
			}
		}
	}

	close (sockfd);
	return 0;
}


int client_connect(char *pIP, int nPort)
{
	int sockfd;
	struct sockaddr_in dest_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("Client-Socket Fail!!");
		exit(1);
	}
	

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(nPort);
	dest_addr.sin_addr.s_addr = inet_addr(pIP);

	memset(&(dest_addr.sin_zero), 0, 8);
	///////////////////////////////////////////////////////////////////////////////////
	if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("Client Connect Function  Fail!!");
		return -1;
	}
	///////////////////////////////////////////////////////////////////////////////////

	
	return sockfd;
}


int rm_lf(char *pBuf, int len)
{
	for (int i = 0; i < len; i++)
	{
		// CR/LF »èÁ¦
		if ((pBuf[i] == 10) || (pBuf[i] == 13))
		{
			pBuf[i] = '\0';
		}
	}
}
