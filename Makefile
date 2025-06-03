CC = clang
CFLAGS = -std=c11 -g -Wall -Werror -Wextra

OBJ_DIR = ./obj

SRCS = main.c
OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)

TARGET = main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean-all:
	rm -rf $(OBJ_DIR) $(TARGET)

clean-obj: 
	rm -rf $(OBJ_DIR)

clean: clean-all

.PHONY: all clean-all clean-obj
