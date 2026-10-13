INCLUDE = include

SRCS = $(shell find modules -name "*.c")
ifeq ($(os), win)
	SRCS := $(filter-out %linux.c,$(SRCS))
else
	SRCS := $(filter-out %win.c,$(SRCS))
endif

OBJS_RAW = $(SRCS:.c=.o)
TARGET = midilab


OBJS = $(filter %.o, $(OBJS_RAW))
BAD_FILES = $(filter-out %.o, $(OBJS_RAW))
ifneq ($(BAD_FILES),)
	$(error WARNING: Non-object file detected in OBJS list: [$(BAD_FILES)]. Please use .o extensions only)
endif


DEPS_SRC_WIN := $(wildcard libs/win/*.dll)
DEPS_DST_WIN := $(patsubst libs/win/%, ./%, $(DEPS_SRC_WIN))

DEPS_SRC_LIN := libs/linux/libfluidsynth.so
DEPS_DST_LIN := ./libfluidsynth.so

ifeq ($(os), win)
    DEPS_SRC := $(DEPS_SRC_WIN)
    DEPS_DST := $(DEPS_DST_WIN)
else
    DEPS_SRC := $(DEPS_SRC_LIN)
    DEPS_DST := $(DEPS_DST_LIN)
endif


CC = gcc
WARNINGS = -Wall -Wextra -Werror -Wpedantic

CFLAGS = -I$(INCLUDE) $(WARNINGS)
modules/tinyfiledialogs/tinyfiledialogs.o: CFLAGS += -Wno-pedantic -Wno-cast-function-type -Wno-format

LDFLAGS = -Llibs/linux -lraylib libs/linux/libmp3lame.a -lfluidsynth -lGL -lm -lpthread -ldl -lrt -lX11 -Wl,-rpath,'$$ORIGIN/bin/libs'
OUT = $(TARGET)

ifeq ($(debug), 1)
	CFLAGS += -DDEBUG
endif

ifeq ($(os), win)
	CC = x86_64-w64-mingw32-gcc
	WINDRES = x86_64-w64-mingw32-windres
	OUT = $(TARGET).exe
	OBJS += resource.o

	COMMON_WIN_LIBS = -Llibs/win libs/win/libraylib.a libs/win/libmp3lame.a libs/win/libfluidsynth-3.lib -lgdi32 -lwinmm -lopengl32 -lole32 -lcomdlg32 -lws2_32 -ldsound

	ifeq ($(debug), 1)
		LDFLAGS = -static-libgcc $(COMMON_WIN_LIBS)
	else
		LDFLAGS = -static-libgcc $(COMMON_WIN_LIBS) -mwindows
	endif
endif

ifneq ($(filter mem,$(MAKECMDGOALS)),)
	ifneq ($(os), win)
		CFLAGS += -fsanitize=address -g
		LDFLAGS += -fsanitize=address
	endif
endif


all: $(OUT) copy_deps


copy_deps: $(DEPS_DST)
ifeq ($(os), win)
	rm -f $(DEPS_DST_LIN)
else
	rm -f $(DEPS_DST_WIN)
endif


$(OUT): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(OUT) $(LDFLAGS)


%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

./%.dll: libs/win/%.dll
	cp $< $@

./%.so: libs/linux/%.so
	cp $< $@

ifeq ($(os), win)
resource.o: resource.rc assets/logo/midilab.ico
	$(WINDRES) resource.rc -o $@
endif

clean:
	rm -f *.o $(OBJS) $(TARGET) $(TARGET).exe resource.o
	rm -rf bin
	rm -f -- *.dll *.so

mostlyclean:
	rm -f *.o $(OBJS) resource.o

run: all
	./$(OUT)


mem: run

.PHONY: all clean mostlyclean run mem