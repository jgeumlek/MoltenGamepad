
all:
	g++  -std=c++14 -ludev -lpthread -I source source/*.cpp source/*/*.cpp source/*/*/*.cpp -o moltengamepad

clean:
	rm ./moltengamepad
debug:
	g++ -g -std=c++14 -ludev -lpthread -I source source/*.cpp source/*/*.cpp source/*/*/*.cpp -o moltengamepad

steam:
	g++  -DBUILD_STEAM_CONTROLLER_DRIVER -std=c++14 -lscraw -ludev -lpthread -I source source/*.cpp source/*/*.cpp source/*/*/*.cpp -o moltengamepad
