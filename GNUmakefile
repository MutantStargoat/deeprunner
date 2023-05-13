src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(src:.c=.d)
bin = game

warn = -pedantic -Wall
dbg = -g
#opt = -O3
def = -DMINIGLUT_USE_LIBC -DMIKMOD_STATIC
inc = -Ilibs -Ilibs/imago/src -Ilibs/treestor/include -Ilibs/goat3d/include \
	  -Ilibs/drawtext -Ilibs/mikmod/include
libs = libs/unix/imago.a libs/unix/goat3d.a libs/unix/treestor.a \
	   libs/unix/drawtext.a libs/unix/psys.a libs/unix/mikmod.a

CFLAGS = $(warn) $(dbg) $(opt) $(inc) $(def) -MMD
LDFLAGS = $(ldsys_pre) $(libs) $(ldsys)

sys := $(shell uname -s | sed 's/MINGW.*/mingw/')
ifeq ($(sys), mingw)
	bin = game.exe

	ldsys_pre = -static-libgcc -lmingw32 -mconsole
	ldsys = -lopengl32 -lglu32 -lgdi32 -lwinmm
else
	ldsys = -lGL -lGLU -lX11 -lasound -lm
endif

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

.PHONY: crosswin
crosswin:
	$(MAKE) CC=i686-w64-mingw32-gcc sys=mingw

.PHONY: crosswin-clean
crosswin-clean:
	$(MAKE) CC=i686-w64-mingw32-gcc sys=mingw clean

.PHONY: crosswin-cleandep
crosswin-cleandep:
	$(MAKE) CC=i686-w64-mingw32-gcc sys=mingw cleandep
