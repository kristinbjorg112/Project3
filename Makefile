all:
		rm -f *o server
		rm -f *o client	
		g++ -std=c++11 client.cpp -lpthread -o client
		g++ -std=c++11 tsamp3group20.cpp -o tsamp3group20
warnings:
		g++ -std=c++11 -Wall client.cpp -lpthread -o client
		g++ -std=c++11 -Wall tsamp3group20.cpp -o tsamp3group20
clean:
		rm -f *o server
		rm -f *o client