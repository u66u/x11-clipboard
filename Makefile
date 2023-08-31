all: clip

clip: main.cpp
	g++ -o $@ $^ -lX11

clean:
	rm -f clip
