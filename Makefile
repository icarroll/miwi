wiki: wiki.cpp
	g++ -std=gnu++11 -I. -o wiki wiki.cpp /usr/lib/x86_64-linux-gnu/libboost_system.so.1.65.1 -lpthread
