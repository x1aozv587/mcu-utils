CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra
INC     := -I..

SRC_DIR := ..
BLD_DIR := tmp_build

SRCS := $(SRC_DIR)/lists.c \
        $(SRC_DIR)/test_lists.c

OBJS := $(patsubst $(SRC_DIR)/%.c,$(BLD_DIR)/%.o,$(SRCS))

TARGET := $(BLD_DIR)/test_lists.exe

all: $(TARGET)

$(BLD_DIR):
	mkdir -p $(BLD_DIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(BLD_DIR)/%.o: $(SRC_DIR)/%.c | $(BLD_DIR)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(BLD_DIR)/*.o $(TARGET)