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
#include <time.h>
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
	char text[1024];
	char time[N];
}history_info;

typedef struct guest_msg{
	char text[1024];  //数据信息
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
	history_info his;
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
int msqlite3_callback(void* args, int ncolum, char **f_value, char **f_name );
void get_system_time(char *timedata);
int main(int argc, const char *argv[])
{
	int sockfd;
	int ret;
	char *  errmsg;
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
	if(sqlite3_exec(db,"create table StudentList(id int,name char,pwd char,balance char);",NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s.\n",errmsg);
	}else{
		printf("create usrinfo table success.\n");
	}
	//时间相关的初始化
	
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
void exec_register(Client* client_t){   //注册完成

	int ret = 0;
	char attr_buffer[N];
	char *errmsg;
	char sql[N] = {};
	char timedata[N] = {};
	get_system_time(timedata);
	//执行历史记录插入工作
	sprintf(sql, "insert into HISTABLE values('root','register','%s');",timedata);
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
		printf("%s\n",errmsg);
	}
	memset(sql, 0, sizeof(sql));

	//执行注册工作
	sprintf(attr_buffer, "  %d ,'%s', '%s',  %d ",client_t->msg.id, client_t->msg.name,client_t->msg.pwd, client_t->msg.balance);
	//sprintf(attr_buffer, " id = %d ,name ='%s', pwd = '%s', balance = %d ",client_t->msg.id, client_t->msg.name,client_t->msg.pwd, client_t->msg.balance);
	sprintf(sql,"insert into StudentList values(%s);",attr_buffer);
	if((sqlite3_exec(db, sql, NULL, NULL, &errmsg)) != SQLITE_OK){
	printf("%s\n",errmsg);
	}
	send(client_t->accptfd, &(client_t->msg), sizeof(guest_msg), 0);
	DETECT_ERR("register");
}
void exec_login(Client* client_t){  //登录完成

	int ret = 0;
	char *errmsg;
	char sql[N];
	char timedata[N] = {};
	get_system_time(timedata);

	//执行历史记录插入工作	
	sprintf(sql, "insert into HISTABLE values('%s','login','%s');",client_t->msg.name, timedata);
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
		printf("%s\n",errmsg);
	}
	memset(sql, 0, sizeof(sql));
	//执行登录工作
	sprintf(sql,"select * from StudentList where name='%s' and pwd='%s';",client_t->msg.name, client_t->msg.pwd);
	memset(client_t->msg.text, 0, sizeof(client_t->msg.text));
	ret = sqlite3_exec(db, sql, msqlite3_callback, (void *)(client_t->msg.text), &errmsg);
	if(ret != SQLITE_OK){
		printf("%s\n",errmsg);
	}
	if(strlen(client_t->msg.text) != 0){
		strcpy(client_t->msg.text, "success");
	}else{
		strcpy(client_t->msg.text, "error");
	}
	send(client_t->accptfd, &(client_t->msg), sizeof(guest_msg), 0);
	DETECT_ERR("login");
}
void exec_update(Client* client_t){   //更新完成

	int ret = 0;
	char *errmsg;
	char sql[N];
	char timedata[N] = {};
	get_system_time(timedata);

	//执行历史记录插入工作	
	sprintf(sql, "insert into HISTABLE values('root','update','%s');",timedata);
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
		printf("%s\n",errmsg);
	}
	memset(sql, 0, sizeof(sql));

	//执行更新工作
	sprintf(sql,"update StudentList set name='%s', balance=%d where id=%d;"
			,client_t->msg.name, client_t->msg.balance, client_t->msg.id);
	memset(client_t->msg.text, 0, sizeof(client_t->msg.text));
	//ret = sqlite3_exec(db, sql, msqlite3_callback, client_t->msg.text, &errmsg);
	//if(ret != SQLITE_OK){
	if((ret = sqlite3_exec(db, sql, msqlite3_callback, client_t->msg.text, &errmsg)) != SQLITE_OK){
		printf("%s\n",errmsg);
		getchar();
		strcpy(client_t->msg.text,"error");
		send(client_t->accptfd, &(client_t->msg), sizeof(guest_msg), 0);
		DETECT_ERR("update");
	}else{
		exec_query(client_t);
	}
}
void exec_delete(Client* client_t){            //删除记录

	int ret = 0;
	char *errmsg;
	char sql[N];
	char timedata[N] = {};
	get_system_time(timedata);

	//执行历史记录插入工作	
	sprintf(sql, "insert into HISTABLE values('root','delete','%s');",timedata);
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
		printf("%s\n",errmsg);
	}
	memset(sql, 0, sizeof(sql));

	//执行删除工作
	sprintf(sql,"delete from StudentList where id=%d;",client_t->msg.id);
	sprintf(client_t->msg.text,"编号\r姓名\r工资\n");
	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){

		strcpy(client_t->msg.text,"error");
		send(client_t->accptfd, &(client_t->msg), sizeof(guest_msg), 0);
		DETECT_ERR("delete");
	}else{
		strcpy(client_t->msg.text,"success");
		send(client_t->accptfd, &(client_t->msg), sizeof(guest_msg), 0);
		DETECT_ERR("delete");
	}
}
void exec_history(Client* client_t){          //历史查询

	char sql[N] = {0};
	char *errmsg;
	char timedata[N] = {};
	sprintf(sql, "select * from HISTABLE;");
	memset(client_t->msg.text, 0, sizeof(client_t->msg.text));
	sprintf(client_t->msg.text, "历史信息为\n");
	if(sqlite3_exec(db, sql, msqlite3_callback, 
				(void *)client_t->his.text, &errmsg) != SQLITE_OK){
	
		printf("%s\n",errmsg);
		strcpy(client_t->msg.text, "error");
		send(client_t->accptfd, &(client_t->his), sizeof(history_info), 0);
	}else{
		printf("执行查询工作\n");
		printf("%s\n",client_t->his.text);
		send(client_t->accptfd, &(client_t->his), sizeof(history_info), 0);
	}

}
void exec_quit(Client* client_t){             //资源回收 退出完成
	close(client_t->accptfd);
	memset(client_t, 0, sizeof(Client));
}
void exec_query(Client* client_t){           //查询完成

	int ret = 0;
	char *errmsg;
	char sql[N];
	char timedata[N] = {};
	get_system_time(timedata);
	//执行历史记录插入工作
	if(client_t->msg.admin == 1){
		sprintf(sql, "insert into HISTABLE values('root','query','%s');",timedata);
		if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
			printf("%s\n",errmsg);
		}
		memset(sql, 0, sizeof(sql));
	}else{
			
		sprintf(sql, "insert into HISTABLE values('%s','query','%s');",client_t->msg.name, timedata);
		if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
			printf("%s\n",errmsg);
		}
		memset(sql, 0, sizeof(sql));
	}

	//执行查询工作
	sprintf(sql,"select id, name, balance from StudentList where id=%d;",client_t->msg.id);
	memset(client_t->msg.text, 0, sizeof(client_t->msg.text));
	sprintf(client_t->msg.text,"编号\t姓名\t工资\n");
	if(sqlite3_exec(db, sql, msqlite3_callback,
				(void *)(client_t->msg.text), &errmsg) != SQLITE_OK){
		memset(client_t->msg.text, 0 ,sizeof(client_t->msg.text));
		strcpy(client_t->msg.text,"error");
		send(client_t->accptfd, &(client_t->msg), sizeof(guest_msg), 0);
		DETECT_ERR("query");
	}else{
		send(client_t->accptfd, &(client_t->msg), sizeof(guest_msg), 0);
		printf("%s\n", client_t->msg.text);
		DETECT_ERR("query");
	}
}

//回调函数解释
//args是client对象中存放文本信息的数组
//回调函数每次都会把直传给f_value中
//每次for循环都会把检索到的属性添加到数组之中
//最终文本数组获得所有信息返回
int msqlite3_callback(void* args, int ncolum, char **f_value, char **f_name ){

	int count = 0;
	char sql[N] = {0};
	char* buffer = (char *)args;
	for(count=0; count<ncolum; count++){
		sprintf(sql,"%s\t", f_value[count]);
		strcat(buffer,sql);
	}
	strcat(buffer,"\n");
	return 0;
}
void get_system_time(char *timedata){

	time_t t;
	struct tm *tp;

	time(&t);
	tp = localtime(&t);
	sprintf(timedata, "%d-%d-%d %d:%d:%d",tp->tm_year+1900, tp->tm_mon,
			tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
	return;
}
