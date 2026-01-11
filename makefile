CXX = g++
CXXFLAGS = -std=c++17
LIBS = `sdl2-config --cflags --libs` -lGL -lGLEW -lOpenCL -lEGL -ldl

TARGET = app
SRC = main.cpp

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)
