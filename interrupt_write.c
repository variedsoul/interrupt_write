#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <xen/events.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>


#define MY_FILE "/root/irq_message"

MODULE_LICENSE("GPL");
int id=100;
int g_irq=0;


char buf[128];
struct file *file = NULL;

struct task_struct *my_thread;
int my_thread_main(void* data)
{
        int ret;
    
        while(!kthread_should_stop()){

                mm_segment_t old_fs;
                printk("Hello, I'm the module that intends to write messages to file.\n");


                if(file == NULL)
                        file = filp_open(MY_FILE, O_RDWR | O_APPEND | O_CREAT, 0644);
                if (IS_ERR(file)) {
                        printk("error occured while opening file %s, exiting...\n", MY_FILE);
                        return 0;
                }

                sprintf(buf,"%s", "The Messages.");

                old_fs = get_fs();
                set_fs(KERNEL_DS);
                file->f_op->write(file, (char *)buf, sizeof(buf), &file->f_pos);
                set_fs(old_fs);

                set_current_state(TASK_INTERRUPTIBLE);
                schedule();
        }
        printk("thread stop ok\n");
        return 0;
}

irqreturn_t dosomething(int irq,void* dev_id)
{
	wake_up_process(my_thread);

	return IRQ_HANDLED;
}
int hello_init(void)
{
	int irq=bind_virq_to_irqhandler(8,0,dosomething,IRQF_SHARED,"hello",&id);
	if(irq<0)
	{
		printk("bind error\n");
	}
	g_irq=irq;

	my_thread=kthread_create_on_node(my_thread_main,NULL,0,"my_thread");
    if(my_thread==NULL)
    {
        printk("new thread error\n");
        return -1;
    }  

	return 0;
}

void hello_exit(void)
{
	if(g_irq)
		unbind_from_irqhandler(g_irq,&id);

	if(my_thread)
        kthread_stop(my_thread);
       
	if(file != NULL)
        filp_close(file, NULL);


}

module_init(hello_init);
module_exit(hello_exit);