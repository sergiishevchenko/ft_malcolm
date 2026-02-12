NAME		= ft_malcolm

CC			= gcc
CFLAGS		= -Wall -Wextra -Werror

SRC_DIR		= srcs
INC_DIR		= includes
LIBFT_DIR	= libft

SRCS		= $(SRC_DIR)/main.c \
			  $(SRC_DIR)/parsing.c \
			  $(SRC_DIR)/validation.c \
			  $(SRC_DIR)/network.c \
			  $(SRC_DIR)/arp.c \
			  $(SRC_DIR)/signal_handler.c \
			  $(SRC_DIR)/utils.c \
			  $(SRC_DIR)/verbose.c

OBJS		= $(SRCS:.c=.o)
LIBFT		= $(LIBFT_DIR)/libft.a

all: $(NAME)

$(LIBFT):
	$(MAKE) -C $(LIBFT_DIR)

$(NAME): $(LIBFT) $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS) -L$(LIBFT_DIR) -lft

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -I$(LIBFT_DIR) -c $< -o $@

clean:
	$(MAKE) -C $(LIBFT_DIR) clean
	rm -f $(OBJS)

fclean: clean
	$(MAKE) -C $(LIBFT_DIR) fclean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
