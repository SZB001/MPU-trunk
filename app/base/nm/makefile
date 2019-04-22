##################################################################################################
##################################################################################################

include $(ROOTDIR)/make.include

TYPE := staticlib
TARGETNAME := nm

ifeq ($(PLATFORM), EC20)
INCLUDES = -I $(SDKTARGETSYSROOT) \
           -I $(SDKTARGETSYSROOT)/usr/include \
           -I $(SDKTARGETSYSROOT)/usr/include/data \
           -I $(SDKTARGETSYSROOT)/usr/include/qmi \
           -I $(SDKTARGETSYSROOT)/usr/include/dsutils \
           -I $(SDKTARGETSYSROOT)/usr/include/qmi \
           -I $(SDKTARGETSYSROOT)/usr/include/qmi-framework \
           -I $(SDKTARGETSYSROOT)/usr/include/qmi-client-helper 
endif		   

#CCXXFLAGS =
#CFLAGS =
#CXXFLAGS =
#OFLAGS =
#EXOBJS =
#EXCLUDESRCS =

include $(ROOTDIR)/make.share
