EXEC   := ttinfo
EXECPPC:= ttinfo.ppc
EXECARM:= ttinfo.arm
SRCS   := ttinfo.c
OBJS   := ttinfo.o
CFLAGS := -g -Og -W -Wall -Wextra

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $@ $^ $(LDLIBS)

$(EXECPPC): $(SRCS)
	powerpc-unknown-linux-gnu-gcc -o $@ $^

$(EXECPPC): $(SRCS)
	arm-unknown-linux-gnueabi-gcc -o $@ $^

clean:
	$(RM) $(EXEC) $(EXECPPC) $(EXECARM) $(OBJS)

distclean: clean
	$(RM) *.o *~ *.bak
