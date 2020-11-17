# include<linux/module.h>
# include<linux/kernel.h>
# include<linux/init.h>
# include<linux/list.h>
# include<linux/sched.h>
# include<linux/sched/task.h>

//内核模块初始化函数
static int __init lkp_init(void)
{
	printk("Insert the module successfully!\n");
	
	//开始对进程进行遍历和计数
	int process_sum=0;					//进程总数 
	int process_running_sum=0;			//状态为TASK_RUNNING的进程总数 
	int process_interrupible_sum=0;		//状态为TASK_INTERRUPTIBLE的进程总数 
	int process_uninterruptible_sum=0;	//状态为TASK_UNINTERRUPTIBLE的进程总数
	int process_stopped_sum=0;			//状态为TASK_STOPPED的进程总数
	int process_traced_sum=0;			//状态为TASK_TRACED的进程总数
	int process_zombie_sum=0;			//状态为EXIT_ZOMBIE的进程总数
	int process_dead_sum=0;				//状态为EXIT_DEAD的进程总数
	int process_others_sum=0;			//其他状态的进程总数
	long process_state;					//进程的可运行性状态 
	long process_exit_state;			//进程的退出状态 
	
	struct task_struct * task, * cur_process;
	struct list_head * pos;

	task =& init_task;						  //获取0号进程

	printk("#$# Process counter begin:\n");   //输出开始,#$#作为开始标记
	list_for_each(pos, &task->tasks)          //list_for_each对所有进程遍历
	{
		cur_process = list_entry(pos, struct task_struct, tasks);    //通过list_entry获取pos的父结构的地址
		//输出进程id与名称
		printk("@id: %d\n",cur_process->pid);
		printk("@name: %s\n",cur_process->comm);
		
		//输出父进程的id与名称
		printk("@parent id: %d\n",cur_process->parent->pid);
		printk("@parent name: %s\n",cur_process->parent->comm); 
		
		//进程总数计数加一
		process_sum++;
		
		//获取进程状态 
		process_state=cur_process->state; 
		process_exit_state=cur_process->exit_state;
		
		//判断进程的退出状态 
		switch(process_exit_state){
			case EXIT_ZOMBIE:
				process_zombie_sum++;//对状态为EXIT_ZOMBIE的进程计数
				printk("@state: EXIT_ZOMBIE\n"); 
				break;
			case EXIT_DEAD:
				process_dead_sum++;//对状态为EXIT_DEAD的进程计数
				printk("@state: EXIT_DEAD\n");
				break;
			default:
				break;
		}
		
		//若进程的状态为退出状态中的一种，则不可能为其它状态 
		if(process_exit_state){
			continue;
		}
		
		//若进程的状态不为退出状态中的一种，则可能为其他状态
		switch(process_state){
			case TASK_RUNNING:
				process_running_sum++;//对状态为TASK_RUNNING的进程计数
				printk("@state: TASK_RUNNING\n"); 
				break;
			case TASK_INTERRUPTIBLE:
				process_interrupible_sum++;//对状态为TASK_INTERRUPTIBLE的进程计数
				printk("@state: TASK_INTERRUPTIBLE\n"); 
				break;
			case TASK_UNINTERRUPTIBLE:
				process_uninterruptible_sum++;//对状态为TASK_UNINTERRUPTIBLE的进程计数
				printk("@state: TASK_UNINTERRUPTIBLE\n"); 
				break;
			case TASK_STOPPED:
				process_stopped_sum++;//对状态为TASK_STOPPED的进程计数
				printk("@state: TASK_STOPPED\n"); 
				break;
			case TASK_TRACED:
				process_traced_sum++;//对状态为TASK_TRACED的进程计数
				printk("@state: TASK_TRACED\n"); 
				break;
			default:
				process_others_sum++;//对其他状态的进程计数
				printk("@state: Others\n",process_state); 
				break;
		}
	}
	//输出进程总数
	printk("@Total number=%d\n",process_sum);
	//输出状态为TASK_RUNNING的进程总数
	printk("@TASK_RUNNING: %d\n",process_running_sum);
	//输出状态为TASK_INTERRUPTIBLE的进程总数 
	printk("@TASK_INTERRUPTIBLE: %d\n",process_interrupible_sum);
	//输出状态为TASK_UNINTERRUPTIBLE的进程总数 
	printk("@TASK_UNINTERRUPTIBLE: %d\n",process_uninterruptible_sum);
	//输出状态为TASK_STOPPED的进程总数 
	printk("@TASK_STOPPED: %d\n",process_stopped_sum);
	//输出状态为TASK_TRACED的进程总数 
	printk("@TASK_TRACED: %d\n",process_traced_sum);
	//输出状态为EXIT_ZOMBIE的进程总数 
	printk("@EXIT_ZOMBIE: %d\n",process_zombie_sum);
	//输出状态为EXIT_DEAD的进程总数 
	printk("@EXIT_DEAD: %d\n",process_dead_sum);
	//输出状态其他的进程总数
	printk("@UNKNOWN: %d\n",process_others_sum);
	
	printk("#*#Process counter ends\n");  //输出结束，*#*作为结束标记

	return 0;
}

//内核模块移除函数
static void __exit lkp_cleanup(void)
{
	printk("Remove the module successfully!\n");
}

module_init(lkp_init);
module_exit(lkp_cleanup);
MODULE_LICENSE("GPL");
