TARGET    := solution

BIN_DIR   := bin
BUILD_DIR := build
SRC_DIRS  := src

SRCS      := $(shell find $(SRC_DIRS) -name *.cpp)
OBJS      := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS      := $(OBJS:.o=.d)

INC_DIRS  := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CXX       := clang++
CXXFLAGS  := -g -Wall -O3 -std=c++17 -stdlib=libc++ $(INC_FLAGS) -MMD -MP
MKDIR     := mkdir -p
RM        := rm -rf

.PHONY: all run clean

all: $(BIN_DIR)/$(TARGET)
	@$< -a

init: $(BIN_DIR)/$(TARGET)
	@$< -i

run: $(BIN_DIR)/$(TARGET)
	@$< -s

clean:
	@$(RM) $(BIN_DIR) $(BUILD_DIR)

$(BIN_DIR)/$(TARGET): $(OBJS)
	@echo + $@
	@$(MKDIR) $(dir $@)
	@$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.cpp.o: %.cpp
	@echo + $@
	@$(MKDIR) $(dir $@)
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

-include $(DEPS)
