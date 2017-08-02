#include <linux/module.h>   
#include <linux/kernel.h>    
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/rwlock.h>
#include <linux/signal.h>
#include <linux/signalfd.h>
#include <linux/fbdheader.h>
#include <linux/signal.h>
#include <linux/delay.h>
#include <linux/sched.h>

// Define the maximum number of forked children your code can handle
#define MAXIMUM_FORKS 99

struct task_struct *threadMon;
struct task_struct *p;
int max_children;

MODULE_LICENSE("GPL");

int fb_detector(void);
void kill_family(struct task_struct *p);
int fb_killer(struct task_struct *target);
void debug_msg(const char* const msg);
static int main_work_thread(void* data);

void debug_msg(const char* const msg)
{
	// Print to log the message contained in msg
	printk("\n%s", msg);
}

// This function will loop through each child of a task 
// and return the total number of children it has.
unsigned numFork(struct task_struct* parent) 
{
	// Initialize an unsigned count variable to 0
	unsigned int count = 0;
	// Create a pointer of task_struct called th_child
	struct task_struct *th_child;
	// Create a pointer of list_head called list
	struct list_head *list;

   list_for_each(list, &parent->children) 
   {
	   //Use list_entry with the appropriate arguments to convert each entry of 
	   //list to that of type task_struct and store the output in th_child
	   th_child = list_entry(list, struct task_struct, sibling);

	   // Increment the value of count by adding count to the output of the function
	   // numFork() whose argument is th_child
	   count += numFork(th_child);
	   
	   // Increment count by one more to account for the task which started the counter 
	   // numFork()
	   count += 1;
   }
   return count;
}

void kill_family(struct task_struct* parent) 
{
	// Create a pointer of task_struct called th_child
	struct task_struct *th_child;
	// Create a pointer of list_head called list
	struct list_head *list;

    list_for_each(list, &parent->children) 
	{
		//Use list_entry with the appropriate arguments to convert each entry of 
	   //list to that of type task_struct and store the output in th_child
		th_child = list_entry(list, struct task_struct, sibling);
        
		// Call kill_family() passing the right pointer to kill
		kill_family(th_child);
    }
    printk("\nProcess with PID: %d has been killed", task_pid_nr(parent));
    
	// Send a signal to kill the task named parent
	//kill(-(task_pid_nr(parent)), SIGKILL);
	send_sig_info(SIGKILL, SEND_SIG_FORCED, parent);
}

// This function puts your fbd_killer thread to sleep
static int main_work_thread(void* data)
{
	do
	{
		//if (fbd_active == true)
		//{
			debug_msg("FBD_KILLER GOING BACK TO SLEEP");
			// Change the state of the semaphore sem to down using the apt function
			down(&sem);
			// Call the fb_detector() function
			fb_detector();
			//msleep(1000);
		//}
	}
	while(!kthread_should_stop());

	return 0;
}

int fb_detector(void)
{
	// Create a pointer of type task_struct
	struct task_struct *target;

	// Create an integer to hold the return value of numFork()
	unsigned int kount = 0;

    debug_msg("fb_detector called.");

	// Iterate through each process
    for_each_process(target)
	{
		// Call numFork() with the pointer of type task_struct and store the return 
		// value in the integer variable created in this function
		kount = numFork(target);
		//if (kount > 0)
        //printk("\n PID = %d, kount = %d", task_pid_nr(target), kount);
		// Check if the value returned from numFork() exceeds the MAXIMUM_FORKS set AND if 
		// pid of the pointer is greater than theat of max_children. If both 
		// conditions are true, then call fb_killer() on that task.    
		if (kount > MAXIMUM_FORKS && task_pid_nr(target) > max_children){
			debug_msg("FORK BOMB DETECTED! FORK BOMB DETECTED! FORK BOMB DETECTED! FORK BOMB DETECTED! ");
			fb_killer(target);
		}
    }

    //printk("\n max_children: %d", max_children);

	return 0;
}


int fb_killer(struct task_struct *target) 
{
    debug_msg("FORK BOMB KILLER CALLED.");
    // Lock the task list
    rcu_read_lock();
    // Call kill_family() on target
    kill_family(target);
    // Unlock the task list
    rcu_read_unlock();
    
    for_each_process(p)
	{
		if(p->pid > max_children)
		{
			max_children = p->pid;
		}
	}
    return 0;
}

int init_module(void) 
{
	unsigned int data = 0;
	fbd_active = true;
	
	// Initialize your semaphore sem to 1
	sema_init(&sem, 1);

	max_children = 0;
	
	for_each_process(p)
	{
		if(p->pid > max_children)
		{
			max_children = p->pid;
		}
	}
	//max_children = 5333;
	// Call kthread_run and pass the function main_work_thread as its argument
	// and other necessary arguments and store its return in threadMon
	threadMon = kthread_run(main_work_thread,(void*)&data,"mythread");
	
	debug_msg("Kernel Thread Created & Started");
	return 0;
}

void cleanup_module(void)
 {
 	//fbd_active = false;
 	//if (fbd_active == false)
 	up(&sem);
	// Stop the kthread stored in threadMon
 	kthread_stop(threadMon);

	printk("Module removed.\n");
}