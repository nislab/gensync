# Makefile

# Variables
CMAKE := cmake
CMAKE_BUILD := cmake --build
BUILD_DIR := build

.PHONY: all clean

all: $(BUILD_DIR)
	@$(CMAKE_BUILD) $(BUILD_DIR)

$(BUILD_DIR):
	@$(CMAKE) -B $(BUILD_DIR)

clean:
	@$(RM) -r $(BUILD_DIR)
