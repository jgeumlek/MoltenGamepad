SRCS:=$(shell echo *.cpp)
OBJS=$(subst .cpp,.o,$(SRCS))




plug.a : $(OBJS)
	ar rcs $@ $^


%.o : %.cpp $(SRCS)
	g++ -std=c++14 -c -o $@ $<
