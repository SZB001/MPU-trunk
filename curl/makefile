##################################################################################################
##################################################################################################

include $(ROOTDIR)/make.include

TYPE := bin
TARGETNAME := curl.bin


CCXXFLAGS = -DDEBUG -g
OFLAGS = -L $(TARGET_PATH_R) -llog -lfile -ldir -lshm -lmd5 -ldev_rw -lpthread -lrt
OFLAGS += -L $(LIB_PATH_R)  -lz -lcurl -lssl -lcrypto -ldl


#INCLUDES =
#CCXXFLAGS =
#CFLAGS =
#CXXFLAGS =
#OFLAGS =
#EXOBJS =
#EXCLUDESRCS =


include $(ROOTDIR)/make.share
