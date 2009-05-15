OBJFILES=conn.o main.o session.o ssh.o http.o manage.o misc.o
CFLAGS_NORMAL=-O2 -Wall
CFLAGS_DEBUG=$(CFLAGS_NORMAL) -g
OUTFILE=portmuxer
ifdef DEBUG
	CFLAGS=$(CFLAGS_DEBUG)
else
	CFLAGS=$(CFLAGS_NORMAL)
endif

all: $(OUTFILE)

$(OUTFILE): $(OBJFILES)
	$(CC) $(CFLAGS) $(OBJFILES) -o ./portmuxer

clean:
	rm -rf $(OBJFILES) $(OUTFILE)

distclean: clean
	rm -rf *~

tags:
	etags *.h *.c
