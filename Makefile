
#uncomment the lines below to include those plugins
MG_BUILT_INS+="wiimote"
#MG_BUILT_INS+="steamcontroller"

LDLIBS=-ludev -lpthread
CPPFLAGS+=-std=c++14

SRCS=$(shell echo source/core/*.cpp source/core/*/*.cpp source/core/*/*/*.cpp source/plugin/*.cpp)

ifneq (,$(findstring steamcontroller,$(MG_BUILT_INS)))
  LDLIBS+=-lscraw
endif

MG_BUILT_IN_PATHS=$(patsubst %,source/plugin/%/*.cpp,$(MG_BUILT_INS))


SRCS+=$(shell echo $(MG_BUILT_IN_PATHS))
OBJS=$(subst .cpp,.o,$(SRCS))

#Borrowed magic to handle using gcc to generate build dependencies.

DEPDIR := .d/source
$(shell mkdir -p $(DEPDIR) >/dev/null)
$(shell find source -type d -exec mkdir -p .d/{} \;)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cpp = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d


all : moltengamepad
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



moltengamepad : $(OBJS)
	@echo "The following plugins are being statically included:"
	@echo "    " $(MG_BUILT_INS)
	$(CXX) $(LDFLAGS) -o moltengamepad $(OBJS) $(LDLIBS)

clean :
	$(RM) moltengamepad
	$(RM) $(OBJS)

.PHONY: debug
debug : CPPFLAGS+=-DDEBUG -g
debug : moltengamepad

.PHONY: steam
steam : MG_BUILT_INS+="steamcontroller"
steam : LDLIBS+=-lscraw
steam : moltengamepad
