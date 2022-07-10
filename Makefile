CC = gcc
SRC_DIR = src
INC_DIR = .
OBJ_DIR = obj
CFLAGS = -std=gnu18 -pthread -Wall -Wextra -Wpedantic -O2
NAME = webserver
OBJS = $(addprefix $(OBJ_DIR)/, main.o http_common.o http_response.o http_request.o)

all: prog

prog: pre $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

pre:
	@if [ ! -d "$(OBJ_DIR)" ]; then mkdir $(OBJ_DIR); fi;

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(CC) $(CFLAGS) -c -I$(INC_DIR) $< -o $@

clean:
	@if [ -d "$(OBJ_DIR)" ]; then rm -r -f $(OBJ_DIR); fi;

distclean: clean
	@rm -f $(NAME)
