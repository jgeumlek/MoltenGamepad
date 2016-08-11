all : moltengamepad

LDLIBS=-ludev -lpthread
CPPFLAGS+=-std=c++14

SRCS=$(shell echo source/core/*.cpp source/core/*/*.cpp source/core/*/*/*.cpp source/plugin/*.cpp)
SRCS+=$(shell echo source/plugin/wiimote/*.cpp)
OBJS=$(subst .cpp,.o,$(SRCS))

#Borrowed magic to handle using gcc to generate build dependencies.

DEPDIR := .d/source
$(shell mkdir -p $(DEPDIR) >/dev/null)
$(shell find source -type d -exec mkdir -p .d/{} \;)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cpp = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d


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
	$(CXX) $(LDFLAGS) -o moltengamepad $(OBJS) $(LDLIBS)

clean :
	$(RM) moltengamepad
	$(RM) $(OBJS)

.PHONY: debug
debug : CPPFLAGS+=-DDEBUG -g
debug : moltengamepad

.PHONY: steam
steam : CPPFLAGS+=-DBUILD_STEAM_CONTROLLER_DRIVER
steam : LDLIBS+=-lscraw
steam : moltengamepad
