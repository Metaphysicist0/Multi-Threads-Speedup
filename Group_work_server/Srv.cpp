//// 2150248-姚天亮-自动化 2151276-吕泓泰-自动化
//server
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include "stdafx.h"
#include <Windows.h>
#include <math.h>
#include<cmath>
#include<thread>
#include <emmintrin.h>  //open look,look 128bit
#include <immintrin.h>  //open look,look 256bit
#include <zmmintrin.h>  //open look,look 512bit

#define MAX_THREADS 64
#define SUBDATANUM 1000000
#define DATANUM (SUBDATANUM*MAX_THREADS)
#define RECEIVEONCE DATANUM     //单次接收数据量
#define IPconfig "127.0.0.1"   //this ip is for local test , modify it by utilizing ipconfig in your cmd and search for ipv4
#define Port 8001
using namespace std;

//待测试数据
float rawFloatData[DATANUM];
float sortmid[DATANUM];
float summid[MAX_THREADS];
float maxmid[MAX_THREADS];
HANDLE hThreads[MAX_THREADS];
HANDLE hSemaphores[MAX_THREADS];
HANDLE g_hHandle;

float summ = 0.0f;
float maxx = 0.0f;


/***************************************多线程（32）排序*************************************************/

void SWAP(float* x, float* y) {
	float temp = *x;
	*x = *y;
	*y = temp;
}

void quicksort(float arr[], int left, int right) {
	int i, j;
	float s;

	if (left < right) {
		s = arr[(left + right) / 2];
		i = left - 1;
		j = right + 1;

		while (1) {
			do {
				i++;
			} while (arr[i] < s);

			do {
				j--;
			} while (arr[j] > s);

			if (i >= j)
				break;

			SWAP(&arr[i], &arr[j]);
		}

		quicksort(arr, left, i - 1);
		quicksort(arr, j + 1, right);
	}
}

DWORD WINAPI Threadsort(LPVOID lpParameter)
{
	float* p = (float*)lpParameter;
	//avx256_quicksort(p, 0, SUBDATANUM - 1);
	quicksort(p, 0, SUBDATANUM - 1);
	return 0;
}

/***************************************多线程（64）求和*************************************************/

DWORD WINAPI Threadsum(LPVOID lpParameter) {
	WaitForSingleObject(g_hHandle, INFINITE);

	float* p = (float*)lpParameter;
	long long int j = (p - &rawFloatData[0]) / SUBDATANUM;

	const size_t simdSize = 8;  // Use AVX2, processing 8 single-precision (float) data at a time
	__m256 sumVector = _mm256_setzero_ps();

	for (size_t i = 0; i < SUBDATANUM; i += simdSize) {
		__m256 dataVector = _mm256_loadu_ps(&p[i]);

		// Use AVX2 instructions for square root calculation
		dataVector = _mm256_log_ps(_mm256_sqrt_ps(dataVector));

		// Use AVX2 instructions for addition
		sumVector = _mm256_add_ps(sumVector, dataVector);
	}

	// Use AVX2 instructions for horizontal sum
	__m128 sum128 = _mm_add_ps(
		_mm256_extractf128_ps(sumVector, 0),
		_mm256_extractf128_ps(sumVector, 1)
	);

	// Store the result in summid[j]
	_mm_store_ss(&summid[j], sum128);

	ReleaseMutex(g_hHandle);
	return 0;
}
/***************************************多线程（64）求最大值*************************************************/
DWORD WINAPI Threadmax(LPVOID lpParameter) {
	float* p = (float*)lpParameter;
	int j = (p - rawFloatData) / SUBDATANUM;
	WaitForSingleObject(g_hHandle, INFINITE);

	const size_t simdSize = 8;  // Use AVX2, processing 8 single-precision (float) data at a time
	__m256 maxValues = _mm256_set1_ps(-FLT_MAX); // AVX register to store maximum values

	for (size_t i = 0; i < SUBDATANUM; i += simdSize) {
		__m256 data = _mm256_loadu_ps(&p[i]);
		maxValues = _mm256_max_ps(maxValues, data);
	}

	// Get the maximum values from maxValues
	float maxArray[8];
	_mm256_storeu_ps(maxArray, maxValues);

	for (int i = 0; i < simdSize; ++i) {
		if (maxmid[j] < maxArray[i]) {
			maxmid[j] = maxArray[i];
		}
	}

	ReleaseMutex(g_hHandle);
	return 0;
}

/*插入排序函数，是重排序的基本单元，对两个有序数组重排序，比较繁琐，因为是对单一数组操作，考虑到可以用
空间换时间。那么将排序后的数组另行存储其实效率会大大提高*/

void sortinsert(float* p1, float* p2)
{
	long long int i = p2 - p1;
	float* p1_add = p1;
	float* p2_add = p2;
	for (int j = 0; j < i; j++)
	{
		if (*p1_add <= *p2_add)
		{
			p1_add++;
		}
		else
		{
			float tmp = *p2_add;
			for (size_t k = (p2_add - p1_add); k > 0; k--)
			{
				p1_add[k] = p1_add[k - 1];
			}
			*p1_add = tmp;
			if (p2_add - p2 + 1 == p2 - p1)
			{
				break;
			}
			p2_add++;
			p1_add++;
			j--;
		}
	}
}

/***************************************重排序（当线程较多时，会递归执行）*************************************************/

void sortmerge(int Threads)
{
	if (Threads > 1)
	{
		sortmerge(Threads / 2);
	}
	for (size_t i = 0; i < MAX_THREADS / Threads; i += 2)
	{
		sortinsert(&rawFloatData[i * Threads * SUBDATANUM], &rawFloatData[(i + 1) * Threads * SUBDATANUM]);
	}

}

int main()
{
	//数据初始化
	for (size_t i = 0; i < DATANUM; i++)
	{
		rawFloatData[i] = float(i + 1 + DATANUM);

	}
	srand(1234567);//伪随机数种子，注意不能用时间做种子，否则两机无法同步
	float  tmp;
	size_t  T = DATANUM;
	while (T--)
	{
		size_t i, j;
		i = rand() % DATANUM;
		j = rand() % DATANUM;
		tmp = rawFloatData[i];
		rawFloatData[i] = rawFloatData[j];
		rawFloatData[j] = tmp;
	}
	cout << "初始化完成" << endl;

	//TCP连接

	// 导入套接字库
	WSAData wsaData;
	WORD DllVersion = MAKEWORD(2, 1);
	if (WSAStartup(DllVersion, &wsaData) != 0)
	{
		MessageBoxA(NULL, "WinSock startup error", "Error", MB_OK | MB_ICONERROR);
		return 0;
	}

	// 设置服务器地址和端口
	SOCKADDR_IN addr;
	int addrlen = sizeof(addr);
	addr.sin_family = AF_INET; // IPv4 Socket
	addr.sin_port = htons(Port); // 服务器端口
	addr.sin_addr.s_addr = inet_addr(IPconfig); // 服务器IP

	// 创建监听套接字，
	// 创建一个套接字对象 sListen，使用 TCP 协议（SOCK_STREAM）和 IPv4 地址族（AF_INET）。第三个参数为协议参数，设置为 NULL 表示根据协议类型自动选择。
	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
	// 将套接字绑定到特定的 IP 地址和端口号。addr 是一个 SOCKADDR_IN 结构体，其中包含要绑定的 IP 地址和端口号。
	// 
	// 将 addr 强制转换为 SOCKADDR* 类型，并使用 sizeof(addr) 指定地址的大小。
	
	// 开始监听连接请求
	listen(sListen, SOMAXCONN);   //启动服务器套接字以监听传入的连接请求。

	//while ()
	SOCKET newConnection; // 建立一个新的套接字用于新连接，sListen仅用于监听而不用于数据交换
	newConnection = accept(sListen, (SOCKADDR*)&addr, &addrlen); // newConnection用于与客户端进行数据交换

	// 修改缓冲区大小
	int optVal = DATANUM * 4 * 8; // 缓冲区大小的设置，这里的DATANUM是一个预定义的值
	int optLen = sizeof(optVal);  // 代码用于获取一个整数变量 optLen 的大小，用于设置套接字选项的值和长度。
	setsockopt(newConnection, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, optLen); // 设置发送缓冲区大小
	int optval2 = 0;
	getsockopt(newConnection, SOL_SOCKET, SO_SNDBUF, (char*)&optval2, &optLen); // 获取发送缓冲区大小
	printf("send buf is %d\n", optval2); // 打印发送缓冲区大小

	if (newConnection == 0)
	{
		cout << "bad connection." << endl;
	}
	else
	{
		int begin = 0;//开始标志
		while (1)
		{
			recv(newConnection, (char*)&begin, sizeof(begin), NULL);//等待任务开始指令
			if (begin == 1000)
			{
				printf("test begin\n");
				LARGE_INTEGER start, end;
				QueryPerformanceCounter(&start);

				g_hHandle = CreateMutex(NULL, FALSE, NULL);

				for (size_t i = 0; i < MAX_THREADS; i++)
				{
					//hSemaphores[i] = CreateSemaphore(NULL, 0, 1, NULL);//CreateEvent(NULL,TRUE,FALSE)等价
					hThreads[i] = CreateThread(
						NULL,
						0,
						Threadsum,
						&rawFloatData[i * SUBDATANUM],
						0,
						NULL);
				}

				WaitForMultipleObjects(MAX_THREADS, hThreads, TRUE, INFINITE);
				for (size_t i = 0; i < MAX_THREADS; i++)
				{
					summ += summid[i];
				}
				for (size_t i = 0; i < MAX_THREADS; i++)
				{
					hThreads[i] = CreateThread(
						NULL,
						0,
						Threadmax,
						&rawFloatData[i * SUBDATANUM],
						0,
						NULL);
				}
				WaitForMultipleObjects(MAX_THREADS, hThreads, true, INFINITE);
				for (size_t i = 0; i < MAX_THREADS; i++)
				{
					if (maxx < maxmid[i])
					{
						maxx = maxmid[i];
					}
				}
				for (int i = 0; i < MAX_THREADS; i++)
				{

					hThreads[i] = CreateThread(
						NULL,// 安全属性标记
						0,// 默认栈空间
						Threadsort,//回调函数
						&rawFloatData[i * SUBDATANUM],// 回调函数句柄
						0,
						NULL);
				}
				WaitForMultipleObjects(MAX_THREADS, hThreads, true, INFINITE); //当所有线程返回时才能结束

				sortmerge(MAX_THREADS / 2);
				// 对数据进行排序和合并

				int sended;
				sended = send(newConnection, (char*)&rawFloatData, sizeof(rawFloatData), NULL);
				// 发送 rawFloatData 数组的数据给新连接
				// 使用 send 函数发送数据，将 rawFloatData 强制转换为 char* 类型的指针，并指定发送的数据大小为 sizeof(rawFloatData)
				// 第四个参数为发送标志，设置为 NULL（0）表示使用默认标志

				sended = send(newConnection, (char*)&maxx, sizeof(float), NULL);
				// 发送 maxx 变量的值给新连接
				// 将 maxx 强制转换为 char* 类型的指针，并指定发送的数据大小为 sizeof(float)

				sended = send(newConnection, (char*)&summ, sizeof(float), NULL);
				// 发送 summ 变量的值给新连接
				// 将 summ 强制转换为 char* 类型的指针，并指定发送的数据大小为 sizeof(float)

				
				printf("sended:%d\n", sended);
				QueryPerformanceCounter(&end);
				cout << "发送数据时间为：" << (end.QuadPart - start.QuadPart) << endl;
				break;
			}
		}
		closesocket(newConnection);
		WSACleanup();
	}
	return 0;
}