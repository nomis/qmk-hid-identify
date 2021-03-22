.PHONY: all clean

CFLAGS = -O2 -g -Wall -Wextra -Wshadow -Werror
CXXFLAGS = -O2 -g -Wall -Wextra -Wshadow -Werror
LDFLAGS = -flto
LDLIBS = -lstdc++

all: qmk-hid-identify

qmk-hid-identify: qmk-hid-identify.o main.o hid-report-desc.o
qmk-hid-identify.o: hid-report-desc.h qmk-hid-identify.h
hid-report-desc.o: hid-report-desc.h
main.o: qmk-hid-identify.h

clean:
	rm -f -- qmk-hid-identify *.o
