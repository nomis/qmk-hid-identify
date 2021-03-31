.PHONY: all clean

CC = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
LD = $(CC)

CXXFLAGS = -O2 -g -Wall -Wextra -Wshadow -Werror -pedantic -pedantic-errors -DUNICODE -DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 -std=c++17
LDFLAGS = -flto -Wl,-subsystem,windows
LDLIBS = -lstdc++ -lsetupapi -lhid

all: qmk-hid-identify.exe

qmk-hid-identify.exe: main.o windows++.o qmk-hid-identify.o hid-device.o usb-vid-pid.o
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

main.o: qmk-hid-identify.h ../common/hid-device.h ../common/types.h windows++.h
windows++.o: windows++.h
qmk-hid-identify.o: qmk-hid-identify.h ../common/hid-device.h ../common/types.h windows++.h

hid-device.o: ../common/hid-device.cc ../common/hid-device.h ../common/types.h ../common/usb-vid-pid.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c $< -o $@
usb-vid-pid.o: ../common/usb-vid-pid.cc ../common/usb-vid-pid.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c $< -o $@

#qmk-hid-identify.h: ../common/hid-device.h ../common/types.h windows++.h
#hid-report-desc.h: ../common/types.h
#../common/hid-device.h: ../common/types.h
#../common/types.h:
#../common/usb-vid-pid.h:

clean:
	rm -f -- qmk-hid-identify.exe *.o
