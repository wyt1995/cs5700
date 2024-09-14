CXX = g++
CXXFLAGS = -std=c++17 -Wall -Werror -g

TARGET = client
SRC = client.cpp
OBJ = $(SRC:.cpp=.o)
INCLUDES = -I/usr/include -I/opt/homebrew/include

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $<

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
