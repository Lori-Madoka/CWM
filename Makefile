build:
	g++ -o testimp testimp.cpp -lX11

install:
	cp testimp /usr/local/bin/cwm
