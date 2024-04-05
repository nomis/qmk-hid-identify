.PHONY: all debug compile debug-compile check analyse clean distclean install uninstall

BUILD_DIR=build
RELEASE_DIR=$(BUILD_DIR)/release
DEBUG_DIR=$(BUILD_DIR)/debug

ifeq ($(V),1)
	NINJA=ninja -v
else
	NINJA=ninja
endif

ifneq ($(OS),Windows_NT)
	MESON_CROSS=--cross cross64.ini
else
	MESON_CROSS=
endif

all: compile
debug: debug-compile

$(BUILD_DIR)/:
	mkdir $(BUILD_DIR)/
$(RELEASE_DIR)/: | $(BUILD_DIR)/
	rm -rf "$(RELEASE_DIR)/"
	mkdir $(RELEASE_DIR)/
	meson setup --buildtype=release $(RELEASE_DIR)/ $(MESON_CROSS) $(MESON_OPTS) || (rm -rf "$(RELEASE_DIR)/"; false)
$(DEBUG_DIR)/: | $(BUILD_DIR)/
	rm -rf "$(DEBUG_DIR)/"
	mkdir $(DEBUG_DIR)/
	meson setup --buildtype=debug $(DEBUG_DIR)/ $(MESON_CROSS) $(MESON_OPTS) || (rm -rf "$(DEBUG_DIR)/"; false)

compile: | $(RELEASE_DIR)/
	$(NINJA) -C $(RELEASE_DIR)/
debug-compile: | $(DEBUG_DIR)/
	$(NINJA) -C $(DEBUG_DIR)/

analyse: | $(RELEASE_DIR)/
	$(NINJA) -C $(RELEASE_DIR)/ cppcheck

clean:
ifneq ($(wildcard $(RELEASE_DIR)/),)
	$(NINJA) -C $(RELEASE_DIR)/ clean
endif
ifneq ($(wildcard $(DEBUG_DIR)/),)
	$(NINJA) -C $(DEBUG_DIR)/ clean
endif

distclean:
	rm -rf $(BUILD_DIR)/

install: | $(RELEASE_DIR)/
	$(NINJA) -C $(RELEASE_DIR)/ install

uninstall: | $(RELEASE_DIR)/
	$(NINJA) -C $(RELEASE_DIR)/ uninstall
