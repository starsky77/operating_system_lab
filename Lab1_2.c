# include<linux/module.h>
# include<linux/kernel.h>
# include<linux/init.h>
# include<linux/list.h>
# include<linux/sched.h>
# include<linux/sched/task.h>

static int print_pid(void)
{
	int process_count=0;					//操作系统进程总数 
	int process_count_running=0;			//状态为TASK_RUNNING的进程总数 
	int process_count_interrupible=0;		//状态为TASK_INTERRUPTIBLE的进程总数 
	int process_count_uninterruptible=0;	//状态为TASK_UNINTERRUPTIBLE的进程总数
	int process_count_stopped=0;			//状态为TASK_STOPPED的进程总数
	int process_count_traced=0;				//状态为TASK_TRACED的进程总数
	int process_count_zombie=0;				//状态为EXIT_ZOMBIE的进程总数
	int process_count_dead=0;				//状态为EXIT_DEAD的进程总数
	int process_count_unknown=0;			//状态为未知的进程总数
	long process_state;						//进程的可运行性状态 
	long process_exit_state;				//进程的退出状态 
	
	struct task_struct * task, * p;
	struct list_head * pos;
	int count = 0;
	printk("Hello World enter begin:\n");
	
	task =& init_task;
	list_for_each(pos, &task->tasks)
	{
		p = list_entry(pos, struct task_struct, tasks);
		printk("@id: %d\n",p->pid);
		printk("@name: %s\n",p->comm);
		
		//输出父进程名字和id
		printk("@parent id: %d\n",p->parent->pid);
		printk("@parent name: %s\n",p->parent->comm); 
		
		//进程总数计数加一
		process_count++;
		
		//获取进程状态 
		process_state=p->state; 
		process_exit_state=p->exit_state;
		
		//判断进程的退出状态 
		switch(process_exit_state){
			case EXIT_ZOMBIE:
				process_count_zombie++;//状态为EXIT_ZOMBIE的进程总数计数加一 
				break;
			case EXIT_DEAD:
				process_count_dead++;//状态为EXIT_DEAD的进程总数计数加一 
				break;
			default:
				break;
		}
		
		//若进程的状态为退出状态中的一种，则不可能为其它状态 
		if(process_exit_state){
			//输出进程状态
			printk("@state: %ld\n",process_exit_state); 
			continue;
		}
		
		//若进程的状态不为退出状态中的一种，则可能为其他状态
		switch(process_state){
			case TASK_RUNNING:
				process_count_running++;//状态为TASK_RUNNING的进程总数计数加一 
				break;
			case TASK_INTERRUPTIBLE:
				process_count_interrupible++;//状态为TASK_INTERRUPTIBLE的进程总数计数加一 
				break;
			case TASK_UNINTERRUPTIBLE:
				process_count_uninterruptible++;//状态为TASK_UNINTERRUPTIBLE的进程总数计数加一 
				break;
			case TASK_STOPPED:
				process_count_stopped++;//状态为TASK_STOPPED的进程总数计数加一 
				break;
			case TASK_TRACED:
				process_count_traced++;//状态为TASK_TRACED的进程总数计数加一 
				break;
			default:
				process_count_unknown++;//状态为未知的进程总数计数加一 
				break;
		}
		
		//输出进程状态
		printk("@state: %ld\n",process_state); 
	}
	//输出进程总数
	printk("@Total number=%d\n",process_count);
	//输出状态为TASK_RUNNING的进程总数
	printk("@TASK_RUNNING: %d\n",process_count_running);
	//输出状态为TASK_INTERRUPTIBLE的进程总数 
	printk("@TASK_INTERRUPTIBLE: %d\n",process_count_interrupible);
	//输出状态为TASK_UNINTERRUPTIBLE的进程总数 
	printk("@TASK_UNINTERRUPTIBLE: %d\n",process_count_uninterruptible);
	//输出状态为TASK_STOPPED的进程总数 
	printk("@TASK_STOPPED: %d\n",process_count_stopped);
	//输出状态为TASK_TRACED的进程总数 
	printk("@TASK_TRACED: %d\n",process_count_traced);
	//输出状态为EXIT_ZOMBIE的进程总数 
	printk("@EXIT_ZOMBIE: %d\n",process_count_zombie);
	//输出状态为EXIT_DEAD的进程总数 
	printk("@EXIT_DEAD: %d\n",process_count_dead);
	//输出状态为未知的进程总数
	printk("@UNKNOWN: %d\n",process_count_unknown);
	
	//结束标记 
	printk("<1>$*$MyProcessCounter ends\n");
	return 0;
}

static int __init lkp_init(void)
{
	printk("<1>Hello, World! from the kernel space...\n");
	print_pid();
	return 0;
}

static void __exit lkp_cleanup(void)
{
	printk("<1>Good Bye, World! leaving kernel space..\n");
}

module_init(lkp_init);
module_exit(lkp_cleanup);
MODULE_LICENSE("GPL");
