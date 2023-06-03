obj = font.o draw.o drawgl.o drawrast.o utf8.o
alib = ../unix/drawtext.a

CFLAGS = -O3 -g -DNO_FREETYPE -DNO_OPENGL

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(obj) $(alib)
