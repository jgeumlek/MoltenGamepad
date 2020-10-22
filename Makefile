#uncomment the lines below to include those plugins within the MG executable.
MG_BUILT_INS+=wiimote
#MG_BUILT_INS+=steamcontroller
#MG_BUILT_INS+=example
#MG_BUILT_INS+=joycon

#uncomment the lines below to build external plugins to be loaded at run time.
#MG_PLUG_INS+=wiimote
#MG_PLUG_INS+=steamcontroller
#MG_PLUG_INS+=example
#MG_PLUG_INS+=joycon

#If you need to run "make eventlists" and it failed to find your
#input header where all the key codes are defined, put the
#correct path to this header in the following variable.
#(if you can compile successfully, you do not need to do anything here.)
INPUT_HEADER:=""

LDLIBS=-ludev -lpthread -ldl
CPPFLAGS+=-std=c++14 -Wall -Wmissing-field-initializers

SRCS:=$(shell echo source/core/*.cpp source/core/*/*.cpp source/core/*/*/*.cpp)




SRCS:=$(SRCS)
OBJS=$(subst .cpp,.o,$(SRCS))
BUILT_IN_PLUGS=$(patsubst %,source/plugin/%/plug.a,$(MG_BUILT_INS))

BUILT_IN_NEEDED_LIBS=$(patsubst %,source/plugin/%/ldlibs,$(MG_BUILT_INS))
LDLIBS+=$(shell echo "" | cat $(BUILT_IN_NEEDED_LIBS))

EXTERNAL_PLUGS=$(patsubst %,built_plugins/%.so,$(MG_PLUG_INS))

#Borrowed magic to handle using gcc to generate build dependencies.

DEPDIR := .d/source
$(shell mkdir -p $(DEPDIR) >/dev/null)
$(shell find source -type d -exec mkdir -p .d/{} \;)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cpp = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d


all : moltengamepad plugins documentation/moltengamepad.1

.SECONDEXPANSION:
%.o : %.c
%.o : %.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

.SECONDEXPANSION:
%.o : %.cpp
source/%.o : source/%.cpp $(DEPDIR)/%.d
	@echo "compiling $<..."
	@$(COMPILE.cpp) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)


$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

.SECONDEXPANSION:
-include $(patsubst %,.d/%.d,$(basename $(SRCS)))



moltengamepad : source/core/mg_core.a $(BUILT_IN_PLUGS)
	@echo "=========================================================="
	@echo "The following plugins are being statically included:"
	@echo "    " $(MG_BUILT_INS)
	@echo "=========================================================="
	$(CXX) $(LDFLAGS) -o moltengamepad source/core/mg_core.a -Wl,--whole-archive $(BUILT_IN_PLUGS) -Wl,--no-whole-archive $(LDLIBS)


plugins : $(EXTERNAL_PLUGS)
	@echo "=========================================================="
	@echo "The following plugins were built:"
	@echo "    " $(MG_PLUG_INS)
	@echo "Plugin .so files are located in built_plugins/"
	@echo "They still need to be moved to a <MG config dir>/plugins/ directory before MG will see them."
	@echo ""
	@echo "Don't forget that external plugins won't be loaded unless "
	@echo "  --load-plugins is specified when launching MG."
	@echo "=========================================================="

built_plugins :
	mkdir built_plugins

source/core/mg_core.a : $(OBJS)
	ar rcs $@ $^

.SECONDEXPANSION:
source/plugin/%/plug.a : force_look
	cd source/plugin/$*; $(MAKE) $(MFLAGS) plug.a

source/plugin/%/plug.so : force_look
	cd source/plugin/$*; $(MAKE) $(MFLAGS) plug.so

.SECONDEXPANSION:
built_plugins/%.so : source/plugin/%/plug.so built_plugins
	cp source/plugin/$*/plug.so built_plugins/$*.so

force_look:
	true

EVENTSOURCES=source/core/eventlists/key_list.cpp source/core/eventlists/axis_list.cpp source/core/eventlists/rel_list.cpp
BUILT_INS_CLEAN=$(addsuffix _clean,$(MG_BUILT_INS))
EXTERNAL_PLUGS_CLEAN=$(addsuffix _clean,$(MG_PLUG_INS))

clean : $(BUILT_INS_CLEAN)
	$(RM) moltengamepad
	$(RM) $(OBJS)
	$(RM) source/core/mg_core.a
	$(RM) *~ *.bak */*~ */*.bak
	$(RM) documentation/moltengamepad.1.md documentation/moltengamepad.1

clean_plugins : $(EXTERNAL_PLUGS_CLEAN)
	$(RM) $(EXTERNAL_PLUGS)

%_clean :
	$(MAKE) -C source/plugin/$* clean

distclean: clean clean_plugins
	$(RM) -r .d
	$(RM) $(EVENTSOURCES)

.PHONY: debug
debug : CPPFLAGS+=-DDEBUG -g
debug : moltengamepad

.PHONY: eventlists
eventlists : $(EVENTSOURCES)

$(EVENTSOURCES): source/core/eventlists/generate_key_codes
	cd source/core/eventlists && ./generate_key_codes $(INPUT_HEADER)

.PHONY: steam
steam :
	MG_BUILT_INS="steamcontroller" $(MAKE) moltengamepad

documentation/moltengamepad.1.md: documentation/manpage-start.md README.md \
 documentation/config_files.md documentation/profiles.md documentation/gendev.md documentation/manpage-end.md
	cat $+ >$@

documentation/moltengamepad.1: documentation/moltengamepad.1.md
	go-md2man -in=$< -out=$@

.PHONY: debian
debian:
	dpkg-buildpackage -rfakeroot -sa -uc -us
