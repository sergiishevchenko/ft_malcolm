#include "ft_malcolm.h"

static int	is_all_digits(const char *s)
{
	if (!*s)
		return (0);
	while (*s)
	{
		if (*s < '0' || *s > '9')
			return (0);
		s++;
	}
	return (1);
}

static int	parse_decimal_ip(const char *str, uint8_t *ip_out)
{
	unsigned long	val;

	val = 0;
	while (*str)
	{
		val = val * 10 + (*str - '0');
		if (val > 0xFFFFFFFF)
			return (-1);
		str++;
	}
	ip_out[0] = (val >> 24) & 0xFF;
	ip_out[1] = (val >> 16) & 0xFF;
	ip_out[2] = (val >> 8) & 0xFF;
	ip_out[3] = val & 0xFF;
	return (0);
}

static int	resolve_hostname(const char *host, uint8_t *ip_out)
{
	struct addrinfo		hints;
	struct addrinfo		*res;
	struct sockaddr_in	*addr;

	ft_bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	if (getaddrinfo(host, NULL, &hints, &res) != 0)
		return (-1);
	addr = (struct sockaddr_in *)res->ai_addr;
	ft_memcpy(ip_out, &addr->sin_addr.s_addr, 4);
	freeaddrinfo(res);
	return (0);
}

int	validate_ip(const char *ip_str, uint8_t *ip_out)
{
	struct in_addr	addr;

	if (inet_pton(AF_INET, ip_str, &addr) == 1)
	{
		ft_memcpy(ip_out, &addr.s_addr, 4);
		return (0);
	}
	if (is_all_digits(ip_str))
		return (parse_decimal_ip(ip_str, ip_out));
	return (resolve_hostname(ip_str, ip_out));
}
