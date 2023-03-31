obj = src/aabox.o src/chunk.o src/dynarr.o src/extmesh.o src/g3danm.o \
	  src/g3dscn.o src/goat3d.o src/log.o src/read.o src/track.o src/write.o \
	  src/util.o
alib = ../unix/goat3d.a

CFLAGS = -O3 -g -Iinclude -I../treestor/include -I..

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(obj) $(alib)
