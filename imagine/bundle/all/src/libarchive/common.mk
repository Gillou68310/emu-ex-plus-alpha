ifndef CHOST
 CHOST := $(shell $(CC) -dumpmachine)
endif

include $(buildSysPath)/imagineSDKPath.mk

libarchiveVer := 3.1.2-git
libarchiveSrcDir := $(tempDir)/libarchive-$(libarchiveVer)
libarchiveSrcArchive := libarchive-$(libarchiveVer).tar.xz

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/.libs/libarchive.a
installIncludeDir := $(installDir)/include

pkgCFlags := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config liblzma --cflags)
pkgLibs := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config liblzma --libs)

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing libarchive to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(libarchiveSrcDir)/libarchive/archive.h $(libarchiveSrcDir)/libarchive/archive_entry.h $(installIncludeDir)/
	cp $(buildDir)/build/pkgconfig/libarchive.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(libarchiveSrcDir)/configure : | $(libarchiveSrcArchive)
	@echo "Extracting libarchive..."
	@mkdir -p $(libarchiveSrcDir)
	tar -mxJf $| -C $(libarchiveSrcDir)/..
	cp ../gnuconfig/config.* $(libarchiveSrcDir)/build/autoconf/
	cd $(libarchiveSrcDir) && build/autogen.sh

$(outputLibFile) : $(makeFile)
	@echo "Building libarchive..."
	$(MAKE) -C $(<D) DEV_CFLAGS=

$(makeFile) : $(libarchiveSrcDir)/configure
	@echo "Configuring libarchive..."
	@mkdir -p $(@D)
	dir=`pwd` && \
	cd $(@D) && \
	$(toolchainEnvParams) \
	CFLAGS="$(CPPFLAGS) $(CFLAGS) $(pkgCFlags)" \
	LDFLAGS="$(LDFLAGS) $(LDLIBS) $(pkgLibs)" \
	$(libarchiveSrcDir)/configure \
	--prefix='$${pcfiledir}/../..' \
	--disable-shared \
	--disable-rpath \
	--disable-bsdtar \
	--disable-bsdcat \
	--disable-bsdcpio \
	--disable-xattr \
	--disable-acl \
	--without-bz2lib \
	--without-iconv \
	--without-lz4 \
	--without-lzmadec \
	--without-lzo2 \
	--without-nettle \
	--without-openssl \
	--without-xml2 \
	--without-expat \
	--host=$(CHOST) \
	PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) \
	PKG_CONFIG="pkg-config" \
	$(buildArg)
ifeq ($(ENV),android) # work around SDK 9 headers
	echo >> $(buildDir)/config.h
	echo \#"include <stdlib.h>" >> $(buildDir)/config.h
	echo "void arc4random_buf(void*, size_t);" >> $(buildDir)/config.h
	echo \#undef HAVE_FSTATFS >> $(buildDir)/config.h
endif
