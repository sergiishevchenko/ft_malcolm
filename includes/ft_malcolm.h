#ifndef FT_MALCOLM_H
# define FT_MALCOLM_H

# include "libft.h" // IWYU pragma: keep
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <signal.h>
# include <errno.h>
# include <string.h>
# include <stdint.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <arpa/inet.h>
# include <net/if.h>
# include <ifaddrs.h>
# ifdef __linux__
#  include <netpacket/packet.h>
# else
#  define AF_PACKET 17
struct sockaddr_ll
{
	unsigned short	sll_family;
	unsigned short	sll_protocol;
	int				sll_ifindex;
	unsigned short	sll_hatype;
	unsigned char	sll_pkttype;
	unsigned char	sll_halen;
	unsigned char	sll_addr[8];
};
# endif
# ifndef SO_BINDTODEVICE
#  define SO_BINDTODEVICE 25
# endif
# include <netinet/in.h>
# include <netdb.h>

# define PROGRAM_NAME	"ft_malcolm"
# define MAC_LEN		6
# define IPV4_LEN		4
# define ARP_HW_ETHER	1
# define ARP_PROTO_IPV4	0x0800
# define ARP_OP_REQUEST	1
# define ARP_OP_REPLY	2

# ifndef ETH_P_ARP
#  define ETH_P_ARP		0x0806
# endif

typedef struct __attribute__((packed)) s_eth_hdr
{
	uint8_t		dest[MAC_LEN];
	uint8_t		src[MAC_LEN];
	uint16_t	ethertype;
}				t_eth_hdr;

typedef struct __attribute__((packed)) s_arp_hdr
{
	uint16_t	hw_type;
	uint16_t	proto_type;
	uint8_t		hw_len;
	uint8_t		proto_len;
	uint16_t	opcode;
	uint8_t		sender_mac[MAC_LEN];
	uint8_t		sender_ip[IPV4_LEN];
	uint8_t		target_mac[MAC_LEN];
	uint8_t		target_ip[IPV4_LEN];
}				t_arp_hdr;

typedef struct __attribute__((packed)) s_arp_packet
{
	t_eth_hdr	eth;
	t_arp_hdr	arp;
}				t_arp_packet;

typedef struct s_malcolm
{
	uint8_t		source_ip[IPV4_LEN];
	uint8_t		source_mac[MAC_LEN];
	uint8_t		target_ip[IPV4_LEN];
	uint8_t		target_mac[MAC_LEN];
	char		iface_name[IFNAMSIZ];
	uint8_t		iface_mac[MAC_LEN];
	int			iface_index;
	int			sockfd;
	int			verbose;
	int			continuous;
	int			gratuitous;
	int			iface_set;
}				t_malcolm;

extern volatile sig_atomic_t	g_running;

int		parse_args(t_malcolm *ctx, int argc, char **argv);
int		validate_ip(const char *ip_str, uint8_t *ip_out);
int		validate_mac(const char *mac_str, uint8_t *mac_out);
int		find_interface(t_malcolm *ctx);
int		open_raw_socket(t_malcolm *ctx);
int		listen_arp_request(t_malcolm *ctx);
int		send_arp_reply(t_malcolm *ctx);
int		send_gratuitous_arp(t_malcolm *ctx);
void	setup_signals(void);
void	print_mac(const uint8_t *mac);
void	print_ip(const uint8_t *ip);
void	print_verbose_pkt(t_arp_packet *pkt, size_t len, int out);
void	print_hex_dump(const unsigned char *data, size_t len);

#endif
