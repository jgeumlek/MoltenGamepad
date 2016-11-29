SRCS:=$(shell echo *.cpp)
OBJS=$(subst .cpp,.o,$(SRCS))

CXXFLAGS+=-std=c++14

COMPILE.cpp = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

plug.a : $(OBJS)
	ar rcs $@ $^


%.o : %.cpp $(SRCS)
	$(COMPILE.cpp) -o $@ $<
