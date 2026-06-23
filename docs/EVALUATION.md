# ft_malcolm — Evaluation Guide

Complete step-by-step guide to verify every requirement of the project.

## Prerequisites

You need **two machines** on the same local network. The easiest setup:

| Role | Description |
|---|---|
| **VM1 (Attacker)** | Debian / Linux (kernel > 3.14), where `ft_malcolm` runs |
| **VM2 (Target)** | Any Linux, the machine whose ARP table will be poisoned |

Both VMs must be on the same L2 network (use **bridged** or **internal** network in VirtualBox/VMware).

Install debugging tools on both:
```bash
sudo apt install arping tcpdump net-tools
```

Get network info on both VMs:
```bash
# On each VM, note the IP and MAC
ip addr show
# or
ifconfig
```

Example values used throughout this guide:

| Parameter | Value |
|---|---|
| VM1 (attacker) IP | `10.0.2.10` |
| VM1 (attacker) MAC | `08:00:27:aa:bb:cc` |
| VM2 (target) IP | `10.0.2.20` |
| VM2 (target) MAC | `08:00:27:dd:ee:ff` |
| Spoofed source IP | `10.0.2.10` |
| Spoofed source MAC | `de:ad:be:ef:00:01` |

---

## Part 1: General Guidelines

### 1.1 Makefile

```bash
# Clone and enter the repository
cd ft_malcolm

# Build the project
make

# Verify executable name
ls -la ft_malcolm
# Expected: executable file named "ft_malcolm"

# Verify rules
make clean      # removes .o files, keeps executable
make fclean     # removes .o files AND executable
make re         # full rebuild (fclean + all)

# Verify compilation flags (should use -Wall -Wextra -Werror)
# Temporarily add a warning (unused variable) to any .c file and run make:
# It MUST fail to compile. Remove the warning after testing.
```

### 1.2 Global Variables

```bash
# Check for global variables (should be at most 1)
nm ft_malcolm | grep ' [BbDd] ' | grep -v __
# Expected: only g_running (or similar single variable)
```

### 1.3 Allowed Functions

```bash
# List all external function calls
nm -u ft_malcolm | sort
# Each listed function must be one of:
#   sendto, recvfrom, socket, setsockopt, inet_pton, inet_ntop,
#   if_nametoindex, sleep, getuid, close, sigaction, signal,
#   inet_addr, gethostbyname, getaddrinfo, freeaddrinfo,
#   getifaddrs, freeifaddrs, htons, ntohs, strerror, gai_strerror,
#   printf/fprintf/sprintf/snprintf (printf family),
#   or standard C library internals (__libc_start_main, etc.)
# Bonus part may use additional functions if justified.
```

### 1.4 Error Handling — No Crashes

```bash
# All of these must NOT crash (no segfault, bus error, etc.)
sudo ./ft_malcolm
sudo ./ft_malcolm a
sudo ./ft_malcolm a b c
sudo ./ft_malcolm a b c d e f
sudo ./ft_malcolm "" "" "" ""
```

---

## Part 2: Mandatory — Argument Validation

### 2.1 Root Requirement

```bash
# Run WITHOUT sudo — must print error and exit
./ft_malcolm 10.0.2.10 aa:bb:cc:dd:ee:ff 10.0.2.20 08:00:27:dd:ee:ff
# Expected: "ft_malcolm: must be run as root"
```

### 2.2 Wrong Number of Arguments

```bash
sudo ./ft_malcolm
sudo ./ft_malcolm 10.0.2.10
sudo ./ft_malcolm 10.0.2.10 aa:bb:cc:dd:ee:ff
sudo ./ft_malcolm 10.0.2.10 aa:bb:cc:dd:ee:ff 10.0.2.20
# Expected: usage message for all of the above
```

### 2.3 Invalid IP Address

```bash
sudo ./ft_malcolm 10.11.11.1111 aa:bb:cc:dd:ee:ff 10.0.2.20 08:00:27:dd:ee:ff
# Expected: ft_malcolm: unknown host or invalid IP address: (10.11.11.1111).

sudo ./ft_malcolm 999.999.999.999 aa:bb:cc:dd:ee:ff 10.0.2.20 08:00:27:dd:ee:ff
# Expected: ft_malcolm: unknown host or invalid IP address: (999.999.999.999).

sudo ./ft_malcolm 10.0.2.10 aa:bb:cc:dd:ee:ff abc.def.ghi.jkl 08:00:27:dd:ee:ff
# Expected: ft_malcolm: unknown host or invalid IP address: (abc.def.ghi.jkl).
```

### 2.4 Invalid MAC Address

```bash
sudo ./ft_malcolm 10.0.2.10 aaa:bb:cc:dd:ee:ff 10.0.2.20 08:00:27:dd:ee:ff
# Expected: ft_malcolm: invalid mac address: (aaa:bb:cc:dd:ee:ff)

sudo ./ft_malcolm 10.0.2.10 aa:bb:cc:dd:ee 10.0.2.20 08:00:27:dd:ee:ff
# Expected: ft_malcolm: invalid mac address: (aa:bb:cc:dd:ee)

sudo ./ft_malcolm 10.0.2.10 zz:bb:cc:dd:ee:ff 10.0.2.20 08:00:27:dd:ee:ff
# Expected: ft_malcolm: invalid mac address: (zz:bb:cc:dd:ee:ff)

sudo ./ft_malcolm 10.0.2.10 aa-bb-cc-dd-ee-ff 10.0.2.20 08:00:27:dd:ee:ff
# Expected: ft_malcolm: invalid mac address: (aa-bb-cc-dd-ee-ff)
```

### 2.5 Valid Arguments

```bash
sudo ./ft_malcolm 10.0.2.10 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
# Expected: "Found available interface: <iface_name>"
# Then waits for ARP request (program does not exit immediately)
# Press Ctrl+C to stop
```

---

## Part 3: Mandatory — Core Functionality (ARP Spoofing)

This is the main test. You need both VMs running.

### 3.1 Setup Monitoring (Optional but Recommended)

On **VM1 (attacker)**, in a separate terminal:
```bash
sudo tcpdump -i eth0 -n arp
```

### 3.2 Run ft_malcolm on VM1

Replace values with your actual IPs and MACs:
```bash
# source_ip     = IP you want to impersonate (e.g. VM1's own IP)
# source_mac    = spoofed MAC (fake MAC you want the target to cache)
# target_ip     = VM2's IP
# target_mac    = VM2's real MAC

sudo ./ft_malcolm 10.0.2.10 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
```

Expected output so far:
```
Found available interface: eth0
```
Program is now waiting for the matching ARP request.

### 3.3 Trigger ARP Request from VM2

On **VM2 (target)**:
```bash
# First, clear VM2's ARP cache for the source IP
sudo ip neigh del 10.0.2.10 dev eth0 2>/dev/null

# Send an ARP request for the source IP
arping -c 1 -I eth0 10.0.2.10
```

### 3.4 Verify ft_malcolm Output on VM1

Expected output on VM1 after the ARP request is detected:
```
Found available interface: eth0
An ARP request has been broadcast.
    mac address of request: 08:00:27:dd:ee:ff
    IP address of request: 10.0.2.20
Now sending an ARP reply to the target address with spoofed source, please wait...
Sent an ARP reply packet, you may now check the arp table on the target.
Exiting program...
```

The program should exit automatically after sending one reply.

### 3.5 Verify ARP Table on VM2

On **VM2 (target)**:
```bash
arp -a
# or
ip neigh show
```

Look for the source IP entry. It should show the **spoofed MAC**:
```
10.0.2.10  ...  de:ad:be:ef:00:01  ...
```

If you see `de:ad:be:ef:00:01` instead of the real MAC — the ARP poisoning was successful.

### 3.6 Verify Filtering (Ignoring Irrelevant ARP)

On **VM1**:
```bash
sudo ./ft_malcolm 10.0.2.10 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
```

On **VM2** or a **third machine**, send ARP requests for a DIFFERENT IP:
```bash
arping -c 3 -I eth0 10.0.2.99
```

**ft_malcolm should NOT react** — it must ignore ARP requests that don't match the source IP.

Then send the correct request from VM2:
```bash
arping -c 1 -I eth0 10.0.2.10
```

Now ft_malcolm should respond and exit.

### 3.7 Ctrl+C Handling

```bash
sudo ./ft_malcolm 10.0.2.10 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
# While it waits, press Ctrl+C
# Expected: clean exit with "Exiting program..." message, no crash
```

---

## Part 4: Bonus — Decimal IPv4 Notation

```bash
# 168430090 in decimal = 10.11.11.10 in dotted notation
# (10 << 24) + (11 << 16) + (11 << 8) + 10 = 168430090

sudo ./ft_malcolm 168430090 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
# Expected: program starts normally, treats 168430090 as 10.11.11.10

# Verify with another decimal IP:
# 167772161 = 10.0.0.1
sudo ./ft_malcolm 167772161 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
# Expected: program starts normally
```

---

## Part 5: Bonus — Hostname Resolution

```bash
# Use a hostname instead of an IP address
sudo ./ft_malcolm localhost de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
# Expected: resolves "localhost" to 127.0.0.1, program starts

# Use the target's hostname if it's resolvable
sudo ./ft_malcolm 10.0.2.10 de:ad:be:ef:00:01 $(hostname) 08:00:27:dd:ee:ff
# Expected: resolves hostname to IP, program starts

# Invalid hostname
sudo ./ft_malcolm 10.0.2.10 de:ad:be:ef:00:01 nonexistent.invalid.host 08:00:27:dd:ee:ff
# Expected: ft_malcolm: unknown host or invalid IP address: (nonexistent.invalid.host).
```

---

## Part 6: Bonus — Verbose Mode

```bash
sudo ./ft_malcolm -v 10.0.2.10 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
```

Then trigger the ARP request from VM2. Expected: detailed output for each received packet including:
- Ethernet header info (src/dst MAC, ethertype)
- ARP header info (opcode, hw type, proto type)
- Sender/target MAC and IP
- Full hex dump of the packet

Both `--verbose` and `-v` should work.

---

## Part 7: Bonus — Additional Features

### 7.1 Continuous Mode (`-c` / `--continuous`)

```bash
sudo ./ft_malcolm -c 10.0.2.10 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
```

Send multiple ARP requests from VM2:
```bash
arping -c 5 -I eth0 10.0.2.10
```

Expected: ft_malcolm responds to **each** matching ARP request (does NOT exit after the first one). Use Ctrl+C to stop.

### 7.2 Gratuitous ARP (`-g` / `--gratuitous`)

```bash
sudo ./ft_malcolm -g 10.0.2.10 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
```

Expected: sends a gratuitous ARP immediately (broadcast, no waiting) and exits. Check the ARP table on VM2 to verify the entry was updated.

### 7.3 Interface Selection (`-i`)

```bash
# List interfaces
ip link show

# Specify an interface explicitly
sudo ./ft_malcolm -i eth0 10.0.2.10 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
# Expected: "Found available interface: eth0"

# Invalid interface
sudo ./ft_malcolm -i nonexistent0 10.0.2.10 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
# Expected: error about interface not found

# Missing argument for -i
sudo ./ft_malcolm -i
# Expected: error "ft_malcolm: -i requires an argument"
```

### 7.4 Combined Flags

```bash
sudo ./ft_malcolm -v -c -i eth0 10.0.2.10 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
# Expected: verbose + continuous mode on interface eth0
```
