DEPDIR := .d/source
$(shell mkdir -p $(DEPDIR) >/dev/null)
$(shell find source -type d -exec mkdir -p .d/{} \;)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cpp = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d
LDLIBS =-ludev -lpthread
SRCS=$(shell echo source/*.cpp source/*/*.cpp source/*/*/*.cpp)
OBJS=$(subst .cpp,.o,$(SRCS))

CPPFLAGS+=-std=c++14

.SECONDEXPANSION:
%.o : %.c
%.o : %.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

.SECONDEXPANSION:
%.o : %.cpp
source/%.o : source/%.cpp $(DEPDIR)/%.d
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

.SECONDEXPANSION:
-include $(patsubst %,.d/%.d,$(basename $(SRCS)))


all : moltengamepad

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
