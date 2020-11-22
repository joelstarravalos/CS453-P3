/*
 * booga.c -- the bare booga char module
 * This booga shows how to use a semaphore to avoid race conditions
 * in updating a global data structure inside a driver.
 */
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/version.h> /* printk() */
#include <linux/init.h>  /*  for module_init and module_cleanup */
#include <linux/slab.h>  /*  for kmalloc/kfree */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/proc_fs.h>	/* for the proc filesystem */
#include <linux/random.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/sched/signal.h>
#include "booga.h"        /* local definitions */

static int booga_major =   BOOGA_MAJOR;
static int booga_nr_devs = BOOGA_NR_DEVS;    /* number of bare booga devices */
static int booga_minor;
module_param(booga_major, int, 0);
module_param(booga_nr_devs, int, 0);
MODULE_AUTHOR("Amit Jain");
MODULE_LICENSE("GPL v2");

static booga_stats *booga_device_stats;
static struct proc_dir_entry* booga_proc_file;

static ssize_t booga_read (struct file *, char *, size_t , loff_t *);
static ssize_t booga_write (struct file *, const char *, size_t , loff_t *);
static int booga_open (struct inode *, struct file *);
static int booga_release (struct inode *, struct file *);
static int booga_proc_open(struct inode *inode, struct file *file);


/*  The different file operations */
/* The syntax you see below is an extension to gcc. The prefered */
/* way to init structures is to use C99 Taged syntax */
/* static struct file_operations booga_fops = { */
/* 		    .read    =       booga_read, */
/* 			.write   =       booga_write, */
/* 			.open    =       booga_open, */
/* 			.release =       booga_release */
/* }; */
/*  This is where we define the standard read,write,open and release function */
/*  pointers that provide the drivers main functionality. */
static struct file_operations booga_fops = {
		    read:       booga_read,
			write:      booga_write,
			open:       booga_open,
			release:    booga_release,
};

/* The file operations struct holds pointers to functions that are defined by */
/* driver impl. to perform operations on the device. What operations are needed */
/* and what they should do is dependent on the purpose of the driver. */
static const struct file_operations booga_proc_fops = {
		.owner	= THIS_MODULE,
		.open	= booga_proc_open,
		.read	= seq_read,
		.llseek	= seq_lseek,
		.release	= single_release,
};


/*
 * Open and close
 */
static int booga_open (struct inode *inode, struct file *filp)
{
		int num = NUM(inode->i_rdev);
		
		if (num >= booga_nr_devs) return -ENODEV;
		booga_minor = num;
		filp->f_op = &booga_fops;

		/* need to protect this with a semaphore if multiple processes
		   will invoke this driver to prevent a race condition */
		if (down_interruptible (&booga_device_stats->sem))
				return (-ERESTARTSYS);
		if (num == 0) booga_device_stats->num_open_0++;
		else if (num == 1) booga_device_stats->num_open_1++;
		else if (num == 2) booga_device_stats->num_open_2++;
		else if (num == 3) booga_device_stats->num_open_3++;
		up(&booga_device_stats->sem);

		try_module_get(THIS_MODULE);
		return 0;          /* success */
}

static int booga_release (struct inode *inode, struct file *filp)
{

		module_put(THIS_MODULE);
		return 0;
}

/*
 * Data management: read and write
 */

static ssize_t booga_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
		char randval;
		int choice;
		char* string;
		char* p = buf;
		int i = 0;
		printk("<1>booga_read invoked.\n");
		
		/* need to protect this with a semaphore if multiple processes
		   will invoke this driver to prevent a race condition */
		
		get_random_bytes(&randval, 1);
		choice = (randval & 0x7F) % 4;
		
		
		if (down_interruptible (&booga_device_stats->sem))
				return (-ERESTARTSYS);
		if (choice == 0) {
			string = "booga! booga! ";
			booga_device_stats->boogas_printed++;
		}
		else if (choice == 1) {
			string = "googoo! gaagaa! ";
			booga_device_stats->googoos_printed++;
		}
		else if (choice == 2) {
			string = "neka! maka! ";
			booga_device_stats->nekas_printed++;
		}
		else if (choice == 3) {
			string = "wooga! wooga! ";
			booga_device_stats->woogas_printed++;
		}
		
		for (i = 0; i <= count; i += strlen(string)) {
			if (i <	count - strlen(string)){		
				strncpy(p, string, strlen(string));
				//strcat(buf,string);
				p += strlen(string);
			} else {
				strncpy(p, string, count - i);
			}
		}
		booga_device_stats->bytes_read+=count;
		up(&booga_device_stats->sem);
		return count;
}

static ssize_t booga_write (struct file *filp, const char *buf, size_t count , loff_t *f_pos)
{
		struct pid* pid;
		printk("<1>booga_write invoked.\n");
		printk("<1>THIS MODULE : %s\n", THIS_MODULE->name);
		if (booga_minor == 3) {
			pid = get_task_pid(current, PIDTYPE_PID);
			kill_pid(pid, SIGTERM, 1);
			return 0;
		}	
		/* need to protect this with a semaphore if multiple processes
		   will invoke this driver to prevent a race condition */
		if (down_interruptible (&booga_device_stats->sem))
				return (-ERESTARTSYS);
		booga_device_stats->bytes_write += count;
		up(&booga_device_stats->sem);
		return count; // pretend that count bytes were written
}

static void init_booga_device_stats(void)
{
		booga_device_stats->bytes_read=0;
		booga_device_stats->bytes_write=0;
		booga_device_stats->num_open_0=0;
		booga_device_stats->num_open_1=0;
		booga_device_stats->num_open_2=0;
		booga_device_stats->num_open_3=0;
		booga_device_stats->boogas_printed=0;
		booga_device_stats->nekas_printed=0;
		booga_device_stats->googoos_printed=0;
		booga_device_stats->woogas_printed=0;
		sema_init(&booga_device_stats->sem, 1);
}

static int booga_proc_show(struct seq_file *m, void *v)
{
		seq_printf(m, "bytes read = %ld\n", booga_device_stats->bytes_read);
		seq_printf(m, "bytes written = %ld\n", booga_device_stats->bytes_write);
		seq_printf(m, "number of opens:\n");
		seq_printf(m, "\t/dev/booga0 = %ld times\n", booga_device_stats->num_open_0);
		seq_printf(m, "\t/dev/booga1 = %ld times\n", booga_device_stats->num_open_1);
		seq_printf(m, "\t/dev/booga2 = %ld times\n", booga_device_stats->num_open_2);
		seq_printf(m, "\t/dev/booga3 = %ld times\n", booga_device_stats->num_open_3);
		seq_printf(m, "strings output:\n");
		seq_printf(m, "\tbooga! booga! = %ld times\n", booga_device_stats->boogas_printed);
		seq_printf(m, "\tgoogoo! gaagaa! = %ld times\n", booga_device_stats->googoos_printed);
		seq_printf(m, "\tneka! maka! = %ld times\n", booga_device_stats->nekas_printed);
		seq_printf(m, "\twooga! wooga! = %ld times\n", booga_device_stats->woogas_printed);
		return 0;
}


static int booga_proc_open(struct inode *inode, struct file *file)
{
		return single_open(file, booga_proc_show, NULL);
}


static __init int booga_init(void)
{
		int result;

		/*
		 * Register your major, and accept a dynamic number
		 */
		result = register_chrdev(booga_major, "booga", &booga_fops);
		if (result < 0) {
				printk(KERN_WARNING "booga: can't get major %d\n",booga_major);
				return result;
		}
		if (booga_major == 0) booga_major = result; /* dynamic */
		printk("<1> booga device driver version 4: loaded at major number %d\n", booga_major);

		booga_device_stats = (booga_stats *) kmalloc(sizeof(booga_stats),GFP_KERNEL);
		if (!booga_device_stats) {
				result = -ENOMEM;
				goto fail_malloc;
		}
		init_booga_device_stats();

		/* We assume that the /proc/driver exists. Otherwise we need to use proc_mkdir to
		 * create it as follows: proc_mkdir("driver", NULL);
		 */
		booga_proc_file = proc_create("driver/booga", 0, NULL, &booga_proc_fops);
		if (!booga_proc_file)  {
				result = -ENOMEM;
				goto fail_malloc;
		}

		return 0;

fail_malloc:
		unregister_chrdev(booga_major, "booga");
		return  result;
}



static __exit  void booga_cleanup(void)
{
		remove_proc_entry("driver/booga", NULL /* parent dir */);
		kfree(booga_device_stats);
		unregister_chrdev(booga_major, "booga");
		printk("<1> booga device driver version 4: unloaded\n");
}


module_init(booga_init);
module_exit(booga_cleanup);

/* vim: set ts=4: */
