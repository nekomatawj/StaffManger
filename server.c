#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>
#include <sqlite3.h>
#define N 128
#define DATABASE "./Student.db"
//检测错误的宏函数
#define DETECT_ERR(str) \
	do{\
		if(ret == -1){\
			perror(str);\
			close(client_t->accptfd);\
			return;\
		}\
	}while(0);

typedef struct history_info{
	char name[N];
	char text[N];
	char time[N];
}history_info;

typedef struct guest_msg{
	char text[N];  //数据信息
	char pwd[N];   //员工的登录密码
	char name[N];  //员工姓名
	int balance;   //工资
	int id;        //员工编号
	int item;      //功能选择
	bool admin;    //是否是管理员
}guest_msg;

typedef struct Client{
	struct sockaddr_in client_addr;
	pthread_t thread;
	guest_msg msg;
	history_info info;
	int available;
	int accptfd;
}Client;
/*****************以下是全局变量*******************/
Client client[5];      //每个用户需要的所有信息
int num = 0;           //控制线程数量的参数
sqlite3 *db;

void exec_register(Client* client_t);
void exec_login(Client* client_t);
void exec_query(Client* client_t);
void exec_update(Client* client_t);
void exec_delete(Client* client_t);
void exec_history(Client* client_t);
void exec_quit(Client* client_t);
void *thread_handler(void* client_t);
int main(int argc, const char *argv[])
{
	int sockfd;
	int ret;

	struct sockaddr_in service_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(atoi(argv[2])),
		.sin_addr = {
			.s_addr = inet_addr(argv[1]),
		},
	};
	if(sqlite3_open(DATABASE,&db) != SQLITE_OK){   //打开数据库
		printf("%s\n",sqlite3_errmsg(db));
		return -1;
	}
	//服务器端框架
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("socket");
		return -EAGAIN;
	}

	ret = bind(sockfd, (struct sockaddr*)&service_addr, sizeof(service_addr));
	if(ret == -1){
		perror("bind");
		return -1;
	}

	ret = listen(sockfd, 5);
	if(ret == -1){
		perror("listen");
		return -1;
	}
	while(num < 5){
		int i;         //记录可用的位置
		for(i=0; i<5; i++){
			if(client[i].available == 0)
				break;
		}

		socklen_t cli_len = sizeof(struct sockaddr_in);
		client[i].accptfd = accept(sockfd, (struct sockaddr *)&(client[i].client_addr), &cli_len);
		if(client[i].accptfd == -1){
			perror("accptfd");
		}
		pthread_create(&(client[i].thread), NULL, thread_handler, (void*)(client+i));
	}

	return 0;
}
void *thread_handler(void* args){
	
	int ret;
	Client *client_t = (Client *)args;
	while(1){
		ret = recv(client_t->accptfd, &(client_t->msg), sizeof(client_t->msg), 0);
		switch(client_t->msg.item){
		case 1:
			exec_register(client_t);
			break;
		case 2:
			exec_login(client_t);
			break;
		case 3:
			exec_quit(client_t);
			pthread_exit(0);
			break;
		case 4:
			exec_query(client_t);
			break;
		case 5:
			exec_update(client_t);
			break;
		case 6:
			exec_delete(client_t);
			break;
		case 7:
			exec_history(client_t);
			break;
		case 8:
			exec_quit(client_t);
			pthread_exit(0);
			break;
		}
	}
}
void exec_register(Client* client_t){
	
	int ret = 0;
	char attr_buffer[N];
	char *errmsg;
	sprintf(attr_buffer, "id=%d name='%s' balance=%d",client_t->msg.id, client_t->msg.name, client_t->msg.balance);
	char sql[N];
	sprintf(sql,"insert into StudentList values(%s);",attr_buffer);
	sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	send(client_t->accptfd, &(client_t->msg), sizeof(guest_msg), 0);
	DETECT_ERR("register");
}
void exec_login(Client* client_t){  //也是一个查询

	int ret = 0;
	send(client_t->accptfd, &(client_t->msg), sizeof(guest_msg), 0);
	DETECT_ERR("register");
}
void exec_update(Client* client_t){
	
	int ret = 0;
	send(client_t->accptfd, &(client_t->msg), sizeof(guest_msg), 0);
	DETECT_ERR("register");
}
void exec_delete(Client* client_t){
	
	int ret = 0;
	send(client_t->accptfd, &(client_t->msg), sizeof(guest_msg), 0);
	DETECT_ERR("register");
}
void exec_history(Client* client_t){

}
void exec_quit(Client* client_t){             //资源回收
	close(client_t->accptfd);
	memset(client_t, 0, sizeof(Client));
}
void exec_query(Client* client_t){


}
