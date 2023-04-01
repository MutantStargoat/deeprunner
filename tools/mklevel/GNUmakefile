src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(src:.c=.d)
bin = mklevel

warn = -pedantic -Wall
dbg = -g
inc = -I../../libs -I../../libs/goat3d/include -I../../libs/treestor/include
libdir = ../../libs/unix
libs = $(libdir)/goat3d.a $(libdir)/treestor.a

CC = gcc
CFLAGS = $(warn) $(dbg) -MMD
LDFLAGS = $(libs) -lm

$(bin): $(obj) $(libs)
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: cleandep
cleandep:
	rm -f $(dep)

$(libdir)/goat3d.a:
	$(MAKE) -C ../../libs/goat3d

$(libdir)/treestor.a:
	$(MAKE) -C ../../libs/treestor
