# ft_malcolm

An ARP spoofing tool written in C as part of the 42 school curriculum. The program listens for an ARP request from a target host and replies with a forged ARP response, poisoning the target's ARP cache with a spoofed MAC address.

## Table of Contents

- [Overview](#overview)
- [How It Works](#how-it-works)
- [Requirements](#requirements)
- [Building](#building)
- [Usage](#usage)
- [Options](#options)
- [Example](#example)
- [Testing](#testing)
- [Project Structure](#project-structure)
- [References](#references)

## Overview

ARP (Address Resolution Protocol) maps IP addresses to MAC addresses on a local network. Since ARP has no built-in authentication, any host can claim to own any IP address by sending a crafted ARP reply. **ft_malcolm** exploits this by:

1. Listening on the network for an ARP request from the **target** asking "Who has `<source_ip>`?"
2. Responding with a forged ARP reply: "`<source_ip>` is at `<spoofed_mac>`"
3. The target updates its ARP table with the spoofed entry and the program exits

This is one of the fundamental techniques behind Man-in-the-Middle (MITM) attacks at the Data Link Layer (OSI Layer 2).

## How It Works

```
Target                          ft_malcolm (Attacker)
  |                                    |
  |--- ARP Request (broadcast) ------->|  "Who has <source_ip>?"
  |                                    |
  |<-- Forged ARP Reply ---------------|  "<source_ip> is at <spoofed_mac>"
  |                                    |
  [ARP table poisoned]                 [Exit]
```

The program operates at the raw socket level, constructing Ethernet frames and ARP packets manually. It uses `AF_PACKET` / `SOCK_RAW` to send and receive frames directly on the network interface.

## Requirements

- **Linux** (kernel >= 3.14) — raw packet sockets require `AF_PACKET` support
- **Root privileges** — required for raw socket operations
- **gcc** and **make** for building
- Two machines (or VMs) on the same local network for testing

## Building

```bash
make        # Build the project
make clean  # Remove object files
make fclean # Remove object files and the binary
make re     # Rebuild from scratch
```

The binary `ft_malcolm` will be created in the project root.

## Usage

```
sudo ./ft_malcolm [options] <source_ip> <source_mac> <target_ip> <target_mac>
```

| Argument       | Description                                           |
|----------------|-------------------------------------------------------|
| `source_ip`    | IP address to impersonate                             |
| `source_mac`   | MAC address to associate with the source IP (spoofed) |
| `target_ip`    | IP address of the victim host                         |
| `target_mac`   | MAC address of the victim host                        |

- **IP addresses** must be in standard IPv4 dotted-decimal notation (e.g. `192.168.1.10`)
- **MAC addresses** must be in colon-separated hexadecimal notation (e.g. `aa:bb:cc:dd:ee:ff`)

## Options

| Flag            | Description                                      |
|-----------------|--------------------------------------------------|
| `-v, --verbose` | Enable verbose output with detailed packet dumps |
| `-i <iface>`    | Specify the network interface to use             |

## Example

**On the attacker machine (VM1):**

```bash
# Spoof the ARP entry for 10.11.11.1 on the target 10.11.11.2
sudo ./ft_malcolm 10.11.11.1 aa:bb:cc:dd:ee:ff 10.11.11.2 08:00:27:xx:xx:xx
```

The program will print:
```
Found available interface: eth0
Waiting for ARP request...
```

**On the target machine (VM2):**

```bash
# Trigger an ARP request for 10.11.11.1
arping -I eth0 10.11.11.1
```

**Back on VM1**, `ft_malcolm` will detect the request and respond:
```
An ARP request has been broadcast.
    mac address of request: 08:00:27:xx:xx:xx
    IP address of request: 10.11.11.2
Now sending an ARP reply to the target address with spoofed source, please wait...
Sent an ARP reply packet, you may now check the arp table on the target.
Exiting program...
```

**Verify on VM2:**

```bash
arp -a
# Should show: 10.11.11.1 at aa:bb:cc:dd:ee:ff
```

## Testing

### Setup

1. Create two virtual machines on the same internal/bridged network
2. Install debugging tools: `tcpdump`, `wireshark`, `arping`
3. Note the IP and MAC addresses of both machines

### Useful Commands

```bash
# View ARP table
arp -a

# Send ARP request
arping -I eth0 <target_ip>

# Monitor ARP traffic
sudo tcpdump -i eth0 -n arp

# Flush ARP cache
sudo ip -s -s neigh flush all

# List network interfaces
ip addr show
```

### Test Cases

- Run without root — should display an error
- Wrong number of arguments — should display usage
- Invalid IP or MAC — should display a specific error message
- Ctrl+C during operation — should exit cleanly
- ARP requests from unrelated hosts — should be ignored

## Project Structure

```
ft_malcolm/
├── Makefile
├── includes/
│   └── ft_malcolm.h        # Main header: structs, prototypes, constants
├── libft/                   # Minimal utility library
│   ├── Makefile
│   ├── libft.h
│   └── *.c
└── srcs/
    ├── main.c               # Entry point, privilege check, orchestration
    ├── parsing.c            # Command-line argument parsing
    ├── validation.c         # IP and MAC address validation
    ├── network.c            # Interface discovery and raw socket setup
    ├── arp.c                # ARP request listening and reply sending
    ├── signal_handler.c     # SIGINT handler for graceful shutdown
    ├── utils.c              # Printing helpers (MAC, IP formatting)
    └── verbose.c            # Verbose packet dump output
```

## References

- [RFC 826 — An Ethernet Address Resolution Protocol](https://datatracker.ietf.org/doc/html/rfc826)
- [RFC 7042 — IANA Considerations and IETF Protocol and Documentation Usage for IEEE 802 Parameters](https://datatracker.ietf.org/doc/html/rfc7042)
- [Wikipedia — ARP spoofing](https://en.wikipedia.org/wiki/ARP_spoofing)
