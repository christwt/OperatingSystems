### William Christie

SID: 810915676

Email: william.christie@colorado.edu

### Submission Contents: ensure you start from /home/kernel/linux-4.4.0

a. arch/x86/kernel/Simple_add.c

    Implementation of system call 327. Uses KERN_EMERG macro and printk function to print a kernel message of the result of adding two integers. 

b. arch/x86/helloworld.c

    Implementation of system call 326. Uses KERN_EMERG macro and printk function to print the kernel message "Hello World."

c. arch/x86/kernel/Makefile

    Makefile which has been adjusted so that Simple_add.c and helloworld.c are compiled to .o files. 

d. arch/x86/entry/syscalls/syscall_64.tbl

    Table of Kernel system calls. Please note 326 (sys_helloworld) and 327 (sys_simple_add)

e. include/linux/syscalls.h

    Header file to contain all declarations of system calls. 

f. /var/log/syslog

    Use sudo tail /var/log/syslog to check printk messsages from system calls. Can also use dmesg.

g. Source Code for test programs.
  1. test_helloworld.c
  
        tests system call 326 (sys_helloworld). Use dmesg to check prink statement. 

  2. test_simple_add.c

        tests system call 327 (sys_simple_add). Use dmesg to check prink statement.
        
## To Run:
  1. Ensure all files in this submission have been placed in their respective file paths.
  2. Compile test programs with commands noted below. 
  
        gcc -o testhello test_helloworld.c
        gcc -o test_simple_add test_simple_add.c

  3. Run test programs with commands noted below.
  
        ./testhello
        ./test_simple_add

  4. Check printk logs using either of the commands listed below. 
  
        dmesg
        sudo tail /var/log/syslog
