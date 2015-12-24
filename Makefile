
all:
	g++  -std=c++11 -ludev -lpthread -I source source/*.cpp source/*/*.cpp source/*/*/*.cpp -o moltengamepad

clean:
	rm ./moltengamepad
debug:
	g++ -g -std=c++11 -ludev -lpthread -I source source/*.cpp source/*/*.cpp source/*/*/*.cpp -o moltengamepad
