#include "ft_malcolm.h"

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
