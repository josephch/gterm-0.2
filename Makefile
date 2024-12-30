CPP = gcc

# For debugging
#CFLAGS = -Wall `pkg-config gtk+-2.0 --cflags` `pkg-config vte --cflags` -g -ansi
#LFLAGS = `pkg-config gtk+-2.0 --libs` `pkg-config vte --libs` -Wl,-O1

# For running normally
CFLAGS = -Wall `pkg-config gtk+-2.0 --cflags` `pkg-config vte --cflags` -Os -ansi -DUSE_VTE_TERMINAL
LFLAGS = `pkg-config gtk+-2.0 --libs` `pkg-config vte --libs` -Wl,-O1 -Wl,--strip-all

.c.o:
	@echo CC $<
	@${CPP} -c ${CFLAGS} $<

OFILES=		gterm.o gtw_tab.o gtw_window.o gte_terminal.o

# dependencies 
#
default:	gterm
#
gterm:           $(OFILES)
		@echo LD $@
		@${CPP} ${CFLAGS} $(OFILES) ${LFLAGS} -o $@

gterm.o:	gterm.c

gtw_tab.o:	gtw_tab.c

gtw_window.o:	gtw_window.c

gte_terminal.o: gte_terminal.c

#
clean:
	@echo -n Cleaning...
	@rm -f *.o gterm
	@echo done
