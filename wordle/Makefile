CXX = g++
CXXFLAGS = -std=c++17 -Wall -Werror -g
LDFLAGS = -L/opt/homebrew/lib -lssl -lcrypto

TARGET = client
SRC = client.cpp
OBJ = $(SRC:.cpp=.o)
INCLUDES = -I/usr/include -I/opt/homebrew/include

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $<

setup:
	apt-get update && apt-get install -y libssl-dev nlohmann-json3-dev

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
