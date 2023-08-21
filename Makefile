export CC_USE_MATH = 1

TARGET  = libtexgz.a
CLASSES = texgz_tex texgz_jpeg texgz_png pil_lanczos
ifeq ($(TEXGZ_USE_JP2),1)
	CLASSES += texgz_jp2
endif
SOURCE  = $(CLASSES:%=%.c)
OBJECTS = $(SOURCE:.c=.o)
HFILES  = $(CLASSES:%=%.h)
OPT     = -O2 -Wall
CFLAGS  = $(OPT) -I.
ifeq ($(TEXGZ_USE_JP2),1)
	CFLAGS += -DTEXGZ_USE_JP2 -I/usr/local/include/openjpeg-2.2
endif
AR      = ar

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)

$(OBJECTS): $(HFILES)
