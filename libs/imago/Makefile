mobj = src/byteord.o src/filelbm.o src/filergbe.o src/imago2.o src/conv.o \
	   src/filepng.o src/filetga.o src/imago_gl.o src/filejpeg.o src/fileppm.o \
	   src/ftmodule.o src/modules.o
zobj = zlib/adler32.o zlib/compress.o zlib/crc32.o zlib/deflate.o zlib/gzio.o \
	   zlib/infback.o zlib/inffast.o zlib/inflate.o zlib/inftrees.o zlib/trees.o \
	   zlib/uncompr.o zlib/zutil.o
pobj = libpng/pngerror.o libpng/pngget.o libpng/pngmem.o libpng/pngpread.o \
	   libpng/pngread.o libpng/pngrio.o libpng/pngrtran.o libpng/pngrutil.o \
	   libpng/pngset.o libpng/pngtrans.o libpng/pngwio.o libpng/pngwrite.o \
	   libpng/pngwtran.o libpng/pngwutil.o libpng/png.o
jobj = jpeglib/jcapimin.o jpeglib/jcapistd.o jpeglib/jccoefct.o jpeglib/jccolor.o \
	   jpeglib/jcdctmgr.o jpeglib/jchuff.o jpeglib/jcinit.o jpeglib/jcmainct.o \
	   jpeglib/jcmarker.o jpeglib/jcmaster.o jpeglib/jcomapi.o jpeglib/jcparam.o \
	   jpeglib/jcphuff.o jpeglib/jcprepct.o jpeglib/jcsample.o jpeglib/jctrans.o \
	   jpeglib/jdapimin.o jpeglib/jdapistd.o jpeglib/jdatadst.o jpeglib/jdatasrc.o \
	   jpeglib/jdcoefct.o jpeglib/jdcolor.o jpeglib/jddctmgr.o jpeglib/jdhuff.o \
	   jpeglib/jdinput.o jpeglib/jdmainct.o jpeglib/jdmarker.o jpeglib/jdmaster.o \
	   jpeglib/jdmerge.o jpeglib/jdphuff.o jpeglib/jdpostct.o jpeglib/jdsample.o \
	   jpeglib/jdtrans.o jpeglib/jerror.o jpeglib/jfdctflt.o jpeglib/jfdctfst.o \
	   jpeglib/jfdctint.o jpeglib/jidctflt.o jpeglib/jidctfst.o jpeglib/jidctint.o \
	   jpeglib/jidctred.o jpeglib/jmemmgr.o jpeglib/jmemnobs.o jpeglib/jquant1.o \
	   jpeglib/jquant2.o jpeglib/jutils.o
obj = $(mobj) $(zobj) $(pobj) $(jobj)
alib = ../unix/imago.a

CFLAGS = -O3 -Izlib -Ilibpng -Ijpeglib $(pic)

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(obj) $(alib)
