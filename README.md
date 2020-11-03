# Overview

In this assignment, we will write a simple character driver called booga. Please do a git pull --rebase in your git repo to get the booga project folder that contains the starter code and scripts for this project. Note that, you will only be able to test this assignment on a Linux machine where you have root privilege. A VMware-based CentOS 7 (64 bit) VM image is created on each desktop machine in the classroom - follow the in-class instructions to setup the VM. Later on we will refer to this VM as the cs453 VM. (username/password: cs453/cs453, run commands with sudo to gain root privilege).

## Important notes

You MUST build against the kernel version installed on the cs453 VM - but you can copy this same VM image into your own laptop, if you know how to run VMware-based virtual machines on your laptop. If you are unclear what kernel version to build against please ask do NOT just assume. The only time you will need to use root in this project is to load and unload the drivers. You should not use the root account for building, test, etc.

# Specification

The main driver
The booga driver is a simple character driver that supports the open, read and write and close operations. The driver supports four minor numbers: 0, 1, 2, and 3. The device files are: /dev/booga0, /dev/booga1, /dev/booga2, /dev/booga3. We will also create a link from /dev/booga to /dev/booga0, so that acts as the default device when someone accesses /dev/booga. On reading from /dev/booga0, /dev/booga1, /dev/booga2 and /dev/booga3 the driver gives a stream of one of the following phrases:

- booga! booga!
- googoo! gaagaa!
- neka! maka!
- wooga! wooga!


Note that the driver may only output part of a phrase if the number of bytes requested by the user is not a multiple of the length of the chosen phrase.  Also each new phrase is separated from the previous one by a single space.

The driver is unpredictable as to what phrase it says. In order to simulate this behavior, we will use a random number generator. However, we cannot call any standard C library functions in the kernel (e.g. random()).  Linux actually has a device driver that generates strong random numbers by observing hardware noise, interrupts and other system activity. In the user space, the random device driver can be accessed via /dev/random. In your device driver code, you will need to include the header file <linux/random.h> and the function that you will call has the following prototype.

```c
extern void get_random_bytes(void *buf, int nbytes);
```

Here is a sample code on how to use the above function.

```c
char randval;
get_random_bytes(&randval, 1);
choice = (randval & 0x7F) % 4; /* bitwise AND is to guarantee positive numbers */
```

On writing to booga devices /dev/booga0, /dev/booga1, /dev/booga2, the booga device driver works like  /dev/null (so it pretends to write the buffer but doesn't actually write to any device). However if a process tries to write to /dev/booga3, it suffers from sudden death! You cannot call the system call kill() from inside the kernel space so you will have to figure out how to terminate a process from inside the kernel. Search the function kill_pid() in the kernel sources for ideas. Use the SIGTERM signal instead of SIGKILL for terminating the process.
Get booga stats from /proc
Create /proc entries for the booga driver. It reports on the number of bytes read/written since the driver was loaded, number of times it was opened from each supported minor number, and the number of times each of the four strings was selected. So if we output 1000 characters with the string “booga! booga!” in it, then we count that as one more instance of the phrase for this purpose. If the driver outputs an incomplete phrase, we will still count it as another instance of that phrase being selected. Here is a sample output.

```console
[user@localhost booga]# cat /proc/driver/booga
bytes read = 300
bytes written = 200
number of opens:
  /dev/booga0 = 4 times
  /dev/booga1 = 0 times
  /dev/booga2 = 1 times
  /dev/booga3 = 1 times
strings output:
  booga! booga!  = 0 times
  googoo! gaagaa!  = 1 times
  neka! maka!  = 1 times
  wooga! wooga!    = 1 times
```

## Thread-safety
Protect the updating of structure used to track the statistics using semaphores. Refer to example driver code on the course page for an example on how to use semaphores in Linux kernel.

## Notes

Here are the prototypes of the functions that your driver would need to implement - in booga.c and booga.h.

```c
static int booga_open (struct inode *inode, struct file *filp);
static int booga_release (struct inode *inode, struct file *filp);
static ssize_t booga_read (struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t booga_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos);
static int __init booga_init(void);
static void __exit booga_exit(void);
```

Remember that you need to use the __copy_to_user(...) kernel function to copy the data to user space. 

Note that the kernel print messages will not show on the screen. The messages are, however, logged in the file /var/log/messages. You can open another terminal and watch the output to the system messages file with the command:

```console
sudo tail -f /var/log/messages
```

In some versions, you may not have this file setup. Alternatively,  you can use the command  sudo dmesg --follow to watch for kernel log messages.

We have provided a test program called test-booga.c. We will use this program to test the driver. While testing your program, the following situations may arise:

The system hangs and you have to hit the reset button
The system spontaneously reboots. Oops!
The kernel prints an oops message (a milder version of the infamous General Protection Fault). You will get a stack trace with it that should help in narrowing down the cause

Once you run make, your compiler will compile test-booga.c into a binary called test-booga. Two scripts are provided for testing - they will call test-booga. Run booga-test1.sh to test regular functionalities of your driver, and run booga-test2.sh to test if your driver is thread-safe or not.

Here is a sample session with the booga driver. Note that the characters returned by the driver do not contain any newline characters. The output shown below was reformatted slightly for this document.

```console
[user@localhost]$ make
make -C /lib/modules/`uname -r`/build M=`pwd` modules
make[1]: Entering directory '/usr/src/kernels/4.14.11-200.fc26.x86_64'
 CC [M]  /home/user/Documents/classes/cs453/github/CS453/projects/p5/grader/solutions/user/booga.o
 Building modules, stage 2.
 MODPOST 1 modules
 CC      /home/user/Documents/classes/cs453/github/CS453/projects/p5/grader/solutions/user/booga.mod.o
 LD [M]  /home/user/Documents/classes/cs453/github/CS453/projects/p5/grader/solutions/user/booga.ko
make[1]: Leaving directory '/usr/src/kernels/4.14.11-200.fc26.x86_64'
cc    -c -o test-booga.o test-booga.c
cc  -o test-booga test-booga.o

[user@localhost]$ ls
booga.c   booga_load   booga.o         booga_unload   Module.symvers  test-booga.c
booga.h   booga.mod.c  booga-test1.sh  Makefile       README.md       test-booga.o
booga.ko  booga.mod.o  booga-test2.sh  modules.order  test-booga
[user@localhost ]$

[user@localhost]$ sudo ./booga_load       
[sudo] password for user:  

[user@localhost]$ /sbin/lsmod  
Module                  Size  Used by
booga                  16384  0
bluetooth             593920  0
ecdh_generic           24576  1 bluetooth
rfkill                 28672  2 bluetooth
ipt_MASQUERADE         16384  1
nf_nat_masquerade_ipv4    16384  1 ipt_MASQUERADE
. . .

[user@localhost]$ ./test-booga 0 100 read
Read returned 100 characters
wooga! wooga! wooga! wooga! wooga! wooga! wooga! wooga! wooga! wooga! wooga! wooga! wooga! wooga! Wo

[user@localhost]$ ./test-booga 0 100 read
Read returned 100 characters
googoo! gaga! googoo! gaga! googoo! gaga! googoo! gaga! googoo! gaga! googoo! gaga! googoo! gaga! go

[user@localhost]$ ./test-booga 0 100 write
Attempting to write to booga device
Wrote 100 bytes.

[user@localhost]$ ./test-booga 3 100 write  
Attempting to write to booga device
Terminated

[user@localhost]$ cat /proc/driver/booga
bytes read = 200  
bytes written = 100  
number of opens:
 /dev/simple0 = 3 times
 /dev/simple1 = 0 times
 /dev/simple2 = 0 times
 /dev/simple3 = 1 times
strings output:
 booga! booga!  = 0 times
 googoo! gaga!  = 1 times
 wooga! wooga!  = 1 times
 neka! maka!    = 0 times

[user@localhost]$ sudo ./booga_unload 
```

## Submission

Create a private repository on github and give access permission to this account: jidongbsu (or using the email address: jidongxiao@boisestate.edu). You can use one repository for all 3 projects in this course, so that you only need to grant access permission to Jidong once.

Due Date:  November 19th, 2020.

## Grading Rubric

- [10 pts] Compiling
  - Each compiler warning will result in a 3 point deduction.
  - You are not allowed to suppress warnings

- [70 pts] Main driver:
  - Supports read properly:    30/30
  - Use of /dev/random:             10/10
  - Writing acts like /dev/null:    10/10
  - Kill process writing to booga3: 10/10
  - Proper use of semaphores:       10/10

- [10 pts] Get Booga Stats from /proc
  - Report on number bytes written/read: 5/5
  - Report on strings :                  5/5

- [10 pts] Documentation:
  - Plan for testing:

