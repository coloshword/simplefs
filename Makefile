obj-m += s2fs.o 

all: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
install:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo insmod s2fs.ko
uninstall:
	sudo rmmod s2fs.ko
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean