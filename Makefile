CXXFLAGS := -O0 -ggdb -Wall -Wextra -Wno-unused-parameter
CFLAGS := $(CXXFLAGS)

override CFLAGS += -Wmissing-prototypes -ansi -std=gnu99 -D_GNU_SOURCE

DEP_CFLAGS := $(shell pkg-config --cflags ptlib x11 xv glib-2.0)
DEP_LIBS := $(shell pkg-config --libs ptlib x11 xv glib-2.0)

GLIB_CFLAGS := $(shell pkg-config --cflags glib-2.0)

test: test.o pixops.o xwindow.o xvwindow.o boost-exceptions.o
test: override CXXFLAGS += $(DEP_CFLAGS) -I .
test: override CFLAGS += $(GLIB_CFLAGS)
test: override LIBS += $(DEP_LIBS) -lm -lstdc++
bins += test

all: $(bins)

$(bins):
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o:: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -o $@ -c $<

%.o:: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -o $@ -c $<

clean:
	$(RM) $(bins) *.o *.d
