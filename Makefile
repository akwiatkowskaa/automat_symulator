CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -lncurses

TARGET = automat
OBJS = main.o automat.o ipc.o klient.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

main.o: main.c automat.h klient.h
	$(CC) $(CFLAGS) -c main.c

automat.o: automat.c automat.h ipc.h
	$(CC) $(CFLAGS) -c automat.c

klient.o: klient.c klient.h automat.h ipc.h
	$(CC) $(CFLAGS) -c klient.c

ipc.o: ipc.c ipc.h
	$(CC) $(CFLAGS) -c ipc.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
