autoreconf -f -i -Wall,no-obsolete
# TODO remove "-I../../kext/common"
CFLAGS="-DUNNAMED_SEMAPHORES_NOT_SUPPORTED -D_POSIX_C_SOURCE=200112L -I../../kext/common/ -O -gdwarf-2 -arch i386 -arch x86_64 -isysroot /Developer/SDKs/MacOSX10.6.sdk -mmacosx-version-min=10.6" LDFLAGS="-arch i386 -arch x86_64 -framework CoreFoundation" ./configure --prefix=/opt/local/ --disable-dependency-tracking --disable-static
