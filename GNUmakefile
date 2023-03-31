src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(src:.c=.d)
bin = game

warn = -pedantic -Wall
dbg = -g
opt = -O3
def = -DMINIGLUT_USE_LIBC
inc = -Ilibs -Ilibs/imago -Ilibs/treestor/include -Ilibs/goat3d/include
libs = libs/unix/imago.a libs/unix/goat3d.a libs/unix/treestor.a

CFLAGS = $(warn) $(dbg) $(opt) $(inc) $(def)
LDFLAGS = $(libs) -lGL -lGLU -lX11 -lm

$(bin): $(obj) libs
	$(CC) -o $@ $(obj) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: libs
libs:
	$(MAKE) -C libs

.PHONY: clean-libs
clean-libs:
	$(MAKE) -C libs clean
