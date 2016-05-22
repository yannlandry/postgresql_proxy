CC=gcc
CFLAGS=-Wall
TARGET=pg_proxy
# TARGET=pg_proxy pg_proxy2 pg_proxy2a

all:	$(TARGET)

pg_proxy: pg_proxy2.c pg_proxy2.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o *.bak $(TARGET)

