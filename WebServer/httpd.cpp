#include <stdio.h>
#include <string.h>
#include <sys\types.h>
#include <sys\stat.h>
//网络通信相关头文件和需要加载的库文件
#include <WinSock2.h>
#pragma comment(lib,"WS2_32.lib")
#define PRINTF(str)  printf("[%s-%d]"#str"=%s\n", __func__, __LINE__, str)
#pragma warning(push)
#pragma warning(disable: 4996)
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
//从指定的客户端套接字socket中读取一行数据保存在buff中
//返回实际读取到的字节数
int get_line(int socket,char* buff,int size) {
	char c = 0;
	int i = 0;
	//真实的网络请求时以“\r\n”结尾
	while (i<size-1 && c!='\n') {
		int n = recv(socket, &c, 1, 0);
		if (n > 0) {
			if (c == '\r') {
				n = recv(socket, &c, 1, MSG_PEEK);
				if (n > 0 && c == '\n') {
					recv(socket, &c, 1, 0);
				}
				else {
					c = '\n';
				}
			}
			buff[i++] = c;
		}
		else {
			//to do
			c = '\n';
		}
	}
	buff[i] = 0;
	return i;
}
void unimplement(int client) {
	//向指定的socket,返送还没有实现的界面
}
void not_found(int client) {
	char buff[1024];
	strcpy(buff, "HTTP/1.0 404 NOT FOUND\r\n");
	send(client, buff, strlen(buff), 0);
	strcpy(buff, "Server:PBHpptd/0.1\r\n");
	send(client, buff, strlen(buff), 0);
	strcpy(buff, "Content-Type: text/html\r\n");
	send(client, buff, strlen(buff), 0); 
	/*char buf[1024];
	sprintf(buf, "Content-Type: %s\r\n", type);
	send(client, buf, strlen(buf), 0);*/

	strcpy(buff, "\r\n");
	send(client, buff, strlen(buff), 0);
	sprintf(buff,
		"<html>\r\n"
		"  <head><title>404 Not Found</title></head>\r\n"
		"  <body>\r\n"
		"    <h1>404 Not Found</h1>\r\n"
		"    <p>The requested URL was not found on this server.</p>\r\n"
		"  </body>\r\n"
		"</html>\r\n");
	send(client, buff, strlen(buff), 0);

}
void headers(int client,const char* type) {
	//发送响应包的头信息
	char buff[1024];
	strcpy(buff, "HTTP/1.0 200 OK\r\n");
	send(client, buff, strlen(buff), 0);
	strcpy(buff, "Server:PBHpptd/0.1\r\n");
	send(client, buff, strlen(buff), 0);

	char buf[1024];
	sprintf(buf, "Content-Type: %s\r\n",type);
	send(client, buf, strlen(buf), 0);

	strcpy(buff, "\r\n");
	send(client, buff, strlen(buff), 0);
}

//12345
void cat(int client, FILE* resource) {
	char buff[4096];
	int count = 0;
	while (1) {
		int ret = fread(buff, sizeof(char), sizeof(buff), resource);
		if (ret <= 0) {
			break;
		}
		send(client, buff, ret, 0);
		count += ret;
	}
	printf("一共发送[%d]字节给服务器\n",count);
}
void server_file(int client, const char* filename) {
	int numchars = 1;
	char type[1024];
	char buff[1024];
	// 把剩余的请求头读完
	while (numchars > 0 && strcmp(buff, "\n")) {
		numchars = get_line(client, buff, sizeof(buff));
		PRINTF(buff);
	}

	FILE* resource = NULL;
	if (strcmp(filename, "htdocs/index.html") == 0) {
		resource = fopen(filename, "r");
		strcpy(type, "text/html"); // 设置 HTML 类型
	}
	else if (strcmp(filename + strlen(filename) - 4, ".css") == 0) {
		resource = fopen(filename, "rb");
		strcpy(type, "text/css"); // 设置 CSS 类型
	}
	else if (strcmp(filename + strlen(filename) - 3, ".js") == 0) {
		resource = fopen(filename, "rb");
		strcpy(type, "application/javascript"); // 设置 JavaScript 类型
	}
	else if (strcmp(filename + strlen(filename) - 4, ".png") == 0) {
		resource = fopen(filename, "rb");
		strcpy(type, "image/png"); // 设置 PNG 图像类型
	}
	else if (strcmp(filename + strlen(filename) - 4, ".jpg") == 0 || strcmp(filename + strlen(filename) - 5, ".jpeg") == 0) {
		resource = fopen(filename, "rb");
		strcpy(type, "image/jpeg"); // 设置 JPEG 图像类型
	}
	else  if (strcmp(filename + strlen(filename) - 4, ".ico") == 0) {
		resource = fopen(filename, "r");
		strcpy(type, "text/html"); // 设置 PNG 图像类型
	}
	else {
		not_found(client); // 如果文件类型未知，返回 404
		return;
	}
	if (resource == NULL) {
		not_found(client);
	}
	else {
		//正式发送资源给浏览器
		headers(client,type);
		//发送请求的资源信息
		cat(client, resource);

		printf("文件资源发送完毕");
	}
	if (resource == NULL) {
		not_found(client);
	}
	else {
		// 在这里确保 resource 是有效的
		fclose(resource);  // 只有在 resource 不为 NULL 时才调用 fclose
	}

}
//处理用户请求的线程函数
DWORD WINAPI accept_request(LPVOID arg) {
	//读取一行数据
	char buff[1024]; //
	int client = (SOCKET)arg;
	int numchars = get_line(client,buff,sizeof(buff));
	PRINTF(buff);
	char method[255];
	int j = 0;
	int i = 0;
	while (isspace(buff[j]) && j < sizeof(buff)) {
		j++;
	}
	while (!isspace(buff[j]) && i<sizeof(method)-1) {
		method[i++] = buff[j++];
	}
	method[i] = 0;
	PRINTF(method);
	//检查服务器是否支持该方法
	if (_stricmp(method, "GET") && _stricmp(method, "POST")) {
		//返回错误揭秘那
		//todo
		unimplement(client);
		return 0;
	}
	//解析资源路径
	char url[255];  //存放资源请求路径
	while (isspace(buff[j]) && j < sizeof(buff)) {
		j++;
	}
	i = 0;
	while (!isspace(buff[j]) && i < sizeof(url)-1 && j<sizeof(buff)) {
		url[i++] = buff[j++];
	}
	url[i] = 0;
	PRINTF(url);
	//127.0.0.1
	//url / 
	// htdocs/index.html
	char path[512] = "";
	sprintf_s(path,"htdocs%s",url);
		if (path[strlen(path)-1]=='/') {
			strcat_s(path, "index.html");
		}
		PRINTF(path);
		struct stat status;
		if (stat(path, &status) == -1) {
			while (numchars > 0 && strcmp(buff,"\n")) { //和b站不同
			numchars = get_line(client, buff, sizeof(buff));
			}
			not_found(client);
		}
		else {
			if ((status.st_mode & S_IFMT) == S_IFDIR) {
				strcat_s(path, "/index");
			}
		}
		server_file(client, path);

		closesocket(client);

	return 0;
}

int main(void) {
	unsigned short  port = 80 ;			
	int server_socket = startup(&port);
	printf("httpd服务已经启动，正在监听 %d 端口...\n", port);
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
	WSACleanup();
	system("pause");
	return 0;
}



#pragma warning(pop)