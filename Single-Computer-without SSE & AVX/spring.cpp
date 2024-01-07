// 2150248-Ҧ����-�Զ��� 2151276-����̩-�Զ���
#include <iostream>
#include<time.h>
#include<cmath>
#include<thread>
#include <windows.h>
#define MAX_THREADS 64
#define SUBDATANUM 2000000
#define DATANUM (SUBDATANUM*MAX_THREADS)
using namespace std;

float rawFloatData[DATANUM];
float sortmid[DATANUM];
float summid[MAX_THREADS];
float maxmid[MAX_THREADS];
HANDLE hThreads[MAX_THREADS];
HANDLE hSemaphores[MAX_THREADS];
HANDLE g_hHandle;

/*�����࣬���а���������̬��Ա*/
class bubble {
public:
	static float sum;
	static float max;
	bubble();
	void bubblesum();
	void bubblemax();
	void bubblesort();
	int bubblebasesum();
	int bubblebasemax();
	int bubblebasesort();
	int bubblethreadssum();
	int bubblethreadsmax();
	int bubblethreadssort();
	void sortinsert(float* p1, float* p2);
	bool sortmerge(int);
	int timesum = 0;
	int timemax = 0;
	int timesort = 0;
};
float bubble::sum = 0.0f;
float bubble::max = 0.0f;

/***************************************����ĳ�ʼ������˳��*************************************************/

bubble::bubble()
{
	for (size_t i = 0; i < DATANUM; i++)
	{
		rawFloatData[i] = float(i + 1);
	}
	for (size_t i = 0; i < DATANUM; i++)
	{
		sortmid[i] = 0;
	}

	srand(1234567);  //α��������ӣ�ע�ⲻ����ʱ�������ӣ����������޷�ͬ��
	float  tmp;
	size_t  T = DATANUM;
	while (T--)
	{
		size_t i, j;
		i = (size_t(rand()) * size_t(rand())) % DATANUM;
		j = (size_t(rand()) * size_t(rand())) % DATANUM;
		tmp = rawFloatData[i];
		rawFloatData[i] = rawFloatData[j];
		rawFloatData[j] = tmp;
	}
	cout << "��ʼ������" << endl;
}

/***************************************�������*************************************************/

void bubble::bubblesum()
{

	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		summid[i] = 0.0f;
	}

	for (size_t j = 0; j < MAX_THREADS; j++)
	{
		for (size_t i = 0; i < SUBDATANUM; i++)
		{
			summid[j] += log(sqrt(rawFloatData[j * SUBDATANUM + i]));
		}
	}

	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		sum += summid[i];
	}
}

/***************************************���������ֵ*************************************************/

void bubble::bubblemax()
{

	for (size_t i = 0; i < DATANUM; i++)
	{

		if (rawFloatData[i] > max)
		{
			max = rawFloatData[i];
		}
	}
}

/***************************************���п�������*************************************************/

void Swap(float* a, float* b)
{//�������е��������ݽ��н���
	float temp = *b;
	*b = *a;
	*a = temp;
}

int MidNum(float* a, int b, int c, int d)
{
	float n_1 = a[b], n_2 = a[c], n_3 = a[d];
	float temp[3] = { n_1,n_2,n_3 };
	if (temp[0] > temp[1]) {
		Swap(&temp[0], &temp[1]);
	}
	if (temp[1] > temp[2]) {
		Swap(&temp[1], &temp[2]);
	}
	if (temp[0] > temp[1]) {
		Swap(&temp[0], &temp[1]);
	}
	if (temp[1] == a[b]) {
		return b;
	}
	if (temp[1] == a[c]) {
		return c;
	}
	return d;
}

int PartSort(float* a, int left, int right)
{
	int cur, prev, mid = MidNum(a, left, (left + right) / 2, right);//����ȡ��
	Swap(&a[left], &a[mid]);//����׼���ŵ��ײ�
	prev = left;//����ǰ��ָ��
	cur = prev + 1;
	while (cur <= right) {
		if (a[cur] < a[left] && ++prev != cur) {//cur�ұȱ�׼��С��,prev�ұȱ�׼�����Ȼ�󽻻�
			Swap(&a[cur], &a[prev]);
		}
		cur++;
	}
	Swap(&a[left], &a[prev]);//prev���ڵ�λ����Զ��С�ڱ�׼�������һλ,����׼�����佻�������м�
	return prev;
}

typedef int DataType;
typedef struct Stack {//����ջ
	DataType a[10000];
	int size;
}Stack;

void InitStack(Stack* s) {//��ʼ��ջ
	s->size = 0;
}

void StackPush(Stack* s, DataType val)
{//��ջ
	s->a[s->size++] = val;
}

void StackPop(Stack* s) {//��ջ
	s->size--;
}

DataType StackTop(Stack* s) {//��ȡջ��Ԫ��
	return s->a[s->size - 1];
}

void QuickSortNoR(float* a, int left, int right)
{//�ǵݹ����
	if (left >= right) {//����Ԫ��С�ڵ���1ֱ���˳�
		return;
	}
	Stack s;//����ջ
	InitStack(&s);//��ʼ��ջ
	//������������յ���ջ,��Ϊջ��������������,Ϊ�˷������ʹ����right����,left����.
	StackPush(&s, right);
	StackPush(&s, left);
	while (0 != s.size) {//��ջ��Ϊ������ʼģ�µݹ���ý���ѭ������.
		int i = StackTop(&s);//�������е�����Ԫ��ȡ��,��Ϊ��һ�����俪ʼ���ÿ���.
		StackPop(&s);
		int j = StackTop(&s);
		StackPop(&s);
		int mid = PartSort(a, i, j);//����һ�ο���,�����ر�׼�����ڵ�λ��.
		if (mid > i + 1) {//�жϱ�׼������Ƿ񻹴�������,���ھͽ��������յ���ջ.
			StackPush(&s, mid - 1);
			StackPush(&s, i);
		}
		if (mid < j - 1) {//�жϱ�׼���Ҳ��Ƿ񻹴�������,���ھͽ��������յ���ջ.
			StackPush(&s, j);
			StackPush(&s, mid + 1);
		}
	}
}

void bubble::bubblesort()
{
	for (size_t i = 0; i < DATANUM - 1; i++)
	{
		for (size_t j = 0; j < DATANUM - i - 1; j++)
		{
			if (rawFloatData[j] > rawFloatData[j + 1])
			{
				float tmp = rawFloatData[j];
				rawFloatData[j] = rawFloatData[j + 1];
				rawFloatData[j + 1] = tmp;
			}
		}
	}
}

/***************************************��������������*************************************************/

int bubble::bubblebasesum()
{
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);//start  
	bubblesum();
	cout << "����������ͽ��Ϊ:" << sum << endl;
	QueryPerformanceCounter(&end);//end
	std::cout << "�����������ʱ��Ϊ:" << (end.QuadPart - start.QuadPart) << endl;
	timesum = (end.QuadPart - start.QuadPart);
	sum = 0.0;
	max = 0.0;
	return timesum;
}

int bubble::bubblebasemax()
{
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);//start  
	bubblemax();
	cout << "�������������ֵ���Ϊ:" << max << endl;
	QueryPerformanceCounter(&end);//end
	std::cout << "�������������ֵʱ��Ϊ:" << (end.QuadPart - start.QuadPart) << endl;
	timemax = (end.QuadPart - start.QuadPart);
	sum = 0.0;
	max = 0.0;
	return timemax;
}

int bubble::bubblebasesort()
{
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);//start  
	QuickSortNoR(rawFloatData, 0, DATANUM - 1);
	bool comp = true;

	for (size_t i = 0; i < DATANUM - 1; i++)//�ж������Ƿ���ȷ
	{

		if (rawFloatData[i] > rawFloatData[i + 1])
		{
			comp = false;
			break;
		}
	}

	if (comp)
	{
		cout << "��������������ȷ" << endl;
	}

	else
	{
		cout << "���������������" << endl;
	}

	QueryPerformanceCounter(&end);//end
	std::cout << "������������ʱ��Ϊ:" << (end.QuadPart - start.QuadPart) << endl;
	timesort = (end.QuadPart - start.QuadPart);
	sum = 0.0;
	max = 0.0;
	return timesort;
}

/***************************************���̣߳�64������*************************************************/

DWORD WINAPI Threadsort(LPVOID lpParameter)
{
	float* p = (float*)lpParameter;
	QuickSortNoR(p, 0, SUBDATANUM - 1);
	return 0;
}

/***************************************���̣߳�64�����*************************************************/

// �̺߳�����������ȡ������ĺ�
DWORD WINAPI Threadsum(LPVOID lpParameter)
{
	WaitForSingleObject(g_hHandle, INFINITE);
	float* p = (float*)lpParameter;
	long long int j = (p - &rawFloatData[0]) / SUBDATANUM;
	for (size_t i = 0; i < SUBDATANUM; i++)
	{
		summid[j] += log(sqrt(p[i]));
	}
	ReleaseMutex(g_hHandle);
	return 0;
}

/***************************************���̣߳�64�������ֵ*************************************************/

// �̺߳�����������ȡ����������ֵ
DWORD WINAPI Threadmax(LPVOID lpParameter)
{
	float* p = (float*)lpParameter;
	int j = (p - rawFloatData) / SUBDATANUM;

	WaitForSingleObject(g_hHandle, INFINITE);  // �ȴ�������

	for (size_t i = 0; i < SUBDATANUM; i++)
	{
		if (maxmid[j] < p[i])
		{
			maxmid[j] = p[i];
		}
	}
	ReleaseMutex(g_hHandle);  // �ͷŻ�����
	return 0;
}

/*��������������������Ļ�����Ԫ����������������������*/

void bubble::sortinsert(float* p1, float* p2)
{
	long long int length = p2 - p1;
	long long int i = 0;
	long long int j = 0;

	while (i < length && j < length)
	{
		if (p1[i] <= p2[j])
		{
			sortmid[i + j] = p1[i];
			i++;
		}
		else
		{
			sortmid[i + j] = p2[j];
			j++;
		}
	}

	while (i < length)
	{
		sortmid[i + j] = p1[i];
		i++;
	}

	while (j < length)
	{
		sortmid[i + j] = p2[j];
		j++;
	}

	for (size_t i = 0; i < 2 * length; i++)
	{
		p1[i] = sortmid[i];
	}
}

/***************************************�����򣨵��߳̽϶�ʱ����ݹ�ִ�У���ͬʱ��֤�������ȷ��*************************************************/

bool bubble::sortmerge(int Threads)
{
	bool comp = true;
	if (Threads > 1)
	{
		sortmerge(Threads / 2);   // �ݹ���ã��������Ϊ��С����������й鲢����
	}

	// �����ڵ���������й鲢����
	for (size_t i = 0; i < MAX_THREADS / Threads; i += 2)
	{
		sortinsert(&rawFloatData[i * Threads * SUBDATANUM], &rawFloatData[(i + 1) * Threads * SUBDATANUM]);  // ��������������в�������
	}

	// �����һ�ι鲢����󣬼�������Ƿ��Ѿ���ȫ����
	if (Threads == MAX_THREADS / 2)
	{
		for (size_t i = 0; i < DATANUM - 1; i++)
		{
			if (rawFloatData[i] > rawFloatData[i + 1])
			{
				comp = false;   // �������������ԣ����ʾ������ȷ
				break;
			}
		}
	}
	return comp;
}

int bubble::bubblethreadssum()
{

	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		summid[i] = 0.0f;
	}
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);//start  
	g_hHandle = CreateMutex(NULL, FALSE, NULL);  // ����������

	// ����������ͼ�����߳�
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
	WaitForMultipleObjects(MAX_THREADS, hThreads, TRUE, INFINITE);   // �ȴ������߳̽���

	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		sum += summid[i];
	}
	cout << "����������ͽ��Ϊ:" << sum << endl;
	QueryPerformanceCounter(&end);//end
	std::cout << "�����������ʱ��Ϊ:" << (end.QuadPart - start.QuadPart) << endl;
	timesum = (end.QuadPart - start.QuadPart);

	// �����ر��߳̾��
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		CloseHandle(hThreads[i]);
	}
	sum = 0.0f;
	max = 0.0f;
	return timesum;
}

int bubble::bubblethreadsmax()
{

	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);//start  
	g_hHandle = CreateMutex(NULL, FALSE, NULL);   // ����������

	// �������������ֵ���߳�
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

	WaitForMultipleObjects(MAX_THREADS, hThreads, true, INFINITE);   // �ȴ������߳̽���

	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		if (max < maxmid[i])
		{
			max = maxmid[i];
		}
	}
	cout << "�������������ֵ���Ϊ:" << max << endl;
	QueryPerformanceCounter(&end);//end
	std::cout << "�������������ֵʱ��Ϊ:" << (end.QuadPart - start.QuadPart) << endl;
	timemax = (end.QuadPart - start.QuadPart);

	// �����ر��߳̾��
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		CloseHandle(hThreads[i]);
	}
	return timemax;
}

int bubble::bubblethreadssort()
{
	// ��ʼ�� summid ����
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		summid[i] = 0.0f;
	}
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);//start  
	g_hHandle = CreateMutex(NULL, FALSE, NULL);   // ����������

	// ��������������߳�
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

	WaitForMultipleObjects(MAX_THREADS, hThreads, true, INFINITE);	//�������̷߳���ʱ���ܽ���

	// ��������������ִ�й鲢����
	if (sortmerge(MAX_THREADS / 2))
	{
		cout << "��������������ȷ" << endl;
	}

	else
	{
		cout << "���������������" << endl;
	}

	QueryPerformanceCounter(&end);//end
	std::cout << "������������ʱ��Ϊ:" << (end.QuadPart - start.QuadPart) << endl;
	timesort = (end.QuadPart - start.QuadPart);

	// �����ر��߳̾��
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		CloseHandle(hThreads[i]);
	}
	sum = 0.0f;
	max = 0.0f;
	return timesort;
}

int main()
{
	int timemax1, timemax2, timesum1, timesum2, timesort1, timesort2;
	bubble test1;
	timesum1 = test1.bubblebasesum();//���Ե������е��������
	timemax1 = test1.bubblebasemax();//���Ե������е��������
	timesort1 = test1.bubblebasesort();//���Ե������е��������
	cout << "����������ʱ��Ϊ��" << timesum1 + timemax1 + timesort1 << endl;
	cout << endl;
	bubble test2;
	timesum2 = test2.bubblethreadssum();//���Ե������е��������
	timemax2 = test2.bubblethreadsmax();//���Ե������е��������
	timesort2 = test2.bubblethreadssort();//���Ե������е��������
	cout << "����������ʱ��Ϊ��" << timesum2 + timemax2 + timesort2 << endl;
	cout << endl;
	cout << "������������뵥��������ͼ��ٱ�Ϊ��" << double(timesum1) / double(timesum2) << endl;
	cout << "�������������ֵ�뵥�����������ֵ���ٱ�Ϊ��" << double(timemax1) / double(timemax2) << endl;
	cout << "�������������뵥������������ٱ�Ϊ��" << double(timesort1) / double(timesort2) << endl;
	cout << "���������뵥�������ܼ��ٱ�Ϊ��" << double(timesum1 + timemax1 + timesort1) / double(timesum2 + timemax2 + timesort2) << endl;
	return 0;
}
