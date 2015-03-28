SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,%.o,$(SRCS)) #生成所有.c文件对应的.o文件
PATH_NAG_INC := /home/songjm/local/NAG/cll6a23dhl/include/
CUR_CFLAGS := $(CFLAGS) -I$(PATH_NAG_INC) -I. -g
NAG_OBJS := /home/songjm/local/NAG/cll6a23dhl/lib/libnagc_nag.a /home/songjm/local/NAG/cll6a23dhl/lib/libnagc_acml.a 
CUR_LDFLAGS := -lm -lpthread
TARGET := exercise
.PHONY : all
#all : $(OBJS) $(NAG_OBJS)
#	$(CC) $(NAG_OBJS) $(OBJS) $(CUR_LDFLAGS) -o exercise
all : $(SRCS) $(NAG_OBJS)
	$(CC) $(SRCS) $(NAG_OBJS) $(CUR_CFLAGS) $(CUR_LDFLAGS) -o $(TARGET)

#$(OBJS) : %.o : %.c
#	$(CC) $(CUR_CFLAGS) -c $< -o $@

.PHONY : clean
clean :
	rm -f $(OBJS) $(TARGET)
