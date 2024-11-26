CC = gcc
SRC_DIR = src
INC_DIR = .
OBJ_DIR = obj
CFLAGS = -std=gnu18 -pthread -Wall -Wextra -Wpedantic -O2 -lgnutls
NAME = webserver
NAME2 = client
OBJS = $(addprefix $(OBJ_DIR)/, main.o http_common.o http_response.o http_request.o)
OBJS2 = $(addprefix $(OBJ_DIR)/, client.o http_common.o http_response.o http_request.o)

all: prog

client: pre $(OBJS2)
	@$(CC) $(CFLAGS) $(OBJS2) -o $(NAME2)

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
