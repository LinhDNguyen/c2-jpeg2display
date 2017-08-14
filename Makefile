CROSS=$(CC:%gcc=%)

jpeg2display_s=jpeg2display.c

INSTALL_FLAGS=-m 755

TARGET=jpeg2display

LDFLAGS= -L/usr/lib/aml_libs -lamcodec -ldl -lamadec -lasound -lamavutils
CFLAGS+= -I$(shell pwd) -pthread -O2 -g

.PHONY: all config  before_cmd install clean distclean run

all: ${TARGET}

install:all
		install $(INSTALL_FLAGS) $(TARGET) $(TARGET_DIR)/bin

distclean:clean

clean:
		rm jpeg2display *.o

${TARGET}:jpeg2display.o
		$(CC) $(CFLAGS) $($(@:%=%_s)) -o $@ $(LDFLAGS)

run:
	DISPLAY=:0 ./jpeg2display

