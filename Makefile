PASSPHRASE ?= "Put your passphrase here"

obj-m += avm.o
DEVICE_NUMBER ?= 510
DEVICE_NAME ?= avm
MODULE_NAME ?= avm.ko
TEST_APP ?= app

all:
	@$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	@gcc -Wno-implicit-function-declaration -o $(TEST_APP) $(TEST_APP).c
	@echo $(PASSPHRASE) | sudo -S insmod $(MODULE_NAME)
	
install:
	@sudo mknod /dev/$(DEVICE_NAME) c $(shell echo $(PASSPHRASE) | sudo -S dmesg | grep -w "avm device major number" | tail -1| awk '{ print $$9 }') 0
	@sudo chmod 777 /dev/$(DEVICE_NAME)
	@modinfo $(MODULE_NAME)
	
uninstall:
	@echo $(PASSPHRASE) | sudo -S rmmod $(MODULE_NAME)
	@sudo rm /dev/$(DEVICE_NAME)

view:
	@echo $(PASSPHRASE) | sudo -S dmesg | tail -n 20

clean:
	@$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	@rm -f $(TEST_APP)
	@sudo rmmod $(MODULE_NAME)
