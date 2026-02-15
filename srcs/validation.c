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

static int	is_hex_char(char c)
{
	return ((c >= '0' && c <= '9')
		|| (c >= 'a' && c <= 'f')
		|| (c >= 'A' && c <= 'F'));
}

static int	hex_to_val(char c)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	if (c >= 'a' && c <= 'f')
		return (c - 'a' + 10);
	if (c >= 'A' && c <= 'F')
		return (c - 'A' + 10);
	return (-1);
}

static int	check_mac_format(const char *mac_str)
{
	int	i;

	if (ft_strlen(mac_str) != 17)
		return (-1);
	i = 0;
	while (i < 17)
	{
		if (i % 3 == 2)
		{
			if (mac_str[i] != ':')
				return (-1);
		}
		else
		{
			if (!is_hex_char(mac_str[i]))
				return (-1);
		}
		i++;
	}
	return (0);
}

int	validate_mac(const char *mac_str, uint8_t *mac_out)
{
	int	i;
	int	byte_idx;

	if (check_mac_format(mac_str) != 0)
		return (-1);
	i = 0;
	byte_idx = 0;
	while (byte_idx < 6)
	{
		mac_out[byte_idx] = (hex_to_val(mac_str[i]) << 4)
			| hex_to_val(mac_str[i + 1]);
		byte_idx++;
		i += 3;
	}
	return (0);
}
