TARGET   = unpaktex
CLASSES  =
SOURCE   = $(TARGET).c $(CLASSES:%=%.c)
OBJECTS  = $(TARGET).o $(CLASSES:%=%.o)
HFILES   = $(CLASSES:%=%.h)
OPT      = -O2 -Wall
CFLAGS   = $(OPT) -I.
LDFLAGS  = -Llibpak -lpak -Ltexgz -ltexgz -Llibcc -lcc -ljpeg -lz
CCC      = gcc

all: $(TARGET)

$(TARGET): $(OBJECTS) libcc texgz libpak
	$(CCC) $(OPT) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: libcc texgz libpak

libcc:
	$(MAKE) -C libcc

texgz:
	$(MAKE) -C texgz

libpak:
	$(MAKE) -C libpak

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C libcc clean
	$(MAKE) -C texgz clean
	$(MAKE) -C libpak clean
	rm libcc texgz libpak

$(OBJECTS): $(HFILES)
