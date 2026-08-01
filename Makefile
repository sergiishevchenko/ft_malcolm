NAME		= ft_malcolm

CC			= gcc
CFLAGS		= -Wall -Wextra -Werror

SRC_DIR		= srcs
INC_DIR		= includes
LIBFT_DIR	= libft

SRCS		= $(SRC_DIR)/main.c \
			  $(SRC_DIR)/parsing.c \
			  $(SRC_DIR)/validate_ip.c \
			  $(SRC_DIR)/validate_mac.c \
			  $(SRC_DIR)/network.c \
			  $(SRC_DIR)/arp_listen.c \
			  $(SRC_DIR)/arp_send.c \
			  $(SRC_DIR)/signal_handler.c \
			  $(SRC_DIR)/utils.c \
			  $(SRC_DIR)/verbose.c

OBJS		= $(SRCS:.c=.o)
LIBFT		= $(LIBFT_DIR)/libft.a

GREEN		= \033[0;32m
CYAN		= \033[0;36m
YELLOW		= \033[0;33m
RED			= \033[0;31m
RESET		= \033[0m

all: $(NAME)

$(LIBFT):
	@$(MAKE) -C $(LIBFT_DIR)

$(NAME): $(LIBFT) $(OBJS)
	@$(CC) $(CFLAGS) -o $(NAME) $(OBJS) -L$(LIBFT_DIR) -lft
	@printf "$(GREEN)✔ Linked $(NAME)$(RESET)\n"

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@$(CC) $(CFLAGS) -I$(INC_DIR) -I$(LIBFT_DIR) -c $< -o $@
	@printf "$(CYAN)  CC  $(RESET)$<\n"

clean:
	@$(MAKE) -C $(LIBFT_DIR) clean
	@rm -f $(OBJS)
	@printf "$(YELLOW)✔ Cleaned objects$(RESET)\n"

fclean: clean
	@$(MAKE) -C $(LIBFT_DIR) fclean
	@rm -f $(NAME)
	@printf "$(RED)✔ Removed $(NAME)$(RESET)\n"

re: fclean all

.PHONY: all clean fclean re
