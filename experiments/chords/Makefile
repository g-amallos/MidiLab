INCLUDE = include

SRCS = $(shell find modules -name "*.c")
OBJS_RAW = $(SRCS:.c=.o)
TARGET = chords

OBJS = $(filter %.o, $(OBJS_RAW))
BAD_FILES = $(filter-out %.o, $(OBJS_RAW))
ifneq ($(BAD_FILES),)
	$(error WARNING: Non-object file detected in OBJS list: [$(BAD_FILES)]. Please use .o extensions only)
endif


CC = gcc
WARNINGS = -Wall -Wextra -Werror -Wpedantic
CFLAGS = -I$(INCLUDE) $(WARNINGS)
LDFLAGS = -lm
OUT = $(TARGET)



ifeq ($(debug), 1)
	CFLAGS += -DDEBUG
endif

ifeq ($(os), win)
	CC = x86_64-w64-mingw32-gcc
	OUT = $(TARGET).exe
	COMMON_WIN_LIBS = -lm
endif

ifneq ($(filter mem,$(MAKECMDGOALS)),)
	ifneq ($(os), win)
		CFLAGS += -fsanitize=address -g
		LDFLAGS += -fsanitize=address
	endif
endif


all: $(OUT)



$(OUT): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(OUT) $(LDFLAGS)


%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)


clean:
	rm -f *.o $(OBJS) $(TARGET) $(TARGET).exe

mostlyclean:
	rm -f *.o $(OBJS)

run: all
	./$(OUT)


mem: run

.PHONY: all clean mostlyclean run mem