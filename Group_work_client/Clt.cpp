//TCP client

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
#define IPconfig "127.0.0.1"   //修改IP地址，与Server保持一致
#define Port 8001
using namespace std;

//待测试数据
float rawFloatData[DATANUM];

float receiveSorts[DATANUM + RECEIVEONCE];
float tmpSorts[2 * DATANUM];
float sortmid[DATANUM];
float summid[MAX_THREADS];
float maxmid[MAX_THREADS];

HANDLE hThreads[MAX_THREADS];
HANDLE hSemaphores[MAX_THREADS];
HANDLE g_hHandle;

float summ = 0.0f;
float maxx = 0.0f;

/***************************************多线程（64）排序*************************************************/

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

/***************************************多线程（32）求和*************************************************/

DWORD WINAPI Threadsum(LPVOID lpParameter) {
	WaitForSingleObject(g_hHandle, INFINITE);  // 等待互斥量

	float* p = (float*)lpParameter;
	long long int j = (p - &rawFloatData[0]) / SUBDATANUM;  // 计算索引 j

	const size_t simdSize = 8;  // 使用 AVX2，一次处理 8 个单精度浮点数数据
	__m256 sumVector = _mm256_setzero_ps();  // 初始化累加向量为零向量

	for (size_t i = 0; i < SUBDATANUM; i += simdSize) {
		__m256 dataVector = _mm256_loadu_ps(&p[i]);  // 加载数据向量

		// 使用 AVX2 指令进行平方根计算
		dataVector = _mm256_log_ps(_mm256_sqrt_ps(dataVector));

		// 使用 AVX2 指令进行累加
		sumVector = _mm256_add_ps(sumVector, dataVector);
	}

	// 使用 AVX2 指令进行水平求和
	__m128 sum128 = _mm_add_ps(
		_mm256_extractf128_ps(sumVector, 0),
		_mm256_extractf128_ps(sumVector, 1)
	);

	// 将结果存储到 summid[j]
	_mm_store_ss(&summid[j], sum128);

	ReleaseMutex(g_hHandle);  // 释放互斥量
	return 0;
}

/***************************************多线程（64）求最大值*************************************************/

DWORD WINAPI Threadmax(LPVOID lpParameter) {
	float* p = (float*)lpParameter;
	int j = (p - rawFloatData) / SUBDATANUM;  // 计算索引 j
	WaitForSingleObject(g_hHandle, INFINITE);  // 等待互斥量

	const size_t simdSize = 8;  // 使用 AVX2，一次处理 8 个单精度浮点数数据
	__m256 maxValues = _mm256_set1_ps(-FLT_MAX);  // AVX 寄存器用于存储最大值

	for (size_t i = 0; i < SUBDATANUM; i += simdSize) {
		__m256 data = _mm256_loadu_ps(&p[i]);  // 加载数据向量
		maxValues = _mm256_max_ps(maxValues, data);  // 计算最大值
	}

	// 从 maxValues 中获取最大值
	float maxArray[8];
	_mm256_storeu_ps(maxArray, maxValues);

	for (int i = 0; i < simdSize; ++i) {
		if (maxmid[j] < maxArray[i]) {
			maxmid[j] = maxArray[i];
		}
	}

	ReleaseMutex(g_hHandle);  // 释放互斥量
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

void joinsort(float a[], float b[], float c[], int lenth)
{
	int i = 0;
	int j = 0;
	while (i < lenth && j < lenth)
	{
		if (a[i] < b[j])
		{
			c[i + j] = a[i];
			i++;
			continue;
		}
		c[i + j] = b[j];
		j++;
	}
	while (i < lenth)
	{
		c[i + j] = a[i];
		i++;
	}
	while (j < lenth)
	{
		c[i + j] = b[j];
		j++;
	}
}

int main()
{
	//数据初始化
	for (size_t i = 0; i < DATANUM; i++)
	{
		rawFloatData[i] = float(i + 1);
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
	WSAData wsaData;
	WORD DllVersion = MAKEWORD(2, 1);

	if (WSAStartup(DllVersion, &wsaData) != 0)
	{
		MessageBoxA(NULL, "Winsock startup error", "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}

	SOCKADDR_IN addr; //Adres przypisany do socketu Connection
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr(IPconfig); //srv ip
	addr.sin_port = htons(Port); //Port = 1111
	addr.sin_family = AF_INET; //IPv4 Socket

	/*
	这段代码用于创建一个新的套接字并与指定的地址进行连接。
	*/
	SOCKET newConnection = socket(AF_INET, SOCK_STREAM, NULL); // 创建一个新的套接字 newConnection，使用 AF_INET 域和 SOCK_STREAM 类型
	if (connect(newConnection, (SOCKADDR*)&addr, sizeof(addr)) != 0) // 连接到指定的地址（addr）
	{
		MessageBoxA(NULL, "Blad Connection", "Error", MB_OK | MB_ICONERROR); // 显示连接错误消息框
		return 0;
	}

	//修改缓冲区大小
	int optVal = DATANUM * 4 * 8;  // 设置选项值为 DATANUM * 4 * 8
	int optLen = sizeof(optVal);  // 获取选项值的长度
	setsockopt(newConnection, SOL_SOCKET, SO_RCVBUF, (char*)&optVal, optLen);  // 设置接收缓冲区大小为 optVal

	int optval2 = 0;
	getsockopt(newConnection, SOL_SOCKET, SO_RCVBUF, (char*)&optval2, &optLen);  // 获取接收缓冲区的实际大小
	printf("recv buf is %d\n", optval2);  // 打印接收缓冲区的大小

	int receivesuccess = 0;  // 接收成功的标志
	int received = 0;  // 已接收的数据量
	int i = 0;
	char* receivePin = (char*)receiveSorts;  // 接收数据的指针
	int timesum, timemax, timesort;

	if (newConnection == 0)
	{
		std::cout << "bad connection." << std::endl;
	}
	else {

		int begin = 1000;
		send(newConnection, (char*)&begin, sizeof(begin), NULL);
		printf("start signal sended\n");

		LARGE_INTEGER start, end;

		QueryPerformanceCounter(&start);//start 
		g_hHandle = CreateMutex(NULL, FALSE, NULL);

		for (int i = 0; i < MAX_THREADS; i++) {
			hThreads[i] = CreateThread(
				NULL,  // 安全属性标记
				0,  // 默认栈空间
				Threadsort,  // 回调函数
				&rawFloatData[i * SUBDATANUM],  // 回调函数句柄
				0,
				NULL);
		}

		WaitForMultipleObjects(MAX_THREADS, hThreads, true, INFINITE);	//当所有线程返回时才能结束
		sortmerge(MAX_THREADS / 2);
		//end = GetTickCount64();//200ms
		//接收数据

		while (1) {

			receivesuccess = recv(newConnection, &receivePin[received], RECEIVEONCE * sizeof(float), NULL);
			//printf("第%d次接收：接收数据量：%d\n", i, receivesuccess);

			if (receivesuccess == -1) {  // 如果接收失败，打印错误信息
				int erron = WSAGetLastError();
			}
			else {
				received = received + receivesuccess;
			}
			if (received >= (4 * DATANUM)) {  // 如果已接收的数据量达到预期大小，跳出循环
				break;
			}
		}

		//再归并
		joinsort(receiveSorts, rawFloatData, tmpSorts, DATANUM);

		QueryPerformanceCounter(&end);//end
		timesort = (end.QuadPart - start.QuadPart);
		QueryPerformanceCounter(&start);//start  

		for (size_t i = 0; i < MAX_THREADS; i++)
		{
			hThreads[i] = CreateThread(
				NULL,
				0,
				Threadsum,   //回调函数
				&rawFloatData[i * SUBDATANUM],
				0,
				NULL);
		}
		WaitForMultipleObjects(MAX_THREADS, hThreads, TRUE, INFINITE);  //当所有线程返回时才能结束

		for (size_t i = 0; i < MAX_THREADS; i++)
		{
			summ += summid[i];
		}

		receivesuccess = recv(newConnection, &receivePin[received], RECEIVEONCE * sizeof(float), NULL);

		if (receivesuccess == -1)//打印错误信息
		{
			int erron = WSAGetLastError();
			//printf("erron=%d\n", erron);
		}

		else
		{
			received = received + receivesuccess;
		}

		summ += receiveSorts[DATANUM + 1];

		QueryPerformanceCounter(&end);//end
		timesum = (end.QuadPart - start.QuadPart);


		QueryPerformanceCounter(&start);//start 

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

		receivesuccess = recv(newConnection, &receivePin[received], RECEIVEONCE * sizeof(float), NULL);

		if (receivesuccess == -1)//打印错误信息
		{
			int erron = WSAGetLastError();
			//printf("erron=%d\n", erron);
		}

		else
		{
			received = received + receivesuccess;
		}
		if (receiveSorts[DATANUM + 0] > maxx)
			maxx = receiveSorts[DATANUM + 0];
		QueryPerformanceCounter(&end);//end
		timemax = (end.QuadPart - start.QuadPart);
		cout << "输出求和结果：" << summ << endl;
		cout << "双机求和总时间为：" << timesum << endl;
		cout << "输出求最大值结果：" << maxx << endl;
		cout << "双机求最大值总时间为：" << timemax << endl;

		//检查排序是否正确
		bool comp = true;

		for (int i = 0; i < DATANUM; i++) {
			if (rawFloatData[i] > rawFloatData[i + 1]) {
				comp = false;
			}
		}

		//检查排序
		if (comp)
		{
			cout << "输出排序正确" << endl;
		}

		else
		{
			cout << "输出排序错误" << endl;
		}
		cout << "双机排序总时间为：" << timesort << endl;
		cout << "接收并处理数据总时间为：" << timesort + timemax + timesum << endl;
	}

	closesocket(newConnection);
	WSACleanup();
	return 0;
}

