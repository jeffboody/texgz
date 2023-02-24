export CC_USE_MATH = 1

TARGET  = texgz-convert
CLASSES =
SOURCE  = $(TARGET).c $(CLASSES:%=%.c)
OBJECTS = $(TARGET).o $(CLASSES:%=%.o)
HFILES  = $(CLASSES:%=%.h)
OPT     = -O2 -Wall
CFLAGS  = $(OPT) -I.
LDFLAGS = -Ltexgz -ltexgz -Llibcc -lcc -ljpeg -lz -lm
CCC     = gcc
ifeq ($(TEXGZ_USE_JP2),1)
	CFLAGS  += -DTEXGZ_USE_JP2
	LDFLAGS += -lopenjp2
endif

all: $(TARGET)

$(TARGET): $(OBJECTS) libcc texgz
	$(CCC) $(OPT) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: libcc texgz

libcc:
	$(MAKE) -C libcc

texgz:
	$(MAKE) -C texgz

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C libcc clean
	$(MAKE) -C texgz clean
	rm libcc texgz

$(OBJECTS): $(HFILES)
