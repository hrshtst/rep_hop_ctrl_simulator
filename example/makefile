ROOTDIR=..
INCDIR=$(ROOTDIR)/include
SRCDIR=$(ROOTDIR)/src
INCLUDE=-I$(INCDIR)
LINK=-lm -lrt

CC=gcc
CFLAGS=-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -O3 $(INCLUDE) -funroll-loops -MMD -MP

OBJS_SRC=$(wildcard $(SRCDIR)/*.c)
TARGET_SRC=$(wildcard *.c)

OBJS=$(OBJS_SRC:.c=.o)
TARGET=$(TARGET_SRC:.c=)

DEPS=$(OBJS_SRC:.c=.d) $(TARGET_SRC:.c=.d)

.PHONY: all
all: $(TARGET)

.PHONY: debug
debug:
	@echo "OBJS   =" $(OBJS)
	@echo "TARGET =" $(TARGET)
	@echo "DEPS   =" $(DEPS)

%: %.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) $(LINK)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $*.o

.PHONY: clean
clean:
	rm -f *.o *~ $(OBJS) $(DEPS) $(TARGET)

.PRECIOUS: $(OBJS)

-include $(DEPS)
