obj-m = langford.o
KVERSION = $(shell uname -r)

all: langford.ko langford_util langford_adc_util

langford.ko: langford.c
	@echo -e 'Making module.'
	@export CCFLAGS="-ansi -Wall -std=c89"
	@sudo make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

install: langford.ko langford_util langford_adc_util
	@#Ensure module recompilation everytime it needs to be installed.	
	@#(Prevents module version errors after kernel upgrades.) 
	@echo -e 'Installing Langford driver.'
	@echo -e '\t Cleaning up previous binaries...'
	@-sudo rm -f langford.ko
	@-sudo rm -rf /dev/langford
	@echo -e '\t Attempting to remove kernel module...'
	@-sudo modprobe -r langford
	@echo -e '\t Building langford module.'
	make langford.ko
	@sudo cp langford.ko /lib/modules/`uname -r`/kernel/drivers/misc
	@sudo depmod -a
	@sudo modprobe langford
	@#Allow some time for driver to finish loading. If driver does not complete loading, no character device will be registered and mknod will fail.
	@echo -e '\t Copying utility files to /usr/bin'
	@sleep 3
	@sync
	@sudo cp langford_util /usr/bin
	@sudo cp langford_adc_util /usr/bin
	@sudo cp langford_init /usr/bin
	@sudo chmod a+rx /usr/bin/langford_util
	@echo -e 'Installing libraries and additional utilities.'
	make -C libs/ install
	@#Create langford group, if not already.
	@echo -e '\t Attempting to create langford group'
	@-sudo groupadd langford
	@echo -e 'Completed installation routine.'
	@echo -e '******\n***Remember to run langford_init in order to initialize the device at /dev/langford \n*****'

langford_util: langford_util.cpp
	echo -e 'Making langford_util.'
	g++ $< -ansi -Wall -g -o $@

langford_adc_util: langford_adc_util.cpp langford_spi.cpp
	@echo -e 'Making langford_adc_util'
	g++ langford_adc_util.cpp langford_spi.cpp -ansi -Wall -g -o $@

libs:
	@echo -e 'Making libraries.'
	make -f libs/Makefile

clean:
	@echo -e 'Cleaning up existing driver files.'
	-sudo make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
	@-sudo rm -rf langford.ko.gz langford_util langford_adc_util
	@echo -e 'Cleaning up library files.'
	make -C libs/ clean

distclean: clean
	@echo -e 'Making really clean...'
	@-sudo make -C ./ clean
	@-sudo rm -rf .tmp*
	@-sudo rm -rf *~
	@#-sudo rm modules.order Module.symvers

uninstall:
	@echo -e 'Uninstalling langford utilities from /usr/bin'
	@-sudo rm /usr/bin/langford_util
	@-sudo rm /usr/bin/langford_init
	@-sudo rm /usr/bin/langford_adc_util
	@# Remove langford group
	@echo -e 'Removing langford group.'
	@-sudo groupdel langford
	@echo -e 'Uninstalling libraries.'
	make -C libs/ uninstall
