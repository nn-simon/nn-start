include ../env.mk

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,%.o,$(SRCS))


.PHONY : all
all : $(OBJS)

$(OBJS) : %.o : %.c
	$(CC) $(SHARED_CFLAGS) -c $< -o $@

.PHONY : clean
clean :
	rm -f $(OBJS) $(TARGET)
