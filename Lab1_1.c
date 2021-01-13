#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define NORTH 0
#define WEST 1
#define SOUTH 2
#define EAST 3
#define MAXTHREAD 100

typedef struct queue
{
	int capacity; //队列容量
	int size;	  //队列内元素数量
	int front;	  //队列头部
	int rear;	  //队列尾部
	int *element; //对列内储存线程ID
} * Queue;

Queue createQueue(int maxElements); //创建队列
void clearQueue(Queue q);			//清空队列
void enqueue(int x, Queue q);		//入队
int dequeue(Queue q);				//出队
int front(Queue q);					//获取头部元素

Queue createQueue(int maxElements)
{
	Queue q = (Queue)malloc(sizeof(struct queue));
	q->element = (int *)malloc(sizeof(int) * maxElements); //分配数组空间
	q->capacity = maxElements;							   //最大容量为maxElements
	q->size = 0;										   //初始容量为0
	q->front = 1;										   //头指针指向1位置
	q->rear = 0;										   //尾指针指向0位置
	return q;
}
//清空队列
void clearQueue(Queue q)
{
	free(q->element); //释放内部数组空间
	free(q);		  //释放队列指针
}
//求value的下一个队列索引
static int succ(int value, Queue q)
{
	if (++value == q->capacity)
		value = 0; //如果已经到达了数组末尾则下一个索引在数组开头
	return value;
}
//元素X入队
void enqueue(int x, Queue q)
{
	if (q->size == q->capacity)
		return; //若队列已满 则不能再往里添加元素
	else
	{
		q->size++;					//队列元素数量加一
		q->rear = succ(q->rear, q); //队列尾指针后移一位
		q->element[q->rear] = x;	//将元素插入队列尾
	}
}
//出队
int dequeue(Queue q)
{
	int res;
	if (q->size == 0)
		return -1; //若队列为空 则不能从队列里弹出元素
	else
	{
		res = q->element[q->front];	  //取出队列开头元素
		q->size--;					  //队列元素数量减一
		q->front = succ(q->front, q); //队列头指针后移一位
		return res;
	}
}
//获取队列开头的元素
int front(Queue q)
{
	if (q->size == 0) //若队列为空 返回-1
		return -1;
	else //若队列不为空 返回头值
		return q->element[q->front];
}

//车辆线程的信息
typedef struct carThreadInfo
{
	int direction; //车辆所属方向
	int number;	   //线程ID
} * Info;

//pthread_cond_t  queue[4];

//访问四个象限的互斥变量
pthread_mutex_t mutex_a;
pthread_mutex_t mutex_b;
pthread_mutex_t mutex_c;
pthread_mutex_t mutex_d;

pthread_cond_t cond_first[4];	 //下次通行车辆的条件变量
pthread_mutex_t mutex_dir[4];	 //访问某条方向的道路的互斥锁
pthread_mutex_t mutex_cross;	 //访问道路剩余资源crossSource的互斥锁
pthread_cond_t cond_deadLcok;	 //死锁检测的唤醒变量
pthread_mutex_t mutext_deadLock; //访问死锁检测唤醒变量的互斥锁

pthread_t thread_deadLockCheck; //死锁检测线程

int flag[4];	 //表示本方向前面是否有车辆正在形式（有车辆为1，否则为0）
int crossSource; //路口资源数量，初始化为4，如果为0则代表死锁发生

pthread_mutex_t mutex_wait[4]; //访问四个等待队列的互斥锁
Queue Queue_wait[4];		   //四个方向上的等待队列

void *CarThreadFun(void *info)
{
	Info i = (Info)info;
	int dir = i->direction; //线程所属方向
	int ID = i->number;		//线程ID
	char *dirS;				//输出方向用的字符串
	if (dir == NORTH)
		dirS = "North"; //若为北边来车 则设为North
	else if (dir == SOUTH)
		dirS = "South"; //若为南边来车 则设为South
	else if (dir == EAST)
		dirS = "East"; //若为东边来车 则设为East
	else if (dir == WEST)
		dirS = "West"; //若为西边来车 则设为West

	pthread_mutex_lock(&mutex_wait[dir]); //访问等待队列的互斥锁
	enqueue(ID, Queue_wait[dir]);		  //将本车辆加入等待队列中
	pthread_mutex_unlock(&mutex_wait[dir]);

	//DEBUG
	//printf("Before: while(front(Queue_wait[dir])!=ID);\n");

	while (front(Queue_wait[dir]) != ID);													   //若同方向上，队列头部还有其他线程，则等待
	printf("Car %d from %s arrives at crossing.\n", ID, dirS); //当前线程进入队列头，发送提示

	//DEBUG
	//printf("pthread_mutex_lock(&mutex_dir[dir]); \n");
	pthread_mutex_lock(&mutex_dir[dir]); //向该方向的道路加锁

	//以下三种情况需要等待，由于相反方向可以同时通车，因此不需要等待
	while (flag[dir]); //若本方向上有车正在通行，则等待
	while (flag[(dir + 1) % 4]); //若右侧有车正在通行，则等待
	while (flag[(dir + 3) % 4]); //若左侧有车正在通行，也需要等待（虽然右侧车优先级高于左侧车，但是不应当抢占）

	//旧版本下，flag设置在这里会导致死锁，结束部分的条件变量因为互斥锁无法访问，而其他线程由于卡在上方的while循环中无法解锁
	//flag[dir]=1;                     //本方向有车辆正在通信的信号

	//占用道路资源，注意此时尚未发车，是在发车前的检查，用于检测死锁是否发生（一旦死锁发生则不能回退，则必须在发车前检查）
	pthread_mutex_lock(&mutex_cross);
	//DEBUG
	//printf("crossSource--;  %s \n",dirS);
	crossSource--; //道路资源统计量减一
	pthread_mutex_unlock(&mutex_cross);
	if (!crossSource) //四个方向都要占用资源，发生死锁
	{
		printf("DEADLOCK: car jam detected, let North to go first.\n");
		pthread_mutex_lock(&mutext_deadLock);
		pthread_cond_signal(&cond_deadLcok); //触发死锁进程
		pthread_mutex_unlock(&mutext_deadLock);
	}

	//如果右侧有车，则应当右侧先行
	if (Queue_wait[(dir + 1) % 4]->size != 0)
	{
		pthread_cond_wait(&cond_first[dir], &mutex_dir[dir]); //等待右侧车辆（或死锁进程）向自己发送可以通行的信号，避免饥饿
	}

	//DEBUG
	//printf("before lock\n");

	//此时开始发车
	flag[dir] = 1; //本方向有车辆正在通信的信号

	if (dir == NORTH)
	{
		//进入第一段路
		pthread_mutex_lock(&mutex_c);
		pthread_mutex_lock(&mutex_wait[dir]);
		//这一段话表示车辆已经进入路口
		printf("Car %d from %s is going to get into the crossing.\n", ID, dirS);
		//车辆从等待队列中弹出
		dequeue(Queue_wait[dir]);
		pthread_mutex_unlock(&mutex_wait[dir]);

		usleep(500); //过一段路时间0.5ms
		//这一段话表示车辆已经通过第一段路
		printf("Car %d from %s has gone through part c.\n", ID, dirS);
		pthread_mutex_unlock(&mutex_c);

		//第一段路走完，进入第二段路
		pthread_mutex_lock(&mutex_d);
		usleep(500); //过一段路时间0.5ms
		//这一段话表示车辆已经通过第二段路
		printf("Car %d from %s has gone through part d.\n", ID, dirS);
		pthread_mutex_unlock(&mutex_d);

		crossSource++; //通过路口，释放资源
	}
	else if (dir == SOUTH)
	{
		//进入第一段路
		pthread_mutex_lock(&mutex_a);
		pthread_mutex_lock(&mutex_wait[dir]);
		//这一段话表示车辆已经进入路口
		printf("Car %d from %s is going to get into the crossing.\n", ID, dirS);
		//车辆从等待队列中弹出
		dequeue(Queue_wait[dir]);
		pthread_mutex_unlock(&mutex_wait[dir]);

		usleep(500); //过一段路时间0.5ms
		//这一段话表示车辆已经通过第一段路
		printf("Car %d from %s has gone through part a.\n", ID, dirS);
		pthread_mutex_unlock(&mutex_a);

		//第一段路走完，进入第二段路
		pthread_mutex_lock(&mutex_b);
		usleep(500); //过一段路时间500ms
		//这一段话表示车辆已经通过第二段路
		printf("Car %d from %s has gone through part b.\n", ID, dirS);
		pthread_mutex_unlock(&mutex_b);

		crossSource++; //通过路口，释放资源
	}
	else if (dir == WEST)
	{
		//进入第一段路
		pthread_mutex_lock(&mutex_d);
		pthread_mutex_lock(&mutex_wait[dir]);
		//这一段话表示车辆已经进入路口
		printf("Car %d from %s is going to get into the crossing.\n", ID, dirS);
		//车辆从等待队列中弹出
		dequeue(Queue_wait[dir]);
		pthread_mutex_unlock(&mutex_wait[dir]);

		usleep(500); //过一段路时间0.5ms
		//这一段话表示车辆已经通过第一段路
		printf("Car %d from %s has gone through part d.\n", ID, dirS);
		pthread_mutex_unlock(&mutex_d);

		//第一段路走完，进入第二段路
		pthread_mutex_lock(&mutex_a);
		usleep(500); //过一段路时间500ms
		//这一段话表示车辆已经通过第二段路
		printf("Car %d from %s has gone through part a.\n", ID, dirS);
		pthread_mutex_unlock(&mutex_a);

		crossSource++; //通过路口，释放资源
	}
	else if (dir == EAST)
	{
		//进入第一段路
		pthread_mutex_lock(&mutex_b);
		pthread_mutex_lock(&mutex_wait[dir]);
		//这一段话表示车辆已经进入路口
		printf("Car %d from %s is going to get into the crossing.\n", ID, dirS);
		dequeue(Queue_wait[dir]);
		pthread_mutex_unlock(&mutex_wait[dir]);

		usleep(500); //过一段路时间500ms
		//这一段话表示车辆已经通过第一段路
		printf("Car %d from %s has gone through part d.\n", ID, dirS);
		pthread_mutex_unlock(&mutex_b);

		//第一段路走完，进入第二段路
		pthread_mutex_lock(&mutex_c);
		usleep(500); //过一段路时间500ms
		//这一段话表示车辆已经通过第二段路
		printf("Car %d from %s has gone through part c.\n", ID, dirS);
		pthread_mutex_unlock(&mutex_c);

		crossSource++; //通过路口，释放资源
	}

	//Debug:
	//printf("flag[dir]=0; %s \n",dirS);

	flag[dir] = 0;

	//发出信号令左侧的车辆通行
	//pthread_mutex_lock(&mutex_dir[(dir+3)%4]);
	pthread_cond_signal(&cond_first[(dir + 3) % 4]);
	//pthread_mutex_unlock(&mutex_dir[(dir+3)%4]);

	pthread_mutex_unlock(&mutex_dir[dir]); //当前方向解锁

	//DEBUG
	//printf("%s end\n",dirS);
}

//死锁处理线程的函数
void *checkDeadLockFun()
{
	//反复执行
	while (1)
	{
		pthread_mutex_lock(&mutex_cross);
		//阻塞，等待唤醒
		pthread_cond_wait(&cond_deadLcok, &mutex_cross);
		//唤醒后，首先让北方车辆通信，利用条件变量激活北方的车辆
		pthread_mutex_lock(&mutex_dir[NORTH]);
		pthread_cond_signal(&cond_first[NORTH]);
		pthread_mutex_unlock(&mutex_dir[NORTH]);
		pthread_mutex_unlock(&mutex_cross);
	}
}

int main()
{
	int ID_start = 1;
	char input[MAXTHREAD];
	pthread_t carThread[MAXTHREAD];

	for (int i = 0; i < 4; i++)
	{
		//pthread_cond_init(&cond_running[i],NULL);
		pthread_cond_init(&cond_first[i], NULL);  //初始化下次通行车辆的条件变量
		pthread_mutex_init(&mutex_dir[i], NULL);  //初始化各方向上资源的互斥量
		pthread_mutex_init(&mutex_wait[i], NULL); //初始化队列资源的互斥量
		Queue_wait[i] = createQueue(MAXTHREAD);	  //初始化四个方向的等待队列
		//flag[i]=0;
		crossSource = 4;
	}
	pthread_cond_init(&cond_deadLcok, NULL);	//初始化死锁检测线程唤醒的条件变量
	pthread_mutex_init(&mutext_deadLock, NULL); //初始化访问死锁检测线程唤醒条件变量的互斥量
	pthread_mutex_init(&mutex_cross, NULL);		//初始化道路剩余资源crossSource的互斥锁
	pthread_mutex_init(&mutex_a, NULL);			//初始化公有资源a互斥量
	pthread_mutex_init(&mutex_b, NULL);			//初始化公有资源b互斥量
	pthread_mutex_init(&mutex_c, NULL);			//初始化公有资源c互斥量
	pthread_mutex_init(&mutex_d, NULL);			//初始化公有资源d互斥量

	//创建死锁线程
	if (pthread_create(&thread_deadLockCheck, NULL, checkDeadLockFun, NULL) != 0)
	{
		printf("Can't create deadlock thread!");
	}
	//读取输入
	scanf("%s", &input);
	for (int i = 0; i < strlen(input); i++)
	{
		Info thisInfo = (Info)malloc(sizeof(struct carThreadInfo)); //创建车辆（线程）信息
		if (input[i] == 'n')
		{ //如果输入为n，代表新来的车是从北边来的
			thisInfo->direction = NORTH;
		}
		else if (input[i] == 's')
		{ //如果输入为s，代表新来的车是从南边来的
			thisInfo->direction = SOUTH;
			WEST;
		}
		else if (input[i] == 'e')
		{ //如果输入为e，代表新来的车是从东边来的
			thisInfo->direction = EAST;
		}
		else if (input[i] == 'w')
		{ //如果输入为w，代表新来的车是从西边来的
			thisInfo->direction = WEST;
		}
		thisInfo->number = ID_start++; //赋值车辆线程的ID
		//enqueue(thisInfo->number,Queue_wait[thisInfo->direction]);	//将本车辆加入等待队列中
		int err = pthread_create(&carThread[i], NULL, CarThreadFun, thisInfo); //创建车辆（线程）
		if (err != 0)
		{
			printf("Can't create car %d! %s\n", thisInfo->number, strerror(err));
		}
	}
	for (int i = 0; i < strlen(input); i++)
	{
		pthread_join(carThread[i], NULL); //等待所有已经创建的线程结束
	}
	for (int i = 0; i < 4; i++)
	{
		clearQueue(Queue_wait[i]); //清空等待队列
	}
	printf("Program END\n");
	return 0;
}
