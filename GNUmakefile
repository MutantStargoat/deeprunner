src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(src:.c=.d)
bin = game

warn = -pedantic -Wall
dbg = -g
opt = -O3
def = -DMINIGLUT_USE_LIBC -DMIKMOD_STATIC
inc = -Ilibs -Ilibs/imago/src -Ilibs/treestor/include -Ilibs/goat3d/include \
	  -Ilibs/mikmod/include
libs = libs/unix/imago.a libs/unix/goat3d.a libs/unix/treestor.a \
	   libs/unix/mikmod.a

CFLAGS = $(warn) $(dbg) $(opt) $(inc) $(def) -MMD
LDFLAGS = $(libs) -lGL -lGLU -lX11 -lasound -lm

$(bin): $(obj) libs
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: cleandep
cleandep:
	rm -f $(dep)

.PHONY: libs
libs:
	$(MAKE) -C libs

.PHONY: clean-libs
clean-libs:
	$(MAKE) -C libs clean

.PHONY: data
data:
	tools/procdata
