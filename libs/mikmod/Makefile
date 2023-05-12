obj = drivers/drv_alsa.o drivers/drv_ds.o drivers/drv_nos.o drivers/drv_oss.o \
	  drivers/drv_osx.o drivers/drv_pulse.o drivers/drv_sdl.o drivers/drv_sgi.o\
	  drivers/drv_win.o drivers/drv_xaudio2.o \
	  loaders/load_mod.o loaders/load_xm.o loaders/load_s3m.o loaders/load_it.o\
	  mmio/mmio.o mmio/mmerror.o mmio/mmalloc.o \
	  depackers/mmcmp.o depackers/s404.o depackers/pp20.o depackers/xpk.o \
	  playercode/mdreg.o playercode/mdulaw.o playercode/mlreg.o \
	  playercode/mloader.o playercode/virtch2.o playercode/virtch.o \
	  playercode/mplayer.o playercode/sloader.o playercode/mwav.o \
	  playercode/npertab.o playercode/mdriver.o playercode/virtch_common.o \
	  playercode/mlutil.o playercode/munitrk.o \
	  posix/strcasecmp.o

alib = ../unix/mikmod.a

CFLAGS = -g -I. -Iinclude -DHAVE_CONFIG_H -DMIKMOD_STATIC

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(obj) $(alib)
