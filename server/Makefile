SRCS:= $(wildcard *.c)
OBJS:= $(patsubst %.c,%.o,$(SRCS))
CC:= gcc
server: $(OBJS)
	$(CC) $^ -o $@ -g -lpthread -lmysqlclient -ll8w8jwt -lcrypt
%.o:%.c
	$(CC) -c $^ -o $@ -g
clean:
	$(RM) $(OBJS) server
rebuild:clean server