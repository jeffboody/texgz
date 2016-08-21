TARGET   = unpaktex
CLASSES  =
SOURCE   = $(TARGET).c $(CLASSES:%=%.c)
OBJECTS  = $(TARGET).o $(CLASSES:%=%.o)
HFILES   = $(CLASSES:%=%.h)
OPT      = -O2 -Wall
CFLAGS   = $(OPT) -I.
LDFLAGS  = -Llibpak -lpak -Ltexgz -ltexgz -lpng -ljpeg -lz
CCC      = gcc

all: $(TARGET)

$(TARGET): $(OBJECTS) texgz libpak
	$(CCC) $(OPT) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: texgz libpak

texgz:
	$(MAKE) -C texgz

libpak:
	$(MAKE) -C libpak

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C texgz clean
	$(MAKE) -C libpak clean
	rm texgz libpak

$(OBJECTS): $(HFILES)