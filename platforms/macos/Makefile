include ../shared/makefiles/Makefile.sources

SOURCES_MM += $(DEPS_DIR)/nfd/nfd_cocoa.mm
SOURCES_MM += macos.mm
OBJECTS += $(SOURCES_MM:.mm=.o)
LDFLAGS += -framework AppKit -framework UniformTypeIdentifiers

include ../shared/makefiles/Makefile.common

APP_NAME=Geargrafx

bundle:
	rm -rf $(APP_NAME).app
	mkdir -p $(APP_NAME).app/Contents/{MacOS,Resources,Frameworks}
	sed -e "s/@version@/$(GIT_VERSION)/g" Info.plist > $(APP_NAME).app/Contents/Info.plist
	cp iconfile.icns $(APP_NAME).app/Contents/Resources/
	cp $(TARGET) $(APP_NAME).app/Contents/MacOS/
	cp run.sh $(APP_NAME).app/Contents/MacOS/
	mkdir -p $(APP_NAME).app/Contents/Resources/mcp
	cp ../shared/gamecontrollerdb.txt $(APP_NAME).app/Contents/Resources/
	cp -r ../shared/desktop/mcp/resources $(APP_NAME).app/Contents/Resources/mcp/

dist: clean all bundle
	@echo Success!!
