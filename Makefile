EXEC   := ttinfo
EXECPPC:= ttinfo.ppc
SRCS   := ttinfo.c
OBJS   := ttinfo.o
CFLAGS := -g -Og -W -Wall -Wextra

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $@ $^ $(LDLIBS)

$(EXECPPC): $(SRCS)
	/usr/local/powerpc-unknown-linux-gnu-5.3-1/bin/powerpc-unknown-linux-gnu-gcc -o $@ $^

clean:
	$(RM) $(EXEC) $(OBJS)

distclean: clean
	$(RM) *.o *~ *.bak
