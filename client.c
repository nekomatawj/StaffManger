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
#define N 128
//检测错误的宏函数
#define DETECT_ERR(str) \
do{\
	if(ret == -1){\
		perror(str);\
		close(sockfd);\
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

void register_guest(int sockfd, guest_msg* msg_t);
void login_guest(int sockfd, guest_msg* msg_t);
void send_query(int sockfd, guest_msg* msg_t);
void send_update(int sockfd, guest_msg* msg_t);
void send_delete(int sockfd, guest_msg* msg_t);
void send_history(int sockfd, guest_msg* msg_t);
void show_msg(guest_msg *msg_t);                     //显示结束信息
int main(int argc, const char *argv[])
{
	//tcp/ip框架
	//网络结构体的初始化
	int sockfd;          //存放服务器信息的套接字
	int ret;             //返回值判断
	guest_msg msg;       //存放通信信息的结构体
	history_info hsy;    //存放历史记录的结构体
	struct sockaddr_in SeviceAddr = {
		.sin_family = AF_INET,
		.sin_port = htons(atoi(argv[2])),
		.sin_addr = {
			.s_addr = inet_addr(argv[1]),
		},
	};
	memset(&msg, 0, sizeof(msg));
	memset(&hsy, 0, sizeof(hsy));
	//生成套接字
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd < 0){
		perror("socket");
		return -EAGAIN;
	}
	//绑定网络结构体
	ret = bind(sockfd, (struct sockaddr *)&SeviceAddr, sizeof(SeviceAddr));
	if(ret == -1){
		perror("bind");
		return -1;
	}
	//连接到服务器
	ret = connect(sockfd, (struct sockaddr*)&SeviceAddr, sizeof(SeviceAddr));
	if(ret == -1){
		perror("connect is error\n");
		return -1;
	}else{
		while(1){
			//进入send/recv死循环
			//进入交互界面
			printf("************************\n请选择您要执行的操作:\n1.注册\t2.登录\t3.退出\n");
			scanf("%d", &(msg.item));
			switch(msg.item){
				case 1:
					register_guest(sockfd, &msg);
					break;
				case 2:
					login_guest(sockfd, &msg);
					break;
				case 3:
					printf("再见\n");
					usleep(5000);
					exit(0);
					break;
				default:
					printf("请输入1到3之间的数字\n");
			}
		}
	
	}
	//进入send/recv死循环
	//1.进入交互界面
	//2.输入用户名和密码
	//3.将密码和用户名提交给服务器
	//4.阻塞等待服务器返回数据
	//5.如果失败返回错误重新输入密码阶段
	//6.如果成功进入功能选择阶段switch case
	return 0;
}

void register_guest(int sockfd, guest_msg* msg_t){

	char buffer[N] = {};
	int ret;

	printf("请输入您的用户名>>");
	scanf("%s",msg_t->name);
	printf("请输入密码>>");
	scanf("%s",msg_t->pwd);
	if(strcmp(msg_t->name,"root")){
		printf("你不是管理员 不能注册\n");
		return ;
	}
	msg_t->admin = 1;
	ret = send(sockfd, msg_t, sizeof(guest_msg), 0);
	DETECT_ERR("send");
	recv(sockfd, msg_t, sizeof(guest_msg), 0);
	DETECT_ERR("recv");
	printf("---------注册成功---------\n");

	login_guest(sockfd, msg_t);

}
void login_guest(int sockfd, guest_msg* msg_t){

	char buffer[N] = {};
	int ret;

	printf("请输入您的用户名>>");
	scanf("%s",msg_t->name);
	printf("请输入密码>>");
	scanf("%s",msg_t->pwd);
	
	ret = send(sockfd, msg_t, sizeof(guest_msg), 0);
	DETECT_ERR("send");
	recv(sockfd, msg_t, sizeof(guest_msg), 0);
	DETECT_ERR("recv");
	printf("您要查询的功能是什么？\n");
	while(1){
		printf("*********************************************\n请按照提示输入数字进行操作\n1.查询信息\t2.修改信息\t3.删除信息\t4.查询历史\t5.退出");
		scanf("%d",&(msg_t->item));
		msg_t->item += 3;
		switch(msg_t->item){
			case 4:
				send_query(sockfd, msg_t);
				break;
			case 5:
				send_update(sockfd, msg_t);
				break;
			case 6:
				send_delete(sockfd, msg_t);
				break;
			case 7:
				send_history(sockfd, msg_t);
				break;
			case 8:
				close(sockfd);
				exit(0);
			default:
				printf("请输入以下提示的数字\n");
		}
	}
}
void send_query(int sockfd, guest_msg* msg_t){
	
	int ret;
	printf("请输入员工编号>>");
	scanf("%d",&msg_t->id);

	ret = send(sockfd, msg_t, sizeof(guest_msg), 0);
	DETECT_ERR("send_query");
	recv(sockfd, msg_t, sizeof(guest_msg), 0);

	if(strcmp(msg_t->text,"error")){
		printf("没有此编号重新输入\n");
		send_query(sockfd, msg_t);
	}
	show_msg(msg_t);
}
void send_update(int sockfd, guest_msg* msg_t){

	int ret;
	int i;  //for循环使用
	if(msg_t->admin == 0){
		printf("你不是管理员\n");
	}

	printf("请输入员工编号>>");
	scanf("%d",&msg_t->id);
	printf("*************************\n请输入要更改的信息的编号：\n1.姓名\t2.工资\n");
	ret = scanf("%[1,2]",msg_t->text);     //正则表达式 只能输入1 2
	if(ret == EOF){                        //如果不正确递归调用send_update
		printf("输入有误重新输入\n");
		send_update(sockfd, msg_t);
	}else{
		for(i=0; i<strlen(msg_t->text); i++){

			switch(msg_t->text[i]){
				case '1':
					printf("请输入姓名>>");
					scanf("%s",msg_t->name);
				case '2':
					printf("请输入工资>>");
					scanf("%d",&msg_t->balance);
			}
		}
	ret = send(sockfd, msg_t, sizeof(guest_msg), 0);
	DETECT_ERR("send_update");
	recv(sockfd, msg_t, sizeof(guest_msg), 0);
	
	if(strcmp(msg_t->text,"error")){
		printf("没有此编号重新输入\n");
		send_update(sockfd, msg_t);
		}
	show_msg(msg_t);
	}
}
void send_delete(int sockfd, guest_msg* msg_t){
	int ret;
	if(msg_t->admin == 0){
		printf("你不是管理员\n");
	}
	printf("请输入员工编号>>");
	scanf("%d",&msg_t->id);

	ret = send(sockfd, msg_t, sizeof(guest_msg), 0);
	DETECT_ERR("send_delete");
	recv(sockfd, msg_t, sizeof(guest_msg), 0);

	if(strcmp(msg_t->text,"error")){
		printf("没有此编号重新输入\n");
		send_delete(sockfd, msg_t);
	}
}
#if 1
void send_history(int sockfd, guest_msg* msg_t){    //这个没写完
	int ret;
	history_info his = {};

	if(msg_t->admin == 0){
		printf("你不是管理员\n");
	}

	ret = send(sockfd, msg_t, sizeof(guest_msg), 0);
	DETECT_ERR("send_delete");
	recv(sockfd, &his, sizeof(history_info), 0);
	printf("名字>>%s\n登录时间%s\n执行的操作%s\n",his.name, his.time, his.text);
}
#endif
void show_msg(guest_msg *msg_t){                     //显示结束信息
	printf("编号是：%d\n姓名是：%s\n工资是：%d\n",msg_t->id, msg_t->name, msg_t->balance);

}
