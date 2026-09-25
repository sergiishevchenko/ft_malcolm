# ft_malcolm

An ARP spoofing tool written in C as part of the 42 school curriculum. The program listens for an ARP request from a target host and replies with a forged ARP response, poisoning the target's ARP cache with a spoofed MAC address.

## Table of Contents

- [Overview](#overview)
- [Project Structure](#project-structure)
- [How It Works](#how-it-works)
- [Requirements](#requirements)
- [Building](#building)
- [Usage](#usage)
- [Options](#options)
- [Examples](#examples)
- [Bonus Features](#bonus-features)
- [Testing](#testing)
- [References](#references)

## Overview

ARP (Address Resolution Protocol) maps IP addresses to MAC addresses on a local network. Since ARP has no built-in authentication, any host can claim to own any IP address by sending a crafted ARP reply. **ft_malcolm** exploits this by:

1. Listening on the network for an ARP request from the **target** asking "Who has `<source_ip>`?"
2. Responding with a forged ARP reply: "`<source_ip>` is at `<spoofed_mac>`"
3. The target updates its ARP table with the spoofed entry

This is one of the fundamental techniques behind Man-in-the-Middle (MITM) attacks at the Data Link Layer (OSI Layer 2).

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
├── srcs/
│   ├── main.c               # Entry point, privilege check, orchestration
│   ├── parsing.c            # Option and positional argument parsing
│   ├── validate_ip.c        # IP: dotted / decimal / hostname
│   ├── validate_mac.c       # MAC XX:XX:XX:XX:XX:XX
│   ├── network.c            # Interface discovery and raw socket setup
│   ├── arp_listen.c         # ARP request filter and listener
│   ├── arp_send.c           # Forged reply and gratuitous ARP
│   ├── signal_handler.c     # SIGINT/SIGTERM handler for graceful shutdown
│   ├── utils.c              # MAC and IP formatting helpers
│   └── verbose.c            # Verbose packet dump and hex output
├── tests/                   # Unit tests for validation and parsing
└── docs/
    ├── ru/
    │   ├── ARP.md           # ARP protocol (RU)
    │   ├── MAC_IP.md        # MAC and IPv4 addresses (RU)
    │   ├── L2.md            # Layer 2 segment (RU)
    │   └── HOW_IT_WORKS.md  # How the program works (RU)
    ├── ARCHITECTURE.md
    ├── EVALUATION.md
    └── TESTING.md
```

## How It Works

```
Target                          ft_malcolm (Attacker)
  |                                    |
  |--- ARP Request (broadcast) ------->|  "Who has <source_ip>?"
  |                                    |
  |<-- Forged ARP Reply ---------------|  "<source_ip> is at <spoofed_mac>"
  |                                    |
  [ARP table poisoned]                 [Exit or wait for next request]
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
sudo ./ft_malcolm [-v] [-c] [-g] [-i interface] <source_ip> <source_mac> <target_ip> <target_mac>
```

| Argument       | Description                                           |
|----------------|-------------------------------------------------------|
| `source_ip`    | IP address to impersonate (dotted, decimal, or hostname) |
| `source_mac`   | MAC address to associate with the source IP (spoofed) |
| `target_ip`    | IP address of the victim host (dotted, decimal, or hostname) |
| `target_mac`   | MAC address of the victim host                        |

- **IP addresses**: standard IPv4 dotted-decimal (`192.168.1.10`), decimal notation (`3232235786`), or resolvable hostname (`myhost`)
- **MAC addresses**: colon-separated hexadecimal notation (`aa:bb:cc:dd:ee:ff`)

## Options

| Flag                     | Description                                                    |
|--------------------------|----------------------------------------------------------------|
| `-v`, `--verbose`        | Enable verbose output with detailed packet dumps and hex dump  |
| `-c`, `--continuous`     | Keep running and respond to every matching ARP request          |
| `-g`, `--gratuitous`     | Send a gratuitous ARP broadcast immediately and exit            |
| `-i <iface>`             | Specify the network interface to use (auto-detected if omitted) |

## Examples

### Basic ARP Spoofing

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

### Continuous Mode

Respond to every matching ARP request instead of exiting after the first one:

```bash
sudo ./ft_malcolm -c 10.11.11.1 aa:bb:cc:dd:ee:ff 10.11.11.2 08:00:27:xx:xx:xx
# Press Ctrl+C to stop
```

### Gratuitous ARP

Send a gratuitous ARP broadcast immediately without waiting for a request:

```bash
sudo ./ft_malcolm -g 10.11.11.1 aa:bb:cc:dd:ee:ff 10.11.11.2 08:00:27:xx:xx:xx
```

```
Sending gratuitous ARP for 10.11.11.1 with mac aa:bb:cc:dd:ee:ff...
Gratuitous ARP sent.
Exiting program...
```

### Combined Flags

```bash
sudo ./ft_malcolm -v -c -i eth0 10.11.11.1 aa:bb:cc:dd:ee:ff 10.11.11.2 08:00:27:xx:xx:xx
```

## Bonus Features

### Decimal IPv4 Notation

IP addresses can be specified as a single decimal number instead of dotted-decimal notation:

```bash
# 168430090 in decimal = 10.11.11.10 in dotted notation
sudo ./ft_malcolm 168430090 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
```

### Hostname Resolution

Hostnames are resolved to IPv4 addresses via `getaddrinfo`:

```bash
sudo ./ft_malcolm localhost de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
# Resolves "localhost" to 127.0.0.1

sudo ./ft_malcolm 10.0.2.10 de:ad:be:ef:00:01 myhost 08:00:27:dd:ee:ff
# Resolves "myhost" via DNS/hosts
```

### Verbose Mode

With `-v`, every received/sent ARP packet is printed with full details:
- Ethernet header (src/dst MAC, EtherType)
- ARP header (opcode, hardware type, protocol type)
- Sender and target MAC/IP addresses
- Full hex dump of the raw packet

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
- Invalid hostname — should display "unknown host or invalid IP address"
- Unknown option — should display "unknown option" error
- Missing `-i` argument — should display "-i requires an argument"
- Ctrl+C during operation — should exit cleanly
- ARP requests from unrelated hosts — should be ignored

## References

- [RFC 826 — An Ethernet Address Resolution Protocol](https://datatracker.ietf.org/doc/html/rfc826)
- [RFC 7042 — IANA Considerations and IETF Protocol and Documentation Usage for IEEE 802 Parameters](https://datatracker.ietf.org/doc/html/rfc7042)
- [RFC 5227 — IPv4 Address Conflict Detection](https://datatracker.ietf.org/doc/html/rfc5227) (gratuitous ARP)
- [Wikipedia — ARP spoofing](https://en.wikipedia.org/wiki/ARP_spoofing)
