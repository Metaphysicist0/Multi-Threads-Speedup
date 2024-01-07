// 2150248-姚天亮-自动化 2151276-吕泓泰-自动化
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

/*排序类，其中包含两个静态成员*/
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

/***************************************数组的初始化和乱顺序*************************************************/

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

	srand(1234567);  //伪随机数种子，注意不能用时间做种子，否则两机无法同步
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
	cout << "初始化结束" << endl;
}

/***************************************串行求和*************************************************/

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

/***************************************串行求最大值*************************************************/

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

/***************************************串行快速排序*************************************************/

void Swap(float* a, float* b)
{//对数组中的两个数据进行交换
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
	int cur, prev, mid = MidNum(a, left, (left + right) / 2, right);//三数取中
	Swap(&a[left], &a[mid]);//将标准数放到首部
	prev = left;//定义前后指针
	cur = prev + 1;
	while (cur <= right) {
		if (a[cur] < a[left] && ++prev != cur) {//cur找比标准数小的,prev找比标准数大的然后交换
			Swap(&a[cur], &a[prev]);
		}
		cur++;
	}
	Swap(&a[left], &a[prev]);//prev所在的位置永远在小于标准数的最后一位,将标准数与其交换放在中间
	return prev;
}

typedef int DataType;
typedef struct Stack {//定义栈
	DataType a[10000];
	int size;
}Stack;

void InitStack(Stack* s) {//初始化栈
	s->size = 0;
}

void StackPush(Stack* s, DataType val)
{//入栈
	s->a[s->size++] = val;
}

void StackPop(Stack* s) {//出栈
	s->size--;
}

DataType StackTop(Stack* s) {//获取栈顶元素
	return s->a[s->size - 1];
}

void QuickSortNoR(float* a, int left, int right)
{//非递归调用
	if (left >= right) {//数组元素小于等于1直接退出
		return;
	}
	Stack s;//创建栈
	InitStack(&s);//初始化栈
	//将数组的起点和终点入栈,因为栈是先入后出的特性,为了方便后续使用让right先入,left后入.
	StackPush(&s, right);
	StackPush(&s, left);
	while (0 != s.size) {//以栈空为条件开始模仿递归调用进行循环调用.
		int i = StackTop(&s);//将数组中的两个元素取出,作为第一个区间开始调用快排.
		StackPop(&s);
		int j = StackTop(&s);
		StackPop(&s);
		int mid = PartSort(a, i, j);//调用一次快排,并返回标准数所在的位置.
		if (mid > i + 1) {//判断标准数左侧是否还存在区间,存在就将其起点和终点入栈.
			StackPush(&s, mid - 1);
			StackPush(&s, i);
		}
		if (mid < j - 1) {//判断标准数右侧是否还存在区间,存在就将其起点和终点入栈.
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

/***************************************单机串行主函数*************************************************/

int bubble::bubblebasesum()
{
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);//start  
	bubblesum();
	cout << "单机串行求和结果为:" << sum << endl;
	QueryPerformanceCounter(&end);//end
	std::cout << "单机串行求和时间为:" << (end.QuadPart - start.QuadPart) << endl;
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
	cout << "单机串行求最大值结果为:" << max << endl;
	QueryPerformanceCounter(&end);//end
	std::cout << "单机串行求最大值时间为:" << (end.QuadPart - start.QuadPart) << endl;
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

	for (size_t i = 0; i < DATANUM - 1; i++)//判断排序是否正确
	{

		if (rawFloatData[i] > rawFloatData[i + 1])
		{
			comp = false;
			break;
		}
	}

	if (comp)
	{
		cout << "单机串行排序正确" << endl;
	}

	else
	{
		cout << "单机串行排序错误" << endl;
	}

	QueryPerformanceCounter(&end);//end
	std::cout << "单机串行排序时间为:" << (end.QuadPart - start.QuadPart) << endl;
	timesort = (end.QuadPart - start.QuadPart);
	sum = 0.0;
	max = 0.0;
	return timesort;
}

/***************************************多线程（64）排序*************************************************/

DWORD WINAPI Threadsort(LPVOID lpParameter)
{
	float* p = (float*)lpParameter;
	QuickSortNoR(p, 0, SUBDATANUM - 1);
	return 0;
}

/***************************************多线程（64）求和*************************************************/

// 线程函数，用于求取子数组的和
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

/***************************************多线程（64）求最大值*************************************************/

// 线程函数，用于求取子数组的最大值
DWORD WINAPI Threadmax(LPVOID lpParameter)
{
	float* p = (float*)lpParameter;
	int j = (p - rawFloatData) / SUBDATANUM;

	WaitForSingleObject(g_hHandle, INFINITE);  // 等待互斥锁

	for (size_t i = 0; i < SUBDATANUM; i++)
	{
		if (maxmid[j] < p[i])
		{
			maxmid[j] = p[i];
		}
	}
	ReleaseMutex(g_hHandle);  // 释放互斥锁
	return 0;
}

/*插入排序函数，是重排序的基本单元，对两个有序数组重排序*/

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

/***************************************重排序（当线程较多时，会递归执行），同时验证排序的正确性*************************************************/

bool bubble::sortmerge(int Threads)
{
	bool comp = true;
	if (Threads > 1)
	{
		sortmerge(Threads / 2);   // 递归调用，将数组分为更小的子数组进行归并排序
	}

	// 对相邻的子数组进行归并排序
	for (size_t i = 0; i < MAX_THREADS / Threads; i += 2)
	{
		sortinsert(&rawFloatData[i * Threads * SUBDATANUM], &rawFloatData[(i + 1) * Threads * SUBDATANUM]);  // 对两个子数组进行插入排序
	}

	// 在最后一次归并排序后，检查数组是否已经完全排序
	if (Threads == MAX_THREADS / 2)
	{
		for (size_t i = 0; i < DATANUM - 1; i++)
		{
			if (rawFloatData[i] > rawFloatData[i + 1])
			{
				comp = false;   // 如果发现有逆序对，则表示排序不正确
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
	g_hHandle = CreateMutex(NULL, FALSE, NULL);  // 创建互斥锁

	// 创建用于求和计算的线程
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
	WaitForMultipleObjects(MAX_THREADS, hThreads, TRUE, INFINITE);   // 等待所有线程结束

	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		sum += summid[i];
	}
	cout << "单机并行求和结果为:" << sum << endl;
	QueryPerformanceCounter(&end);//end
	std::cout << "单机并行求和时间为:" << (end.QuadPart - start.QuadPart) << endl;
	timesum = (end.QuadPart - start.QuadPart);

	// 清理：关闭线程句柄
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
	g_hHandle = CreateMutex(NULL, FALSE, NULL);   // 创建互斥锁

	// 创建用于求最大值的线程
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

	WaitForMultipleObjects(MAX_THREADS, hThreads, true, INFINITE);   // 等待所有线程结束

	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		if (max < maxmid[i])
		{
			max = maxmid[i];
		}
	}
	cout << "单机并行求最大值结果为:" << max << endl;
	QueryPerformanceCounter(&end);//end
	std::cout << "单机并行求最大值时间为:" << (end.QuadPart - start.QuadPart) << endl;
	timemax = (end.QuadPart - start.QuadPart);

	// 清理：关闭线程句柄
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		CloseHandle(hThreads[i]);
	}
	return timemax;
}

int bubble::bubblethreadssort()
{
	// 初始化 summid 数组
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		summid[i] = 0.0f;
	}
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);//start  
	g_hHandle = CreateMutex(NULL, FALSE, NULL);   // 创建互斥锁

	// 创建用于排序的线程
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

	WaitForMultipleObjects(MAX_THREADS, hThreads, true, INFINITE);	//当所有线程返回时才能结束

	// 对排序后的子数组执行归并排序
	if (sortmerge(MAX_THREADS / 2))
	{
		cout << "单机并行排序正确" << endl;
	}

	else
	{
		cout << "单机并行排序错误" << endl;
	}

	QueryPerformanceCounter(&end);//end
	std::cout << "单机并行排序时间为:" << (end.QuadPart - start.QuadPart) << endl;
	timesort = (end.QuadPart - start.QuadPart);

	// 清理：关闭线程句柄
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
	timesum1 = test1.bubblebasesum();//测试单机串行的运行情况
	timemax1 = test1.bubblebasemax();//测试单机串行的运行情况
	timesort1 = test1.bubblebasesort();//测试单机串行的运行情况
	cout << "单机串行总时间为：" << timesum1 + timemax1 + timesort1 << endl;
	cout << endl;
	bubble test2;
	timesum2 = test2.bubblethreadssum();//测试单机串行的运行情况
	timemax2 = test2.bubblethreadsmax();//测试单机并行的运行情况
	timesort2 = test2.bubblethreadssort();//测试单机并行的运行情况
	cout << "单机并行总时间为：" << timesum2 + timemax2 + timesort2 << endl;
	cout << endl;
	cout << "单机并行求和与单机串行求和加速比为：" << double(timesum1) / double(timesum2) << endl;
	cout << "单机并行求最大值与单机串行求最大值加速比为：" << double(timemax1) / double(timemax2) << endl;
	cout << "单机并行排序与单机串行排序加速比为：" << double(timesort1) / double(timesort2) << endl;
	cout << "单机并行与单机串行总加速比为：" << double(timesum1 + timemax1 + timesort1) / double(timesum2 + timemax2 + timesort2) << endl;
	return 0;
}
