CXX = g++
CXXFLAGS = -std=c++20 -Wall -Werror -g

TARGET = 4700ftp
SRC = ftp_client.cpp
OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
