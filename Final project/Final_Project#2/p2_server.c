#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define SERV_PORT		4675
#define LISTEN_CNT		10

#define WORNG_ID_MSG "WORNG ID\n"
#define WORNG_PW_MSG "WORNG PW\n"

#define USER1_ID	"user1"
#define USER1_PW	"passwd1"
#define USER2_ID	"user2"
#define USER2_PW	"passwd2"

#define LOGIN_OK	"LOGIN_OK"


int cli_fd_arr[10] = { 0, };

void* thread_client_proc(void* num)
{
	char id[20];
	char pw[20];

	int cli_sockfd;
	int index = *(int*)num;

	int k;

	char buf[512];
	char send_buf[512];
	char name_tag[512];
	int read_byte;

	cli_sockfd = cli_fd_arr[index];
	
	///////////////////////////////////////////////////////////
	recv(cli_sockfd, id, sizeof(id), 0);
	recv(cli_sockfd, pw, sizeof(pw), 0);
	///////////////////////////////////////////////////////////


	if (((strcmp(id, USER1_ID) == 0) && (strcmp(pw, USER1_PW) == 0)) || 
		((strcmp(id, USER2_ID) == 0) && (strcmp(pw, USER2_PW) == 0)))
	{
		printf("=======================\nUSER Information\nID :%s, PW :%s\n=======================\n\n", id, pw);

		if (strcmp(id, USER1_ID) == 0)
		{
			if (strcmp(pw, USER1_PW) == 0)
			{
				printf("LOGIN SUCCESS\n");
				send(cli_sockfd, LOGIN_OK, strlen(LOGIN_OK) + 1, 0);
			}
			else
			{
				printf(WORNG_PW_MSG);
				send(cli_sockfd, WORNG_PW_MSG, strlen(WORNG_PW_MSG) + 1, 0);

				close(cli_sockfd);
				cli_fd_arr[index] = 0;

				return 0;
			}
		}
		else if ((strcmp(id, USER1_ID) != 0) && (strcmp(id, USER2_ID) != 0))
		{
			printf(WORNG_ID_MSG);
			send(cli_sockfd, WORNG_ID_MSG, strlen(WORNG_ID_MSG) + 1, 0);

			close(cli_sockfd);
			cli_fd_arr[index] = 0;

			return 0;
		}
		else if (strcmp(id, USER2_ID) == 0)
		{
			if (strcmp(pw, USER2_PW) == 0)
			{
				printf("LOGIN SUCCESS\n");
				send(cli_sockfd, LOGIN_OK, strlen(LOGIN_OK) + 1, 0);
			}
			else
			{
				printf(WORNG_PW_MSG);
				send(cli_sockfd, WORNG_PW_MSG, strlen(WORNG_PW_MSG) + 1, 0);

				close(cli_sockfd); 
				cli_fd_arr[index] = 0;

				return 0;
			}
		}

		//////////////////////////////////////////////////////////////////////////////
		while (1)
		{
			memset(buf, 0x0, sizeof(buf));

			if ((read_byte = read(cli_sockfd, buf, 512)) > 0)
			{ 
				read(cli_sockfd, name_tag, 512);

				strcat(name_tag, buf);
				strcpy(buf, name_tag);

				printf("%s\n", buf);

				for (k = 0; k < 10; k++)
				{
					if ((cli_fd_arr[k] != 0) && (cli_fd_arr[k] != cli_sockfd))
					{
						write(cli_fd_arr[k], buf, sizeof(buf));
					}
				}
			}
			else if (read_byte <= 0)
			{
				close(cli_sockfd);
				cli_fd_arr[index] = 0;

				return 0;
			}
		}
		//////////////////////////////////////////////////////////////////////////////
	}
	else
	{
		printf(WORNG_ID_MSG);
		send(cli_sockfd, WORNG_ID_MSG, strlen(WORNG_ID_MSG) + 1, 0);

		close(cli_sockfd);
		cli_fd_arr[index] = 0;

		return 0;
	}
}

/////////////////////////////////////////////////////////////////////////
// Main Function
/////////////////////////////////////////////////////////////////////////
int main(void)
{
	int sockfd, new_fd;

	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	unsigned int sin_size;

	int val = 1;

	pthread_t thread[10];

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		perror("Server Socket Fail!!");
		exit(1);
	}
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(SERV_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	memset(&(my_addr.sin_zero), 0, 8);
	
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&val, sizeof(val)) < 0){
		perror("Setsockopt Function Fail!!");
		close(sockfd);
		return -1;
	}

	if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) ==-1)
	{
		perror("BIND FAIL!!");
		exit(1);
	}
	

	if (listen(sockfd, LISTEN_CNT) == -1)
	{
		perror("LISTEN FAIL!!");
		exit(1);
	}
	
	////////////////////////////////////////////////////////
	printf("------------------------------------------\n");
	printf("---------- START SERVER PROGRAM ----------\n");
	printf("------------------------------------------\n");

	while (1)
	{
		
		sin_size = sizeof(struct sockaddr_in);
		new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size);
		if (new_fd < 0)
		{
			printf("ACCEPT FAIL!!\n");
			return -1;
		}
		else
		{	
			for (int i = 0; i < 10; i++)
			{
				if (cli_fd_arr[i] == 0)
				{
					cli_fd_arr[i] = new_fd;

					pthread_create(&thread[i], NULL, &thread_client_proc, &i);
					break;
				}
			}
		}
	}

	close(new_fd);
	close(sockfd);
	return 0;
}
