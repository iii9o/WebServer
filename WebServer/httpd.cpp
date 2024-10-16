#include <stdio.h>
#include <string.h>
//网络通信相关头文件和需要加载的库文件
#include <WinSock2.h>
#pragma comment(lib,"WS2_32.lib")
void error_die(const char* str) {
	perror(str);
	exit(1);
}


//实现网络初始化，
//返回值：服务器端的套接字（socket）
//参数：port 表示端口
//如果*port的值是0，那么自动分配可用端口，若port为0，则动态分配可使用端口
int startup(unsigned short * port) {
	//1.网络通信初始化
	WSADATA data;
	int ret = WSAStartup(
		MAKEWORD(1, 1),//1,1版本的协议
		&data);
	if (ret) {
		error_die("WSAStartup");
	}
	//创建套接字
	int server_socket = socket(PF_INET,//套接字的类型
		SOCK_STREAM,//数据流
		IPPROTO_TCP//具体通讯协议
		);
	if (server_socket == -1) {
		//打印错误提示并结束
		error_die("socket");
	}
	//设置套接字属性，端口可复用
	int opt = 1;
	ret = setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,(const char*) & opt, sizeof(opt));
	if (ret == -1) {
		error_die("setsockopt");
	}
	//配置服务器端的网络地址
	struct sockaddr_in server_addr;

	memset(&server_addr,0,sizeof(server_addr));

	server_addr.sin_family = AF_INET;//AF_INET==PF_INET

	server_addr.sin_port = htons(*port); //host to net short
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //host to net long 



	//绑定套接字
	if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		error_die("bind");
	}
	//动态分配端口
	int nameLen = sizeof(server_addr);
	if (*port == 0) {
		if (getsockname(server_socket, (struct sockaddr*)&server_addr, &nameLen) < 0) {
			error_die("getsockname");
		}
		*port = server_addr.sin_port;
	}

	//创建监听队列
	if (listen(server_socket, 5) < 0) {
		error_die("listen");
	}


	return server_socket;
}

//处理用户请求的线程函数
DWORD WINAPI accept_request(LPVOID arg) {

}

int main(void) {
	unsigned short  port = 0 ;			
	int server_socket = startup(&port);
	printf("httpd服务已经启动，正在监听 %d 端口...", port);
	struct sockaddr_in client_addr;
	int client_addr_Len = sizeof(client_addr);
	while (1) {
		//阻塞式等待用户通过浏览器进行访问
		//clientsocket(connection socket),与特定客户端进行通信
		int client_socket = accept(server_socket,  //serversocket （listen Socket）只负责监听,不实际处理客户端与服务器之间的通讯
			(struct sockaddr*)&client_addr,
			&client_addr_Len);
		if (client_socket == -1) {
			error_die("accept");
		}
		//直接使用client_server对用户进行访问会导致拥塞
		// 解决方法：创建新的线程,对client进行单独服务
		// 进程可包含多个线程，线程之间共享内存互不隔离
		DWORD threadId = 0;
		CreateThread(0,0,accept_request
			,(void*)client_socket
			,0,&threadId);
	}

	closesocket(server_socket); 
	system("pause");
	return 0;
}