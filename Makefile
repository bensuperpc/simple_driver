CONFIG_MODULE_SIG=n

TARGET_MODULE := simple_linux_driver
KVERSION := $(shell uname -r)
KBUILD_CFLAGS_MODULE += -Wall -Wextra -Wno-unused-parameter -Wstrict-prototypes -Wmissing-prototypes -O2 -DDEBUG

# If we running by kernel building system
ifneq ($(KERNELRELEASE),)
	$(TARGET_MODULE)-objs := main.o
	obj-m := $(TARGET_MODULE).o
else
	BUILDSYSTEM_DIR:=/lib/modules/$(KVERSION)/build
	PWD:=$(shell pwd)

all: 
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) clean

load:
	insmod ./$(TARGET_MODULE).ko

unload:
	rmmod -f ./$(TARGET_MODULE).ko

endif

