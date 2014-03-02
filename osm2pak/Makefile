TARGET   = osm2pak
CLASSES  =
SOURCE   = $(TARGET).c $(CLASSES:%=%.c)
OBJECTS  = $(TARGET).o $(CLASSES:%=%.o)
HFILES   = $(CLASSES:%=%.h)
OPT      = -O2 -Wall
#OPT      = -g -Wall
CFLAGS   = $(OPT) -I.
LDFLAGS  = -Llibpak -lpak -Ltexgz -ltexgz -lpng -lm -lz
CCC      = gcc

all: $(TARGET)

$(TARGET): $(OBJECTS) libpak texgz
	$(CCC) $(OPT) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: libpak texgz

libpak:
	$(MAKE) -C libpak

texgz:
	$(MAKE) -C texgz

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C libpak clean
	$(MAKE) -C texgz clean
	rm libpak texgz

$(OBJECTS): $(HFILES)
