NAME	= libft.a

CC		= gcc
CFLAGS	= -Wall -Wextra -Werror
AR		= ar rcs

SRCS	= ft_strlen.c \
		  ft_strcmp.c \
		  ft_memset.c \
		  ft_bzero.c \
		  ft_memcpy.c \
		  ft_memcmp.c \
		  ft_strlcpy.c

OBJS	= $(SRCS:.c=.o)

GREEN	= \033[0;32m
CYAN	= \033[0;36m
YELLOW	= \033[0;33m
RED		= \033[0;31m
RESET	= \033[0m

all: $(NAME)

$(NAME): $(OBJS)
	@$(AR) $(NAME) $(OBJS)
	@printf "$(GREEN)✔ Archived $(NAME)$(RESET)\n"

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@printf "$(CYAN)  CC  $(RESET)$<\n"

clean:
	@rm -f $(OBJS)
	@printf "$(YELLOW)✔ Cleaned libft objects$(RESET)\n"

fclean: clean
	@rm -f $(NAME)
	@printf "$(RED)✔ Removed $(NAME)$(RESET)\n"

re: fclean all

.PHONY: all clean fclean re
