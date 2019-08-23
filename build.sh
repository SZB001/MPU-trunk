#!/bin/bash


if [[ $1 == "EC20" ]];
	then 
	echo "target tpye:EC20"
	OPTIONS="PLATFORM=EC20"
	export SDKTARGETSYSROOT=/opt/ql-oe/sysroots/armv7a-vfp-neon-oe-linux-gnueabi
	export PATH=/opt/ql-oe/sysroots/x86_64-oesdk-linux/usr/bin:/opt/ql-oe/sysroots/x86_64-oesdk-linux/usr/bin/arm-oe-linux-gnueabi:$PATH
	#export CCACHE_PATH=$sdkpathnative$bindir:$sdkpathnative$bindir/arm-oe-linux-gnueabi:$CCACHE_PATH
	export PKG_CONFIG_SYSROOT_DIR=$SDKTARGETSYSROOT
	export PKG_CONFIG_PATH=$SDKTARGETSYSROOT/usr/lib/pkgconfig
	export OECORE_NATIVE_SYSROOT="/opt/ql-oe/sysroots/x86_64-oesdk-linux"
	export OECORE_TARGET_SYSROOT="$SDKTARGETSYSROOT"
	export OECORE_ACLOCAL_OPTS="-I /opt/ql-oe/sysroots/x86_64-oesdk-linux/usr/share/aclocal"
	export PYTHONHOME=/opt/ql-oe/sysroots/x86_64-oesdk-linux/usr
	unset command_not_found_handle
	export CC="arm-oe-linux-gnueabi-gcc  -march=armv7-a -mfloat-abi=soft -mfpu=vfp -mtune=cortex-a9 --sysroot=$SDKTARGETSYSROOT -g"
	#export CC="arm-oe-linux-gnueabi-gcc  -march=armv7-a -mfloat-abi=soft -mfpu=vfp -mtune=cortex-a9 --sysroot=$SDKTARGETSYSROOT"
	export CXX="arm-oe-linux-gnueabi-g++  -march=armv7-a -mfloat-abi=soft -mfpu=vfp -mtune=cortex-a9 --sysroot=$SDKTARGETSYSROOT"
	export CPP="arm-oe-linux-gnueabi-gcc -E  -march=armv7-a -mfloat-abi=soft -mfpu=vfp -mtune=cortex-a9 --sysroot=$SDKTARGETSYSROOT"
	export AS="arm-oe-linux-gnueabi-as "
	export LD="arm-oe-linux-gnueabi-ld  --sysroot=$SDKTARGETSYSROOT"
	export GDB=arm-oe-linux-gnueabi-gdb
	export STRIP=arm-oe-linux-gnueabi-strip
	export RANLIB=arm-oe-linux-gnueabi-ranlib
	export OBJCOPY=arm-oe-linux-gnueabi-objcopy
	export OBJDUMP=arm-oe-linux-gnueabi-objdump
	export AR=arm-oe-linux-gnueabi-ar
	export NM=arm-oe-linux-gnueabi-nm
	export TARGET_PREFIX=arm-oe-linux-gnueabi-
	export CONFIGURE_FLAGS="--target=arm-oe-linux-gnueabi --host=arm-oe-linux-gnueabi --build=x86_64-oesdk-linux --with-libtool-sysroot=$SDKTARGETSYSROOT"
	export CFLAGS=" -O2 -pipe -g -feliminate-unused-debug-types -D_GNU_SOURCE"
	export CXXFLAGS=" -O2 -pipe -g -feliminate-unused-debug-types"
	export LDFLAGS="-Wl,-O2 -Wl,--hash-style=gnu -Wl,--as-needed"
	export CPPFLAGS=""
	export KCFLAGS="--sysroot=$SDKTARGETSYSROOT"
	export ARCH=arm
	export CROSS_COMPILE=arm-oe-linux-gnueabi-
	export GCC_COLORS='error=01;31:warning=01;35:note=01;36:caret=01;32:locus=01:quote=01'
	else
	echo "target tpye:X86"
    OPTIONS="PLATFORM=X86"
fi

make cleanall
make $OPTIONS
#make striped