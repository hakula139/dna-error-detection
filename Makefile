TARGET    := solution
TEST      := test

BIN_DIR   := bin
BUILD_DIR := build
SRC_DIR   := src
TEST_DIR  := tests/unit

SRCS      := $(shell find $(SRC_DIR) -name *.cpp)
OBJS      := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS      := $(OBJS:.o=.d)

TEST_SRCS := $(shell find $(TEST_DIR) -name *.cpp)
TEST_OBJS := $(TEST_SRCS:%=$(BUILD_DIR)/%.o)
TEST_OBJS += $(filter-out $(BUILD_DIR)/$(SRC_DIR)/main.cpp.o, $(OBJS))

INC_DIRS  := $(shell find $(SRC_DIR) $(TEST_DIR) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CXX       := clang++
CXXFLAGS  := -g -Wall -O3 -std=c++17 -stdlib=libc++ $(INC_FLAGS) -MMD -MP
MKDIR     := mkdir -p
RM        := rm -rf

.PHONY: all test run index merge start clean

all: $(BIN_DIR)/$(TARGET)

test: $(BIN_DIR)/$(TEST)
	@$<

run: $(BIN_DIR)/$(TARGET)
	@$< -a

index: $(BIN_DIR)/$(TARGET)
	@$< -i

merge: $(BIN_DIR)/$(TARGET)
	@$< -m

start: $(BIN_DIR)/$(TARGET)
	@$< -s

clean:
	@$(RM) $(BIN_DIR) $(BUILD_DIR)

$(BIN_DIR)/$(TARGET): $(OBJS)
	@echo + $@
	@$(MKDIR) $(dir $@)
	@$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

$(BIN_DIR)/$(TEST): $(TEST_OBJS)
	@echo + $@
	@$(MKDIR) $(dir $@)
	@$(CXX) $(CXXFLAGS) -o $@ $(TEST_OBJS)

$(BUILD_DIR)/%.cpp.o: %.cpp
	@echo + $@
	@$(MKDIR) $(dir $@)
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

-include $(DEPS)
