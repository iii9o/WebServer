#include <stdio.h>
#include <string.h>
#include <sys\types.h>
#include <sys\stat.h>
//����ͨ�����ͷ�ļ�����Ҫ���صĿ��ļ�
#include <WinSock2.h>
#pragma comment(lib,"WS2_32.lib")
#define PRINTF(str)  printf("[%s-%d]"#str"=%s\n", __func__, __LINE__, str)
#pragma warning(push)
#pragma warning(disable: 4996)
void error_die(const char* str) {
	perror(str);
	exit(1);
}


//ʵ�������ʼ����
//����ֵ���������˵��׽��֣�socket��
//������port ��ʾ�˿�
//���*port��ֵ��0����ô�Զ�������ö˿ڣ���portΪ0����̬�����ʹ�ö˿�
int startup(unsigned short * port) {
	//1.����ͨ�ų�ʼ��
	WSADATA data;
	int ret = WSAStartup(
		MAKEWORD(1, 1),//1,1�汾��Э��
		&data);
	if (ret) {
		error_die("WSAStartup");
	}
	//�����׽���
	int server_socket = socket(PF_INET,//�׽��ֵ�����
		SOCK_STREAM,//������
		IPPROTO_TCP//����ͨѶЭ��
		);
	if (server_socket == -1) {
		//��ӡ������ʾ������
		error_die("socket");
	}
	//�����׽������ԣ��˿ڿɸ���
	int opt = 1;
	ret = setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,(const char*) & opt, sizeof(opt));
	if (ret == -1) {
		error_die("setsockopt");
	}
	//���÷������˵������ַ
	struct sockaddr_in server_addr;

	memset(&server_addr,0,sizeof(server_addr));

	server_addr.sin_family = AF_INET;//AF_INET==PF_INET

	server_addr.sin_port = htons(*port); //host to net short
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //host to net long 



	//���׽���
	if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		error_die("bind");
	}
	//��̬����˿�
	int nameLen = sizeof(server_addr);
	if (*port == 0) {
		if (getsockname(server_socket, (struct sockaddr*)&server_addr, &nameLen) < 0) {
			error_die("getsockname");
		}
		*port = server_addr.sin_port;
	}

	//������������
	if (listen(server_socket, 5) < 0) {
		error_die("listen");
	}


	return server_socket;
}
//��ָ���Ŀͻ����׽���socket�ж�ȡһ�����ݱ�����buff��
//����ʵ�ʶ�ȡ�����ֽ���
int get_line(int socket,char* buff,int size) {
	char c = 0;
	int i = 0;
	//��ʵ����������ʱ�ԡ�\r\n����β
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
	//��ָ����socket,���ͻ�û��ʵ�ֵĽ���
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
	//������Ӧ����ͷ��Ϣ
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
	printf("һ������[%d]�ֽڸ�������\n",count);
}
void server_file(int client, const char* filename) {
	int numchars = 1;
	char type[1024];
	char buff[1024];
	// ��ʣ�������ͷ����
	while (numchars > 0 && strcmp(buff, "\n")) {
		numchars = get_line(client, buff, sizeof(buff));
		PRINTF(buff);
	}

	FILE* resource = NULL;
	if (strcmp(filename, "htdocs/index.html") == 0) {
		resource = fopen(filename, "r");
		strcpy(type, "text/html"); // ���� HTML ����
	}
	else if (strcmp(filename + strlen(filename) - 4, ".css") == 0) {
		resource = fopen(filename, "rb");
		strcpy(type, "text/css"); // ���� CSS ����
	}
	else if (strcmp(filename + strlen(filename) - 3, ".js") == 0) {
		resource = fopen(filename, "rb");
		strcpy(type, "application/javascript"); // ���� JavaScript ����
	}
	else if (strcmp(filename + strlen(filename) - 4, ".png") == 0) {
		resource = fopen(filename, "rb");
		strcpy(type, "image/png"); // ���� PNG ͼ������
	}
	else if (strcmp(filename + strlen(filename) - 4, ".jpg") == 0 || strcmp(filename + strlen(filename) - 5, ".jpeg") == 0) {
		resource = fopen(filename, "rb");
		strcpy(type, "image/jpeg"); // ���� JPEG ͼ������
	}
	else  if (strcmp(filename + strlen(filename) - 4, ".ico") == 0) {
		resource = fopen(filename, "r");
		strcpy(type, "text/html"); // ���� PNG ͼ������
	}
	else {
		not_found(client); // ����ļ�����δ֪������ 404
		return;
	}
	if (resource == NULL) {
		not_found(client);
	}
	else {
		//��ʽ������Դ�������
		headers(client,type);
		//�����������Դ��Ϣ
		cat(client, resource);

		printf("�ļ���Դ�������");
	}
	if (resource == NULL) {
		not_found(client);
	}
	else {
		// ������ȷ�� resource ����Ч��
		fclose(resource);  // ֻ���� resource ��Ϊ NULL ʱ�ŵ��� fclose
	}

}
//�����û�������̺߳���
DWORD WINAPI accept_request(LPVOID arg) {
	//��ȡһ������
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
	//���������Ƿ�֧�ָ÷���
	if (_stricmp(method, "GET") && _stricmp(method, "POST")) {
		//���ش��������
		//todo
		unimplement(client);
		return 0;
	}
	//������Դ·��
	char url[255];  //�����Դ����·��
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
			while (numchars > 0 && strcmp(buff,"\n")) { //��bվ��ͬ
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
	printf("httpd�����Ѿ����������ڼ��� %d �˿�...\n", port);
	struct sockaddr_in client_addr;
	int client_addr_Len = sizeof(client_addr);
	while (1) {
		//����ʽ�ȴ��û�ͨ����������з���
		//clientsocket(connection socket),���ض��ͻ��˽���ͨ��
		int client_socket = accept(server_socket,  //serversocket ��listen Socket��ֻ�������,��ʵ�ʴ���ͻ����������֮���ͨѶ
			(struct sockaddr*)&client_addr,
			&client_addr_Len);
		if (client_socket == -1) {
			error_die("accept");
		}
		//ֱ��ʹ��client_server���û����з��ʻᵼ��ӵ��
		// ��������������µ��߳�,��client���е�������
		// ���̿ɰ�������̣߳��߳�֮�乲���ڴ滥������
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