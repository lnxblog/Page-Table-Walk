#include <linux/fs.h>
#include <linux/io.h>
#include <linux/memblock.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/highmem.h>

#define WRITE_SIZE 32

static struct miscdevice misc;


static int hemanth_fop_open(struct inode *inode, struct file *filp)
{
    printk(KERN_EMERG "test driver opened\n");
    return 0;
}


static int hemanth_fop_release(struct inode *inode, struct file *filp)
{
    printk(KERN_EMERG "test driver closed / released\n");
    return 0;
}

static ssize_t hemanth_fop_read(
	struct file *filp, char __user *data, size_t size, loff_t *offset)
{
	printk(KERN_EMERG "test driver read\n");
	return 0;
}

static ssize_t hemanth_fop_write(
	struct file *filp, char __user *data, size_t size, loff_t *offset)
{
	int bytes_not_write = 0,pid,i,j,begin;
	struct pid *pid_struct;
	struct task_struct *task;
	struct mm_struct *task_mm;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	struct page *page=NULL;
	unsigned long offset_within_page,virt_addr;
	char temp_ptr[WRITE_SIZE],dataset[2][10];
	int *ptr;

	bytes_not_write = copy_from_user(temp_ptr,data,WRITE_SIZE);
	printk(KERN_EMERG "user writes : %s\n",temp_ptr);
	
	begin=i=j=0;
	while(i<strlen(temp_ptr))
	{
		if(temp_ptr[i]==' ')
		{
			strncpy(dataset[j++],&temp_ptr[begin],i-begin);
			dataset[j-1][i-begin]='\0';
			begin=i+1;
		}
		else if(temp_ptr[i]=='\0')break;
		i++;
	}

	printk(KERN_EMERG "user 1st write %s",dataset[0]);
	printk(KERN_EMERG "user 2nd write %s",dataset[1]);		

	kstrtouint(dataset[0],10,&pid);
	pid_struct=find_get_pid(pid);
	task=pid_task(pid_struct,PIDTYPE_PID);
	printk(KERN_EMERG "task_struct pid : %d\n",task->pid);
	
	kstrtoul(dataset[1],16,&virt_addr);
	printk(KERN_EMERG "virt addr :%ld %lx\n",virt_addr,virt_addr);

	task_mm=task->mm;
	// acquire page table lock
	spin_lock(&(task_mm->page_table_lock));

	pgd=pgd_offset(task_mm,virt_addr);
	if(pgd_none(*pgd))printk(KERN_EMERG "No pgd");
	printk(KERN_EMERG "pgd pointer: %p pgd entry:%p\n",task_mm->pgd,pgd);

	pud=pud_offset(pgd,virt_addr);
	if(pud_none(*pud))printk(KERN_EMERG "No pud");
	printk(KERN_EMERG "pud pointer: %p \n",pud);

	pmd=pmd_offset(pud,virt_addr);
	if(pmd_none(*pmd))printk(KERN_EMERG "No pmd");

	pte=pte_offset_map(pmd,virt_addr);
	if(pte_present(*pte))
	{
		page=pte_page(*pte);
		offset_within_page=(virt_addr)&(PAGE_SIZE-1);
		printk(KERN_EMERG "page address %lx physical address %lx",page_to_phys(page),page_to_phys(page)+offset_within_page);

		/* Gain access to the contents of that page. */
		void *vaddr = kmap_atomic(page);
		/* Do something to the contents of that page. */
		ptr=vaddr+offset_within_page;
		printk(KERN_EMERG "value at address : %d",*ptr);
		/* Unmap that page. */
		kunmap_atomic(vaddr);
	}	
	else
		printk(KERN_EMERG "No pte present");
	pte_unmap(pte);
	//Release spin lock
	spin_unlock(&(task_mm->page_table_lock));
	return size-bytes_not_write;
}

static const struct file_operations hemanth_fops = {
	.owner = THIS_MODULE,
	.open = hemanth_fop_open,
	.release = hemanth_fop_release,
	.read = hemanth_fop_read,
	.write = hemanth_fop_write,
};

int __init initfunction(void)
{
	int ret;
	

	printk(KERN_EMERG "test driver"); 
	misc.minor = MISC_DYNAMIC_MINOR;
	misc.name = "test driver";
	misc.fops = &hemanth_fops;

	ret = misc_register(&misc);

	if (ret) {
		printk(KERN_EMERG "test driver: failed to register device %s\n",
			misc.name);
		return -1;
	}
	return 0;
}
int __exit exitfunction(void)
{
	misc_deregister(&misc);
	printk(KERN_ALERT "test driver exit");

}

module_init(initfunction);
module_exit(exitfunction);
