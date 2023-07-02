wiki: wiki.cpp
	g++ -std=gnu++11 -I. -o wiki wiki.cpp -lboost_system -lpthread
