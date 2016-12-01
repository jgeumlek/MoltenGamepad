SRCS:=$(shell echo *.cpp)
OBJS=$(subst .cpp,.o,$(SRCS))
PLUGOBJS=$(subst .cpp,.po,$(SRCS))

LDLIBS+=$(shell cat ldlibs)

CXXFLAGS+=-std=c++14

COMPILE.cpp = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

steamcontroller.so : $(PLUGOBJS)
	$(CXX) $(LDFLAGS) -DPLUGIN  $^  -shared -o $@ $(LDLIBS)

plug.a : $(OBJS)
	ar rcs $@ $^

%.o : %.cpp $(SRCS)
	$(COMPILE.cpp) -o $@ $<

%.po : %.cpp $(SRCS)
	$(COMPILE.cpp) -fPIC -DPLUGIN -o $@ $<
