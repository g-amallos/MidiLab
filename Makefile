INCLUDE = include

SRCS = $(shell find modules -name "*.c")
OBJS_RAW = $(SRCS:.c=.o)
#OBJS_RAW = modules/main.o modules/interface/general.o modules/interface/textfont.o modules/utils/utils.o modules/interface/rectangles.o modules/interface/render/backgrounds.o modules/interface/render/controlLine.o
TARGET = midilab


OBJS = $(filter %.o, $(OBJS_RAW))
BAD_FILES = $(filter-out %.o, $(OBJS_RAW))
ifneq ($(BAD_FILES),)
    $(error WARNING: Non-object file detected in OBJS list: [$(BAD_FILES)]. Please use .o extensions only)
endif


CC = gcc
WARNINGS = -Wall -Wextra -Werror -Wpedantic

# Common flags for both systems
CFLAGS = -I$(INCLUDE) $(WARNINGS)
modules/tinyfiledialogs/tinyfiledialogs.o: CFLAGS += -Wno-pedantic -Wno-cast-function-type

# Platform specific flags
FLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
OUT = $(TARGET)

ifeq ($(os), win)
    CC = x86_64-w64-mingw32-gcc
    FLAGS = libs/libraylib_win.a -lgdi32 -lwinmm -lopengl32 -lole32 -lcomdlg32 -static
    OUT = $(TARGET).exe
endif

all: $(OUT)

$(OUT): $(OBJS)
	$(CC) $(OBJS) -o $(OUT) $(CFLAGS) $(FLAGS)


%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f *.o $(OBJS) $(TARGET) $(TARGET).exe

run: $(OUT)
	./$(OUT)

.PHONY: all clean run