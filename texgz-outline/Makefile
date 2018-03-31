TARGET  = texgz-outline
CLASSES =
SOURCE  = $(TARGET).c $(CLASSES:%=%.c)
OBJECTS = $(TARGET).o $(CLASSES:%=%.o)
HFILES  = $(CLASSES:%=%.h)
OPT     = -O2 -Wall
CFLAGS  = $(OPT) -I.
LDFLAGS = -Ltexgz -ltexgz -lpng -lz
CCC     = gcc
ifeq ($(TEXGZ_USE_JP2),1)
	CFLAGS  += -DTEXGZ_USE_JP2
	LDFLAGS += -lopenjp2
endif

all: $(TARGET)

$(TARGET): $(OBJECTS) texgz
	$(CCC) $(OPT) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: texgz

texgz:
	$(MAKE) -C texgz

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C texgz clean
	rm texgz

$(OBJECTS): $(HFILES)
