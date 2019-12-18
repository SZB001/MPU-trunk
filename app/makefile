##################################################################################################
##################################################################################################

include $(ROOTDIR)/make.include

TYPE := bin
TARGETNAME := tbox_app.bin

CCXXFLAGS = -DDEBUG -g

# app module 
OFLAGS = -L $(TARGET_PATH_R)  -lassist -lgb32960 -lprvtprot -lsockproxy -lasn -lFileUpload -lremoteControl -lremoteDiag -livi -lprotocol  -lftp -lhttp -lminizip -lzlib -lluajit -lremote_diag -lKSPSM -lxdsse -lwsrv -lfota -ludefcfg
# base module
OFLAGS +=  -ltcom -lshell -lscom -lcfg -lgps -lcan -lnm  -lat -lpm -ldev -lupg -lfault -lstatus -ldev_time -lsock -ldsu -luds -luds_client -luds_server -luds_vendor -lJ1939 -lfct  -lble -lbsa_client 
# mid module
OFLAGS += -lrds -ldev_rw -lhash -ltimer -llog -ldir -lfile -lshm -lmd5 -lrb -lipc -lmsg -lbase64 -lbcd -luart -lgpio -laudio -li2c -ltz -lpwdg -laes -laes_e -lsha -lnetlink  -lprotobuf



# 3rd party module
ifeq ($(PLATFORM), EC20)
OFLAGS += -ldsi_netctrl -ldsutils -lqcmap_client  -lqmi_cci -lqmiservices -lqmi_common_so -lqmi -lql_mgmt_client -lmcm
OFLAGS += -L $(LIB_PATH_R) -lql_common_api
OFLAGS += -L $(LIB_PATH_R) -lbsa -lbtsock -ltboxsock
OFLAGS += -L $(LIB_PATH_R) -lql_peripheral
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
