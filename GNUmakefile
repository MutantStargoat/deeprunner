-include config.mk

# defaults for config.mk build options so that config.mk need not exist
rend ?= gl

gawsrc_gl = $(wildcard src/opengl/*.c) src/gaw/gaw_gl.c
gawsrc_sw = $(wildcard src/swsdl/*.c) src/gaw/gaw_sw.c src/gaw/gawswtnl.c \
			src/gaw/polyfill.c src/gaw/polyclip.c

src = $(wildcard src/*.c) $(gawsrc_$(rend))
obj = $(src:.c=.o)
dep = $(src:.c=.d)
bin = game

warn = -pedantic -Wall
dbg = -g
#opt = -O3
def = -DMINIGLUT_USE_LIBC -DMIKMOD_STATIC
inc = -Isrc -Ilibs -Ilibs/imago/src -Ilibs/treestor/include -Ilibs/goat3d/include \
	  -Ilibs/drawtext -Ilibs/mikmod/include
libs = libs/unix/imago.a libs/unix/goat3d.a libs/unix/treestor.a \
	   libs/unix/drawtext.a libs/unix/psys.a libs/unix/mikmod.a

CFLAGS = $(warn) $(dbg) $(opt) $(inc) $(def) $(cflags_$(rend)) -MMD
LDFLAGS = $(ldsys_pre) $(libs) $(ldsys)

cflags_gl = -Isrc/opengl
cflags_sw = -Isrc/swsdl `sdl-config --cflags`

ldflags_sw = `sdl-config --libs`

sys := $(shell uname -s | sed 's/MINGW.*/mingw/')
ifeq ($(sys), mingw)
	bin = game.exe

	ldflags_gl = -lopengl32 -lglu32 -lgdi32 -lwinmm
	ldsys_pre = -static-libgcc -lmingw32 -mconsole
	ldsys = $(ldflags_$(rend))
else
	ldflags_gl = -lGL -lGLU -lX11
	ldsys = $(ldflags_$(rend)) -lasound -lm
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
