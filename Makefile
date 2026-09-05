INCLUDE = include

SRCS = $(shell find modules -name "*.c")
#SRCS := $(filter-out modules/backend/export/midi.c,$(SRCS))
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
modules/tinyfiledialogs/tinyfiledialogs.o: CFLAGS += -Wno-pedantic -Wno-cast-function-type -Wno-format

# Platform specific flags
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
OUT = $(TARGET)

ifeq ($(os), win)
    CC = x86_64-w64-mingw32-gcc
    WINDRES = x86_64-w64-mingw32-windres
    OUT = $(TARGET).exe
    OBJS += resource.o

    ifeq ($(debug), 1)
        # Keeps console window open for stdout/stderr logs
        LDFLAGS = libs/libraylib_win.a -lgdi32 -lwinmm -lopengl32 -lole32 -lcomdlg32 -static
    else
        # Hides console
        LDFLAGS = libs/libraylib_win.a -lgdi32 -lwinmm -lopengl32 -lole32 -lcomdlg32 -static -mwindows
    endif
endif

ifneq ($(filter mem,$(MAKECMDGOALS)),)
    ifneq ($(os), win)
        CFLAGS += -fsanitize=address -g
        LDFLAGS += -fsanitize=address
    endif
endif


all: $(OUT)

$(OUT): $(OBJS)
	$(CC) $(OBJS) -o $(OUT) $(CFLAGS) $(LDFLAGS)


%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

ifeq ($(os), win)
resource.o: resource.rc assets/logo/midilab.ico
	$(WINDRES) resource.rc -o $@
endif

clean:
	rm -f *.o $(OBJS) $(TARGET) $(TARGET).exe resource.o

mostlyclean:
	rm -f *.o $(OBJS) resource.o

run: $(OUT)
	./$(OUT)


mem: run

.PHONY: all clean mostlyclean run mem