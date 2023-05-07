obj = pattr.o pstrack.o psys.o psys_gl.o rndval.o
alib = ../unix/psys.a

CFLAGS = -O3 -I../imago/src

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(obj) $(alib)
