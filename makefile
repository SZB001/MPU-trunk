##################################################################################################
##################################################################################################

# The target platform information.
# You can change the MACRO 'PLATFORM' or use make : make PLATFORM=[IMX6SX, X86]
# IMX6SX X86
# PLATFORM = X86
export PLATFORM

# SUB DIRS
SUBDIRS := \
	./mid/gpio \
	./mid/uart \
	./mid/timer \
	./mid/md5 \
	./mid/ringbuffer   \
	./mid/ipc   \
	./mid/file  \
	./mid/dir  \
	./mid/shm  \
	./mid/log  \
	./mid/dev_rw \
	./mid/rds \
	./mid/msg \
	./mid/base64 \
	./mid/bcd \
	./mid/hash \
	./mid/i2c \
	./mid/audio \
	./mid/tz \
	./mid/pwdg \
	./mid/netlink \
    ./mid/aes \
	./mid/aes_e \
    ./mid/sha \
	./app/base/tcom \
	./app/base/shell \
	./app/base/scom \
	./app/base/cfg \
	./app/base/gps \
	./app/base/can \
	./app/base/nm \
	./app/base/at \
	./app/base/dev/upg \
	./app/base/dev/fault \
	./app/base/dev/status \
	./app/base/dev/time \
	./app/base/dev \
	./app/base/pm \
	./app/base/uds/client \
	./app/base/uds/server/foton \
	./app/base/uds/server \
	./app/base/uds/J1939 \
	./app/base/uds \
	./app/base/dsu \
	./app/base/sock \
	./app/base/fct \
    ./app/base/httpclient \
    ./app/base/zlib \
    ./app/base/minizip \
	./app/interface/assist \
	./app/interface/support \
    ./app/interface/fota \
    ./app/interface/geelyhu \
	./app/interface/gb32960 \
	./app/interface/foton \
	./app/interface/ftp \
	./app/interface/autopilot \
	./app/interface/hozon \
	./app/interface/hozon/PrvtProtocol \
	./app/interface/hozon/sockproxy \
	./app \
	./appl \
	./shell
	
# ROOT DIR
ROOTDIR := $(shell pwd)
export ROOTDIR

# Global includes
GLOBAL_INCLUDES := -I$(ROOTDIR)/include 
export GLOBAL_INCLUDES

# Global flags for compile
#GLOBAL_CCXXFLAGS :=
GLOBAL_CCXXFLAGS := -Wall
export GLOBAL_CCXXFLAGS

# Target path
TARGET_PATH := $(ROOTDIR)/target
export TARGET_PATH 
LIB_PATH := $(ROOTDIR)/lib
export LIB_PATH

SUPPORTCMDS := all objs clean cleanall rebuild 
export SUPPORTCMDS

.PHONY : SUPPORTCMDS

$(SUPPORTCMDS):
	for subdir in $(SUBDIRS); do \
		echo "Making " $$subdir; \
		(cd $$subdir && make $@); \
	done;

striped:
	$(STRIP) $(TARGET_PATH)/*.bin
