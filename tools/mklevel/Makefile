obj = src/main.o
bin = mklevel

dbg = -g
inc = -I../../libs -I../../libs/goat3d/include -I../../libs/treestor/include
libdir = ../../libs/unix
libs = $(libdir)/goat3d.a $(libdir)/treestor.a

CFLAGS = $(dbg) $(inc)
LDFLAGS = $(libs) -lm

$(bin): $(obj) $(libs)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(obj) $(bin)

$(libdir)/goat3d.a:
	cd ../../libs/goat3d; $(MAKE)

$(libdir)/treestor.a:
	cd ../../libs/treestor; $(MAKE)
