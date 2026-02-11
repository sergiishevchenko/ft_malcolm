#ifndef LIBFT_H
# define LIBFT_H

# include <stddef.h>

size_t	ft_strlen(const char *s);
int		ft_strcmp(const char *s1, const char *s2);
void	*ft_memset(void *s, int c, size_t n);
void	ft_bzero(void *s, size_t n);
void	*ft_memcpy(void *dest, const void *src, size_t n);
int		ft_memcmp(const void *s1, const void *s2, size_t n);
size_t	ft_strlcpy(char *dst, const char *src, size_t size);

#endif
