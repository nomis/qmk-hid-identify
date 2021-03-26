.PHONY: all clean

CFLAGS = -O2 -g -Wall -Wextra -Wshadow -Werror -std=c99
CXXFLAGS = -O2 -g -Wall -Wextra -Wshadow -Werror -std=c++11
LDFLAGS = -flto
LDLIBS = -lstdc++

all: qmk-hid-identify

qmk-hid-identify: qmk-hid-identify.o main.o hid-report-desc.o usb-vid-pid.o
qmk-hid-identify.o: hid-report-desc.h qmk-hid-identify.h ../common/usb-vid-pid.h
hid-report-desc.o: hid-report-desc.h
usb-vid-pid.o: ../common/usb-vid-pid.c ../common/usb-vid-pid.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c $< -o $@
main.o: qmk-hid-identify.h

clean:
	rm -f -- qmk-hid-identify *.o
