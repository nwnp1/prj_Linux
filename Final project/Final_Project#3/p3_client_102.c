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
#define SERVER_PORT		4650

#define P2P_SERVER_IP 	"220.149.128.102"
#define P2P_PORT		"4652"


#define LOGIN_OK		"LOGIN_OK"
#define LISTEN_CNT		10

#define END				"END"


///////////////////////////////////////////////////////////////////////////////////
typedef struct _p2p_info
{
	char id[20];
	char ip[16];
	char port[20];

}P2PInfo;
P2PInfo p2pInfo;

///////////////////////////////////////////////////////////////////////////////////
// Accept 설정 (bind, listen 포함)
int server_accept(int nPort);

// Client connection 설정
int client_connect();

// p2p server 처리 Process function
int p2p_server();

// p2p client 처리 Process function
int p2p_client();

// 디렉토리내의 파일 정보 리스트를 가져오는 함수
void list_dir(char *path, int sockfd);

// 디렉토리 정보를 수신하여 출력하는 함수
void recv_dirs(int sockfd);

// p2p 파일 전송 처리 함수
int file_transfer(int sockfd, char *pFile);

// p2p 파일 수신 처리 함수
int file_recv(int sockfd, char *pFile);

// stdin (표준 입출력시 포함되는 (ASCII Code)LF 문자 삭제함수
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
	// Server와 TCP로 Connection 처리 
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
	// 사용자로부터 ID를 입력 받아 Server에 송신
	printf("ID : ");
	scanf("%s", id);
	send(sockfd, id, strlen(id) + 1, 0);
	
	////////////////////////////////////////////////////////
	// 사용자로부터 Password를 입력 받아 Server에 송신
	printf("PW : ");
	scanf("%s", pw);
	send(sockfd, pw, strlen(pw) + 1, 0);

	////////////////////////////////////////////////////////
	// 로그인 결과를 수신 
	rcv_byte = recv(sockfd, buf, sizeof(buf), 0);
	

	if (strcmp(buf, LOGIN_OK) != 0)
	{
		printf("### LOGIN FAIL!!, PROGRAM EXIT!! ###\n");
		close(sockfd);
		return 0;
	}
	else
	{
		printf("Log-in Success! - Welcome to MyChat\n");
		printf("--------------------Chatting Room--------------------\n");
	}
	
	////////////////////////////////////////////////////////
	// P2P 통신을 위한 IP/Port 송신
	send(sockfd, P2P_SERVER_IP, strlen(P2P_SERVER_IP), 0);
	send(sockfd, P2P_PORT, strlen(P2P_PORT), 0);
	////////////////////////////////////////////////////////

	fgets(buf, sizeof(buf), stdin);

	////////////////////////////////////////////////////////
	// child process (Data 수신 처리)
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

				////////////////////////////////////////////////////////
				// FILE 수신 시 P2P 를 위한 fork()
				if (strncmp("FILE", recv_buf, strlen("FILE")) == 0)
				{
					pid_t p2p_pid;

					memset(&p2pInfo, 0x0, sizeof(p2pInfo));
					memset(recv_buf, 0x0, sizeof(recv_buf));

					////////////////////////////////////////////////////////
					// P2P Server의 IP/Port를 수신
					read(sockfd, recv_buf, sizeof(recv_buf));
					sprintf(p2pInfo.ip, "%s", recv_buf);

					memset(recv_buf, 0x0, sizeof(recv_buf));
					read(sockfd, recv_buf, sizeof(recv_buf));
					sprintf(p2pInfo.port, "%s", recv_buf);
					//printf("P2P Server IP[%s]:PORT[%s]\n", p2pInfo.ip, p2pInfo.port);
					////////////////////////////////////////////////////////

					// Child Process는 P2P Clent 처리
					if ((p2p_pid = fork()) == 0)
					{
						p2p_client();
					}

					// Parent Process를 기존 수신 작업을 수행
				}
				else
				{ 
					printf(">>");
					fflush(stdout);
				}
			}
		}
	}
	////////////////////////////////////////////////////////
	// parent process (키보드 입력을 받은 후 Server에 송신 처리)
	else
	{
		sprintf(name_tag,"[%s] : ",id);
		while (1)
		{
			////////////////////////////////////
			// 입력 모드 및 표준 입력(키보드) 처리
			printf(">>");
			fgets(buf, sizeof(buf), stdin);

			////////////////////////////////////
			// 입력된 Data를 Server로 송신
			if (write(sockfd, buf, sizeof(buf)) > 0)
			{
				////////////////////////////////////////////////
				// [FILE-사용자]를 입력 받으면 P2P Server를 실행
				if (strncmp("[FILE-", buf, strlen("[FILE-")) != 0)
				{ 
					////////////////////////////////////////////////
					// [FILE-사용자]가 아닐 경우  name_tag를 서버로 송신
					write(sockfd, name_tag, sizeof(name_tag));
				}
				else
				{
					pid_t p2p_pid;

					//////////////////////////////////////////
					// Child Process는 P2P Server 실행
					if ((p2p_pid = fork()) == 0)
					{
						p2p_server();
					}

					//////////////////////////////////////////////////////////
					// Parent Process는 Child Process가 종료 될때까지 기다린다.
					// 표준 입력을 막기위해 (P2P서버에서 표준 입력을 사용
					else
					{
						int status;
						wait(&status);
					}
				}
			}
			//////////////////////////////////////////////
			// 송신 실패시 Socket를 Close하고 프로세스 종료
			else
			{
				break;
			}
		}
	}

	close (sockfd);
	return 0;
}

int server_accept(int nPort)
{
	int sockfd, new_fd;
	struct sockaddr_in svr_addr;
	struct sockaddr_in their_addr;

	int val = 1;
	unsigned intsin_size;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("P2P-Server Socket Fail!!");
		return -1;
	}
	//else printf("P2P-socket() sockfd is OK...\n");

	svr_addr.sin_family = AF_INET;
	svr_addr.sin_port = htons(nPort);
	svr_addr.sin_addr.s_addr = INADDR_ANY;

	memset(&(svr_addr.sin_zero), 0, 8);

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&val, sizeof(val)) < 0) {
		perror("Setsockopt Function Fail!!");
		close(sockfd);
		return -1;
	}

	if (bind(sockfd, (struct sockaddr *)&svr_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("BIND FAIL!!");
		return -1;
	}


	if (listen(sockfd, LISTEN_CNT) == -1)
	{
		perror("LISTEN FAIL!!");
		return -1;
	}


	return sockfd;

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
	//else printf("Client-socket() sockfd is OK...\n");


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
	//else printf("Client-connect() is OK...\n\n");
	///////////////////////////////////////////////////////////////////////////////////


	
	return sockfd;
}


int p2p_server()
{
	int p2p_sockfd, p2p_new_fd;
	struct sockaddr_in p2p_their_addr;
	unsigned int p2p_sin_size;

	char send_buf[512] = { 0, };
	char save_file[512] = { 0, };

	p2p_sockfd = server_accept(atoi(P2P_PORT));

	//p2p_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (p2p_sockfd == -1)
	{
		perror("P2P Server Create Error !!");
		exit(1);
	}
	//else printf("P2P-socket() sockfd is OK...\n");

	while (1)
	{
		/*************** accept ******************/
		p2p_sin_size = sizeof(struct sockaddr_in);
		p2p_new_fd = accept(p2p_sockfd, (struct sockaddr*)&p2p_their_addr, &p2p_sin_size);
		if (p2p_new_fd < 0)
		{
			//printf("accept() error lol!\n");
			return -1;
		}
		else
		{
			while (1)
			{
				/////////////////////////////////////
				// 현재 디렉토리 구하기
				char curr_dir[1024] = { 0, };
				getcwd(curr_dir, sizeof(curr_dir));
				/////////////////////////////////////

				printf("Select file\n");
				recv_dirs(p2p_new_fd);

				printf(">>");
				memset(send_buf, 0x0, sizeof(send_buf));
				fgets(send_buf, sizeof(send_buf), stdin);

				write(p2p_new_fd, send_buf, sizeof(send_buf));
				rm_lf(send_buf, sizeof(send_buf));

				strcat(save_file, send_buf);
				sprintf(save_file, "%s/%s", curr_dir, send_buf);
				file_recv(p2p_new_fd, save_file);

				break;
			}

			close(p2p_new_fd);
			close(p2p_sockfd);

			printf(">>File Save Success!!\n");
			exit(0);
		}
	}
}

int p2p_client()
{
	int p2p_cli_fd;
	char recv_buf[512] = { 0, };

	char send_path[255] = { 0, };
	char send_name[255] = { 0, };
	char send_file[512] = { 0, };

	p2p_cli_fd = client_connect(p2pInfo.ip, atoi(p2pInfo.port));
	if (p2p_cli_fd == -1)
	{
		perror("P2P Client-connect() error lol");
		return -1;
	}
	//else printf("Client-connect() is OK...\n\n");

	while (1)
	{
		/////////////////////////////////////
		// 현재 디렉토리 구하기
		char curr_dir[1024] = { 0, };
		getcwd(curr_dir, sizeof(curr_dir));
		/////////////////////////////////////

		list_dir(curr_dir, p2p_cli_fd);

		memset(recv_buf, 0x0, sizeof(recv_buf));
		if (read(p2p_cli_fd, recv_buf, sizeof(recv_buf)) <= 0)
		{
			printf("Disconnect server\n");
			close(p2p_cli_fd);
			exit(0);
		}
		else
		{
			rm_lf(recv_buf, sizeof(recv_buf));
			sprintf(send_file, "%s/%s", curr_dir, recv_buf);
			file_transfer(p2p_cli_fd, send_file);

			printf(">>File transfer Success!!\n");
			break;
		}
	}

	close(p2p_cli_fd);
	
}

void list_dir(char *path, int sockfd)
{

	struct dirent *entry;
	DIR *dir = opendir(path);
	if (dir == NULL) {
		return;
	}
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".") != 0) {
			printf("%s\n", entry->d_name);
			send(sockfd, entry->d_name, strlen(entry->d_name) + 1, 0);
			usleep(100000);
		}
	}

	send(sockfd, END, strlen(END) + 1, 0);
	printf("\n");

	closedir(dir);
}

void recv_dirs(int sockfd)
{
	char recv_buf[255];

	while (1)
	{
		memset(recv_buf, 0x0, sizeof(recv_buf));
		if (read(sockfd, recv_buf, sizeof(recv_buf)) <= 0)
		{
			return;
		}
		else
		{
			if (strncmp(END, recv_buf, strlen(END)) == 0)
			{
				return;
			}
			else
			{
				printf("%s\n", recv_buf);
			}
		}
	}

}

int file_transfer(int sockfd, char *pFile)
{
	char fild_path[512] = { 0, };
	char send_buf[512] = { 0, };
	int fild_fd;
	int len;

	sprintf(fild_path, "%s", pFile);

	fild_fd = open((const char *)pFile, O_RDONLY);
	if (fild_fd == -1)
	{
		printf("File open error : %s\n", fild_path);
		close(fild_fd);
		return 0;
	}

	while ((len = read(fild_fd, send_buf, sizeof(send_buf))) != 0)
	{
		send(sockfd, send_buf, len, 0);
	}

	close(fild_fd);
	return 0;

}

int file_recv(int sockfd, char *pFile)
{
	int file_fd;
	int recv_byte;
	char recv_buf[512];
	int len;

	file_fd = open((const char *)pFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (file_fd == -1)
	{ 
		printf("File save error : %s\n", pFile);
	}

	while ((len = read(sockfd, recv_buf, sizeof(recv_buf))) != 0)
	{
		write(file_fd, recv_buf, len);
	}

	close(file_fd);
}

int rm_lf(char *pBuf, int len)
{
	for (int i = 0; i < len; i++)
	{
		// CR/LF 삭제
		if ((pBuf[i] == 10) || (pBuf[i] == 13))
		{
			pBuf[i] = '\0';
		}
	}
}
