CC=gcc -m32
CFLAGS=-O2 -Wall
LIBS=-pthread

FILES = Makefile csapp.h csapp.c echoclient.c echoservert.c echo.c proxy.c

PROGS = echoclient echoserver proxy

all: $(PROGS)

echoclient: echoclient.c csapp.o csapp.h
	$(CC) $(CFLAGS) $(LIBS) -o echoclient echoclient.c csapp.o

echoserver: echoservert.c csapp.o csapp.h echo.o echo.h
	$(CC) $(CFLAGS) $(LIBS) -o echoserver echoservert.c csapp.o echo.o

proxy: proxy.c csapp.o csapp.h
	$(CC) $(CFLAGS) $(LIBS) -o proxy proxy.c csapp.o

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

echo.o: echo.c csapp.h
	$(CC) $(CFLAGS) -c echo.c

clean:
	rm -f *.o *~ *.exe *.tar
	rm -f $(PROGS)
