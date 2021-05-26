EXEC   := ttinfo
SRCS   := ttinfo.c
OBJS   := ttinfo.o
CFLAGS := -g -Og -W -Wall -Wextra

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $@ $^ $(LDLIBS)

clean:
	$(RM) $(EXEC) $(OBJS)

distclean: clean
	$(RM) *.o *~ *.bak
