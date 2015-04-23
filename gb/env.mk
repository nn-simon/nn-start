PATH_NAG_INC := /home/songjm/local/NAG/cll6a23dhl/include/
SHARED_CFLAGS := $(CFLAGS) -I$(PATH_NAG_INC) -I. -g
NAG_OBJS := /home/songjm/local/NAG/cll6a23dhl/lib/libnagc_nag.a /home/songjm/local/NAG/cll6a23dhl/lib/libnagc_acml.a /home/songjm/local/NAG/cll6a23dhl/acml/libacml.a
SHARED_LDFLAGS := -lm -lpthread
