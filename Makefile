CXX = g++
CXXFLAGS = -Iinclude -Wall -std=c++14

# Platform-specific settings
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)  # This checks if you're on macOS
    LDFLAGS = 
else
    LDFLAGS = -lpthread
endif


SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

EXEC1 = tsamgroup5
EXEC2 = client

SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))

ROOT_SRC1 = server.cpp
ROOT_SRC2 = client.cpp

ROOT_OBJ1 = $(BUILD_DIR)/server.o
ROOT_OBJ2 = $(BUILD_DIR)/client.o

all: $(BUILD_DIR) $(EXEC1) $(EXEC2)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

$(BUILD_DIR)/server.o: $(ROOT_SRC1)
	$(CXX) $(CXXFLAGS) -c $< -g -o $@ $(LDFLAGS)

$(BUILD_DIR)/client.o: $(ROOT_SRC2)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

$(EXEC1): $(ROOT_OBJ1) $(OBJ_FILES)
	$(CXX) $(ROOT_OBJ1) $(OBJ_FILES) -o $@ $(LDFLAGS)

$(EXEC2): $(ROOT_OBJ2) $(OBJ_FILES)
	$(CXX) $(ROOT_OBJ2) $(OBJ_FILES) -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(EXEC1) $(EXEC2)
