.PHONY: all clean

CXXFLAGS = -O2 -g -Wall -Wextra -Wshadow -Werror -pedantic -pedantic-errors -std=c++17
LDFLAGS = -flto
LDLIBS = -lstdc++

all: qmk-hid-identify

qmk-hid-identify: main.o qmk-hid-identify.o hid-report-desc.o hid-device.o usb-vid-pid.o

main.o: qmk-hid-identify.h ../common/hid-device.h ../common/types.h unique-fd.h
qmk-hid-identify.o: qmk-hid-identify.h ../common/hid-device.h ../common/types.h hid-report-desc.h unique-fd.h
hid-report-desc.o: hid-report-desc.h ../common/types.h

hid-device.o: ../common/hid-device.cc ../common/hid-device.h ../common/types.h ../common/usb-vid-pid.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c $< -o $@
usb-vid-pid.o: ../common/usb-vid-pid.cc ../common/usb-vid-pid.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c $< -o $@

#qmk-hid-identify.h: ../common/hid-device.h ../common/types.h unique-fd.h
#hid-report-desc.h: ../common/types.h
#../common/hid-device.h: ../common/types.h
#../common/types.h:
#../common/usb-vid-pid.h:

clean:
	rm -f -- qmk-hid-identify *.o
