# Makefile

# Compiler
CC = g++

# Compiler flags
CFLAGS = -Wall -Wextra

# Directories
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj

# Targets
TARGET = main

# Build target
build: $(OBJ_DIR)/$(TARGET).o
	$(CC) -o $(BIN_DIR)/$(TARGET) $(OBJ_DIR)/$(TARGET).o

# Clean target
clean:
	 rm -rf $(BIN_DIR)/$(TARGET) $(OBJ_DIR)/*.o

test:
	echo "Running tests..."

# Run target
run: build
	$(BIN_DIR)/$(TARGET)

# Pattern rules
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@