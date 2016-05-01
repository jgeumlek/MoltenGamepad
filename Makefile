
all:
	g++ -std=c++14 -I source source/*.cpp source/*/*.cpp source/*/*/*.cpp -o moltengamepad -ludev -lpthread

clean:
	rm ./moltengamepad
debug:
	g++ -g -std=c++14 -I source source/*.cpp source/*/*.cpp source/*/*/*.cpp -o moltengamepad -ludev -lpthread

steam:
	g++ -DBUILD_STEAM_CONTROLLER_DRIVER -std=c++14 -lscraw -I source source/*.cpp source/*/*.cpp source/*/*/*.cpp -o moltengamepad -ludev -lpthread
