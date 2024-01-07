//// 2150248-Ҧ����-�Զ��� 2151276-����̩-�Զ���
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
#define RECEIVEONCE DATANUM     //���ν���������
#define IPconfig "127.0.0.1"   //this ip is for local test , modify it by utilizing ipconfig in your cmd and search for ipv4
#define Port 8001
using namespace std;

//����������
float rawFloatData[DATANUM];
float sortmid[DATANUM];
float summid[MAX_THREADS];
float maxmid[MAX_THREADS];
HANDLE hThreads[MAX_THREADS];
HANDLE hSemaphores[MAX_THREADS];
HANDLE g_hHandle;

float summ = 0.0f;
float maxx = 0.0f;


/***************************************���̣߳�32������*************************************************/

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

/***************************************���̣߳�64�����*************************************************/

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
/***************************************���̣߳�64�������ֵ*************************************************/
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

/*��������������������Ļ�����Ԫ���������������������򣬱ȽϷ�������Ϊ�ǶԵ�һ������������ǵ�������
�ռ任ʱ�䡣��ô���������������д洢��ʵЧ�ʻ������*/

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

/***************************************�����򣨵��߳̽϶�ʱ����ݹ�ִ�У�*************************************************/

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
	//���ݳ�ʼ��
	for (size_t i = 0; i < DATANUM; i++)
	{
		rawFloatData[i] = float(i + 1 + DATANUM);

	}
	srand(1234567);//α��������ӣ�ע�ⲻ����ʱ�������ӣ����������޷�ͬ��
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
	cout << "��ʼ�����" << endl;

	//TCP����

	// �����׽��ֿ�
	WSAData wsaData;
	WORD DllVersion = MAKEWORD(2, 1);
	if (WSAStartup(DllVersion, &wsaData) != 0)
	{
		MessageBoxA(NULL, "WinSock startup error", "Error", MB_OK | MB_ICONERROR);
		return 0;
	}

	// ���÷�������ַ�Ͷ˿�
	SOCKADDR_IN addr;
	int addrlen = sizeof(addr);
	addr.sin_family = AF_INET; // IPv4 Socket
	addr.sin_port = htons(Port); // �������˿�
	addr.sin_addr.s_addr = inet_addr(IPconfig); // ������IP

	// ���������׽��֣�
	// ����һ���׽��ֶ��� sListen��ʹ�� TCP Э�飨SOCK_STREAM���� IPv4 ��ַ�壨AF_INET��������������ΪЭ�����������Ϊ NULL ��ʾ����Э�������Զ�ѡ��
	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
	// ���׽��ְ󶨵��ض��� IP ��ַ�Ͷ˿ںš�addr ��һ�� SOCKADDR_IN �ṹ�壬���а���Ҫ�󶨵� IP ��ַ�Ͷ˿ںš�
	// 
	// �� addr ǿ��ת��Ϊ SOCKADDR* ���ͣ���ʹ�� sizeof(addr) ָ����ַ�Ĵ�С��
	
	// ��ʼ������������
	listen(sListen, SOMAXCONN);   //�����������׽����Լ����������������

	//while ()
	SOCKET newConnection; // ����һ���µ��׽������������ӣ�sListen�����ڼ��������������ݽ���
	newConnection = accept(sListen, (SOCKADDR*)&addr, &addrlen); // newConnection������ͻ��˽������ݽ���

	// �޸Ļ�������С
	int optVal = DATANUM * 4 * 8; // ��������С�����ã������DATANUM��һ��Ԥ�����ֵ
	int optLen = sizeof(optVal);  // �������ڻ�ȡһ���������� optLen �Ĵ�С�����������׽���ѡ���ֵ�ͳ��ȡ�
	setsockopt(newConnection, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, optLen); // ���÷��ͻ�������С
	int optval2 = 0;
	getsockopt(newConnection, SOL_SOCKET, SO_SNDBUF, (char*)&optval2, &optLen); // ��ȡ���ͻ�������С
	printf("send buf is %d\n", optval2); // ��ӡ���ͻ�������С

	if (newConnection == 0)
	{
		cout << "bad connection." << endl;
	}
	else
	{
		int begin = 0;//��ʼ��־
		while (1)
		{
			recv(newConnection, (char*)&begin, sizeof(begin), NULL);//�ȴ�����ʼָ��
			if (begin == 1000)
			{
				printf("test begin\n");
				LARGE_INTEGER start, end;
				QueryPerformanceCounter(&start);

				g_hHandle = CreateMutex(NULL, FALSE, NULL);

				for (size_t i = 0; i < MAX_THREADS; i++)
				{
					//hSemaphores[i] = CreateSemaphore(NULL, 0, 1, NULL);//CreateEvent(NULL,TRUE,FALSE)�ȼ�
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
						NULL,// ��ȫ���Ա��
						0,// Ĭ��ջ�ռ�
						Threadsort,//�ص�����
						&rawFloatData[i * SUBDATANUM],// �ص��������
						0,
						NULL);
				}
				WaitForMultipleObjects(MAX_THREADS, hThreads, true, INFINITE); //�������̷߳���ʱ���ܽ���

				sortmerge(MAX_THREADS / 2);
				// �����ݽ�������ͺϲ�

				int sended;
				sended = send(newConnection, (char*)&rawFloatData, sizeof(rawFloatData), NULL);
				// ���� rawFloatData ��������ݸ�������
				// ʹ�� send �����������ݣ��� rawFloatData ǿ��ת��Ϊ char* ���͵�ָ�룬��ָ�����͵����ݴ�СΪ sizeof(rawFloatData)
				// ���ĸ�����Ϊ���ͱ�־������Ϊ NULL��0����ʾʹ��Ĭ�ϱ�־

				sended = send(newConnection, (char*)&maxx, sizeof(float), NULL);
				// ���� maxx ������ֵ��������
				// �� maxx ǿ��ת��Ϊ char* ���͵�ָ�룬��ָ�����͵����ݴ�СΪ sizeof(float)

				sended = send(newConnection, (char*)&summ, sizeof(float), NULL);
				// ���� summ ������ֵ��������
				// �� summ ǿ��ת��Ϊ char* ���͵�ָ�룬��ָ�����͵����ݴ�СΪ sizeof(float)

				
				printf("sended:%d\n", sended);
				QueryPerformanceCounter(&end);
				cout << "��������ʱ��Ϊ��" << (end.QuadPart - start.QuadPart) << endl;
				break;
			}
		}
		closesocket(newConnection);
		WSACleanup();
	}
	return 0;
}