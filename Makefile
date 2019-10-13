all:
		rm -f *o server
		rm -f *o client	
		g++ -std=c++17 client.cpp -lpthread -o client
		g++ -std=c++11 tsamgroup20.cpp -lpthread -o server
client: 
		g++ -std=c++11 client.cpp -lpthread -o client
rmclient:
		rm -f *o client	
server:
		g++ -std=c++17 server.cpp -lpthread -o server
rmserver:
		rm -f *o server
rmscratch:
		rm -f scratch
scratch:
		rm -f scratch
		g++-8 -std=c++17 scratch_pad.cpp -o scratch
warnings:
		g++  -std=c++11 -Wall scanner.cpp -o scanner
clean:
		rm -f scratch
		rm -f *o server
		rm -f *o client		
