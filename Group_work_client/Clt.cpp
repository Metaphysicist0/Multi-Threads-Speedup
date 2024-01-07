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
#define RECEIVEONCE DATANUM     //���ν���������
#define IPconfig "127.0.0.1"   //�޸�IP��ַ����Server����һ��
#define Port 8001
using namespace std;

//����������
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

/***************************************���̣߳�64������*************************************************/

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

/***************************************���̣߳�32�����*************************************************/

DWORD WINAPI Threadsum(LPVOID lpParameter) {
	WaitForSingleObject(g_hHandle, INFINITE);  // �ȴ�������

	float* p = (float*)lpParameter;
	long long int j = (p - &rawFloatData[0]) / SUBDATANUM;  // �������� j

	const size_t simdSize = 8;  // ʹ�� AVX2��һ�δ��� 8 �������ȸ���������
	__m256 sumVector = _mm256_setzero_ps();  // ��ʼ���ۼ�����Ϊ������

	for (size_t i = 0; i < SUBDATANUM; i += simdSize) {
		__m256 dataVector = _mm256_loadu_ps(&p[i]);  // ������������

		// ʹ�� AVX2 ָ�����ƽ��������
		dataVector = _mm256_log_ps(_mm256_sqrt_ps(dataVector));

		// ʹ�� AVX2 ָ������ۼ�
		sumVector = _mm256_add_ps(sumVector, dataVector);
	}

	// ʹ�� AVX2 ָ�����ˮƽ���
	__m128 sum128 = _mm_add_ps(
		_mm256_extractf128_ps(sumVector, 0),
		_mm256_extractf128_ps(sumVector, 1)
	);

	// ������洢�� summid[j]
	_mm_store_ss(&summid[j], sum128);

	ReleaseMutex(g_hHandle);  // �ͷŻ�����
	return 0;
}

/***************************************���̣߳�64�������ֵ*************************************************/

DWORD WINAPI Threadmax(LPVOID lpParameter) {
	float* p = (float*)lpParameter;
	int j = (p - rawFloatData) / SUBDATANUM;  // �������� j
	WaitForSingleObject(g_hHandle, INFINITE);  // �ȴ�������

	const size_t simdSize = 8;  // ʹ�� AVX2��һ�δ��� 8 �������ȸ���������
	__m256 maxValues = _mm256_set1_ps(-FLT_MAX);  // AVX �Ĵ������ڴ洢���ֵ

	for (size_t i = 0; i < SUBDATANUM; i += simdSize) {
		__m256 data = _mm256_loadu_ps(&p[i]);  // ������������
		maxValues = _mm256_max_ps(maxValues, data);  // �������ֵ
	}

	// �� maxValues �л�ȡ���ֵ
	float maxArray[8];
	_mm256_storeu_ps(maxArray, maxValues);

	for (int i = 0; i < simdSize; ++i) {
		if (maxmid[j] < maxArray[i]) {
			maxmid[j] = maxArray[i];
		}
	}

	ReleaseMutex(g_hHandle);  // �ͷŻ�����
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
	//���ݳ�ʼ��
	for (size_t i = 0; i < DATANUM; i++)
	{
		rawFloatData[i] = float(i + 1);
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
	��δ������ڴ���һ���µ��׽��ֲ���ָ���ĵ�ַ�������ӡ�
	*/
	SOCKET newConnection = socket(AF_INET, SOCK_STREAM, NULL); // ����һ���µ��׽��� newConnection��ʹ�� AF_INET ��� SOCK_STREAM ����
	if (connect(newConnection, (SOCKADDR*)&addr, sizeof(addr)) != 0) // ���ӵ�ָ���ĵ�ַ��addr��
	{
		MessageBoxA(NULL, "Blad Connection", "Error", MB_OK | MB_ICONERROR); // ��ʾ���Ӵ�����Ϣ��
		return 0;
	}

	//�޸Ļ�������С
	int optVal = DATANUM * 4 * 8;  // ����ѡ��ֵΪ DATANUM * 4 * 8
	int optLen = sizeof(optVal);  // ��ȡѡ��ֵ�ĳ���
	setsockopt(newConnection, SOL_SOCKET, SO_RCVBUF, (char*)&optVal, optLen);  // ���ý��ջ�������СΪ optVal

	int optval2 = 0;
	getsockopt(newConnection, SOL_SOCKET, SO_RCVBUF, (char*)&optval2, &optLen);  // ��ȡ���ջ�������ʵ�ʴ�С
	printf("recv buf is %d\n", optval2);  // ��ӡ���ջ������Ĵ�С

	int receivesuccess = 0;  // ���ճɹ��ı�־
	int received = 0;  // �ѽ��յ�������
	int i = 0;
	char* receivePin = (char*)receiveSorts;  // �������ݵ�ָ��
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
				NULL,  // ��ȫ���Ա��
				0,  // Ĭ��ջ�ռ�
				Threadsort,  // �ص�����
				&rawFloatData[i * SUBDATANUM],  // �ص��������
				0,
				NULL);
		}

		WaitForMultipleObjects(MAX_THREADS, hThreads, true, INFINITE);	//�������̷߳���ʱ���ܽ���
		sortmerge(MAX_THREADS / 2);
		//end = GetTickCount64();//200ms
		//��������

		while (1) {

			receivesuccess = recv(newConnection, &receivePin[received], RECEIVEONCE * sizeof(float), NULL);
			//printf("��%d�ν��գ�������������%d\n", i, receivesuccess);

			if (receivesuccess == -1) {  // �������ʧ�ܣ���ӡ������Ϣ
				int erron = WSAGetLastError();
			}
			else {
				received = received + receivesuccess;
			}
			if (received >= (4 * DATANUM)) {  // ����ѽ��յ��������ﵽԤ�ڴ�С������ѭ��
				break;
			}
		}

		//�ٹ鲢
		joinsort(receiveSorts, rawFloatData, tmpSorts, DATANUM);

		QueryPerformanceCounter(&end);//end
		timesort = (end.QuadPart - start.QuadPart);
		QueryPerformanceCounter(&start);//start  

		for (size_t i = 0; i < MAX_THREADS; i++)
		{
			hThreads[i] = CreateThread(
				NULL,
				0,
				Threadsum,   //�ص�����
				&rawFloatData[i * SUBDATANUM],
				0,
				NULL);
		}
		WaitForMultipleObjects(MAX_THREADS, hThreads, TRUE, INFINITE);  //�������̷߳���ʱ���ܽ���

		for (size_t i = 0; i < MAX_THREADS; i++)
		{
			summ += summid[i];
		}

		receivesuccess = recv(newConnection, &receivePin[received], RECEIVEONCE * sizeof(float), NULL);

		if (receivesuccess == -1)//��ӡ������Ϣ
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

		if (receivesuccess == -1)//��ӡ������Ϣ
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
		cout << "�����ͽ����" << summ << endl;
		cout << "˫�������ʱ��Ϊ��" << timesum << endl;
		cout << "��������ֵ�����" << maxx << endl;
		cout << "˫�������ֵ��ʱ��Ϊ��" << timemax << endl;

		//��������Ƿ���ȷ
		bool comp = true;

		for (int i = 0; i < DATANUM; i++) {
			if (rawFloatData[i] > rawFloatData[i + 1]) {
				comp = false;
			}
		}

		//�������
		if (comp)
		{
			cout << "���������ȷ" << endl;
		}

		else
		{
			cout << "����������" << endl;
		}
		cout << "˫��������ʱ��Ϊ��" << timesort << endl;
		cout << "���ղ�����������ʱ��Ϊ��" << timesort + timemax + timesum << endl;
	}

	closesocket(newConnection);
	WSACleanup();
	return 0;
}

