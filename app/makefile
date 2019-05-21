##################################################################################################
##################################################################################################

include $(ROOTDIR)/make.include

TYPE := bin
TARGETNAME := tbox_app.bin

CCXXFLAGS = -DDEBUG -g

# app module 
OFLAGS = -L $(TARGET_PATH_R) -lgeelyhu -lfota -lassist -lfoton -lgb32960 -lprvtprot -lsockproxy -lasn -lautopilot -lprotocol  -lftp -lhttp -lminizip -lzlib -lluajit  
# base module
OFLAGS +=  -ltcom -lshell -lscom -lcfg -lgps -lcan -lnm  -lat -lpm -ldev -lupg -lfault -lstatus -ldev_time -lsock -ldsu -luds -luds_client -luds_server -luds_vendor -lJ1939 -lfct 
# mid module
OFLAGS += -lrds -ldev_rw -lhash -ltimer -llog -ldir -lfile -lshm -lmd5 -lrb -lipc -lmsg -lbase64 -lbcd -luart -lgpio -laudio -li2c -ltz -lpwdg -laes -laes_e -lsha -lnetlink



# 3rd party module
ifeq ($(PLATFORM), EC20)
OFLAGS += -ldsi_netctrl -ldsutils -lqcmap_client  -lqmi_cci -lqmiservices -lqmi_common_so -lqmi -lmcm
OFLAGS += -L $(LIB_PATH_R) -lql_common_api	#-ltboxsock -lssl -lcrypto
endif

# OS module
OFLAGS += -lpthread -lrt -lz -ldl -lm

#INCLUDES =
#CCXXFLAGS =
#CFLAGS =
#CXXFLAGS =
#OFLAGS =
#EXOBJS =
#EXCLUDESRCS =


include $(ROOTDIR)/make.share
