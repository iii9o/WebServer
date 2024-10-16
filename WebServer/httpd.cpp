#include <stdio.h>
#include <string.h>
//����ͨ�����ͷ�ļ�����Ҫ���صĿ��ļ�
#include <WinSock2.h>
#pragma comment(lib,"WS2_32.lib")
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

//�����û�������̺߳���
DWORD WINAPI accept_request(LPVOID arg) {

}

int main(void) {
	unsigned short  port = 0 ;			
	int server_socket = startup(&port);
	printf("httpd�����Ѿ����������ڼ��� %d �˿�...", port);
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
	system("pause");
	return 0;
}