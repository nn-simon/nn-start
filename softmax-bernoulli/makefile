include env.mk

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,%.o,$(SRCS)) #生成所有.c文件对应的.o文件
MODEL_SRCS := $(shell ls model/*.c)
UTILS_SRCS := $(shell ls utils/*.c)
GET_HID_SRCS := $(shell ls get_hid/*.c)
MODEL_OBJS := $(patsubst %.c,%.o,$(MODEL_SRCS)) 
UTILS_OBJS := $(patsubst %.c,%.o,$(UTILS_SRCS)) 
GET_HID_OBJS := $(patsubst %.c,%.o,$(GET_HID_SRCS)) 

all : data2V train output tune
.PHONY : all

$(OBJS) : %.o : %.c
	$(CC) $(SHARED_CFLAGS) -c $< -o $@

tune : tune.o $(UTILS_OBJS) $(MODEL_OBJS) $(NAG_OBJS)
	$(CC) $(SHARED_LDFLAGS) $^ -o $@

data2V : data2V.o $(UTILS_OBJS)
	$(CC) $^ -o $@

train : train.o $(UTILS_OBJS) $(MODEL_OBJS) $(GET_HID_OBJS) $(NAG_OBJS)
	$(CC) $(SHARED_LDFLAGS) $^ -o $@

output : output.o $(GET_HID_OBJS) $(UTILS_OBJS) $(MODEL_OBJS) $(NAG_OBJS)
	$(CC) $(SHARED_LDFLAGS) $^ -o $@

$(MODEL_OBJS) : $(MODEL_SRCS)
	cd model && $(MAKE)

$(UTILS_OBJS) : $(UTILS_SRCS)
	cd utils && $(MAKE)

$(GET_HID_OBJS) : $(GET_HID_SRCS)
	cd get_hid && $(MAKE)

.PHONY : clean
clean :
	rm -f $(OBJS) $(TARGET)
	cd model && $(MAKE) clean
	cd utils && $(MAKE) clean
	cd get_hid && $(MAKE) clean
