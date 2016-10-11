William Christie

SID: 810915675

william.christie@colorado.edu	

##Using Linux Kernel Module Programming to Code a Character Device Driver

###Submission Contents.
####hello_modules folder
a. PA2_helloModule.c

    introductory character device to learn how to create, install and uninstall an LKM

b. Makefile
    
    simple Makefile to compile PA2_helloModule.c to module object
    
####modules folder
a. simple_char_driver.c
    
    LKM simple character device driver implementation

b. test_app.c
  
    simple user space testing application for reading and writing to a device file. 

c. Makefile
  
    simple Makefile to compile simple_char_driver.c to a module object. 

###To Run: (Linux)
  1. Create a folder named modules and place all 3 submission files into the folder. 
  2. Create the module object with the following command:

        make -C /lib/modules/$(uname -r)/build M=$PWD modules

  3. Compile the test application with the following command:
    
        gcc -o test_app test_app.c
  
  4. Create the device file with the following command:
    
        sudo mknod -m 777 /dev/simple_character_device c 247 0
        it may be necessary to change 247 to some other major number of the driver. 
  
  5. Install the module with the following command: 
    
        sudo insmod simple_character_device.ko
  
  6. Confirm module installation and device file installation  using the following commands: 
    
        lsmod 
        cat /proc/devices
  
  7. Run test_app using the following command:
    
        ./test_app
        If desired: check any kernel output messages using --dmesg or --sudo tail /var/log/syslog
  
  8. When finished uninstall module using the following command:

        sudo rmmod simple_character_device
