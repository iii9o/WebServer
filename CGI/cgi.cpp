//使用匿名管道实现进程通信
#include <stdio.h>
#include <Windows.h>
int main(void) {
	//创建管道
	HANDLE output[2]; //管道两端句柄
	SECURITY_ATTRIBUTES la;
	la.nLength = sizeof(la);
	la.bInheritHandle = true;
	la.lpSecurityDescriptor = 0;

	BOOL bCreat = CreatePipe(&output[0], &output[1], &la, 0);

	if (bCreat == false)
	{	
		MessageBox(0, L"creat pipe cgi error!", 0, 0);
		return 1;
	}
	char buff[1024];
	DWORD size;
	while (1) {
		printf("请输入");
		gets_s(buff, sizeof(buff));
		WriteFile(output[1], buff, strlen(buff) + 1, &size, NULL);
		printf("已经写入%d字节\n", size);
		ReadFile(output[0], buff, sizeof(buff), &size, NULL);
		printf("已经读取%d字节:[%s]\n", size,buff);

	}

	return 0;
}