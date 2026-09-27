# ft_malcolm — Evaluation Guide

Complete step-by-step guide to verify every requirement of the project.

## Prerequisites

You need **two machines** on the same local network. The easiest setup:

| Role | Description |
|---|---|
| **VM1 (Attacker)** | Debian / Linux (kernel > 3.14), where `ft_malcolm` runs |
| **VM2 (Target)** | Any Linux, the machine whose ARP table will be poisoned |

Both VMs must be on the same L2 network (`192.168.65.0/24` here; interface `enp0s1` on both).

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

Lab values used throughout this guide:

| Parameter | Value |
|---|---|
| VM1 (attacker) interface | `enp0s1` |
| VM1 (attacker) IP | `192.168.65.14/24` |
| VM1 (attacker) MAC | `aa:77:fb:2e:e0:ed` |
| VM2 (target) interface | `enp0s1` |
| VM2 (target) IP | `192.168.65.15/24` |
| VM2 (target) MAC | `aa:77:fb:2e:e0:aa` |
| Spoofed source IP | `192.168.65.14` (attacker's own address) |
| Spoofed source MAC | `de:ad:be:ef:00:01` |

`source_ip` is the attacker's real address, so VM1's kernel also answers ARP for `192.168.65.14` with `aa:77:fb:2e:e0:ed`. A single spoofed reply can lose that race. Use `-c` when checking the neighbor table.

---

## Part 1: General Guidelines

### 1.1 Makefile

```bash
# Clone and enter the repository
cd ft_malcolm

# Build the project
make
make all      # must also work (explicit default target)

# Verify executable name
ls -la ft_malcolm
# Expected: executable file named "ft_malcolm"

# Verify no relink
make
# Expected: "make: Nothing to be done for 'all'." (or similar)
# Must NOT recompile or relink anything

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

# Unknown option
sudo ./ft_malcolm --unknown 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
# Expected: "ft_malcolm: unknown option: --unknown"
```

### 1.5 Unit Tests (optional)

```bash
make test
# Runs validation and parsing tests (no root required)
```

---

## Part 2: Mandatory — Argument Validation

### 2.1 Root Requirement

```bash
# Run WITHOUT sudo — must print error and exit
./ft_malcolm 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
# Expected: "ft_malcolm: must be run as root"
```

### 2.2 Wrong Number of Arguments

```bash
sudo ./ft_malcolm
sudo ./ft_malcolm 192.168.65.14
sudo ./ft_malcolm 192.168.65.14 de:ad:be:ef:00:01
sudo ./ft_malcolm 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15
# Expected: usage message for all of the above
```

### 2.3 Invalid IP Address

```bash
sudo ./ft_malcolm 10.11.11.1111 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
# Expected: ft_malcolm: unknown host or invalid IP address: (10.11.11.1111).

sudo ./ft_malcolm 999.999.999.999 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
# Expected: ft_malcolm: unknown host or invalid IP address: (999.999.999.999).

sudo ./ft_malcolm 192.168.65.14 de:ad:be:ef:00:01 abc.def.ghi.jkl aa:77:fb:2e:e0:aa
# Expected: ft_malcolm: unknown host or invalid IP address: (abc.def.ghi.jkl).
```

### 2.4 Invalid MAC Address

```bash
sudo ./ft_malcolm 192.168.65.14 aaa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
# Expected: ft_malcolm: invalid mac address: (aaa:bb:cc:dd:ee:ff)

sudo ./ft_malcolm 192.168.65.14 aa:bb:cc:dd:ee 192.168.65.15 aa:77:fb:2e:e0:aa
# Expected: ft_malcolm: invalid mac address: (aa:bb:cc:dd:ee)

sudo ./ft_malcolm 192.168.65.14 zz:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
# Expected: ft_malcolm: invalid mac address: (zz:bb:cc:dd:ee:ff)

sudo ./ft_malcolm 192.168.65.14 aa-bb-cc-dd-ee-ff 192.168.65.15 aa:77:fb:2e:e0:aa
# Expected: ft_malcolm: invalid mac address: (aa-bb-cc-dd-ee-ff)
```

### 2.5 Valid Arguments

```bash
sudo ./ft_malcolm 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
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
sudo tcpdump -i enp0s1 -n arp
```

### 3.2 Run ft_malcolm on VM1

```bash
# source_ip     = 192.168.65.14   (VM1's own IP, the address to impersonate)
# source_mac    = de:ad:be:ef:00:01   (MAC the target should cache)
# target_ip     = 192.168.65.15   (VM2)
# target_mac    = aa:77:fb:2e:e0:aa   (VM2's real MAC)

sudo ./ft_malcolm 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
```

Expected output so far:
```
Found available interface: enp0s1
```
Program is now waiting for the matching ARP request.

### 3.3 Trigger ARP Request from VM2

On **VM2 (target)**:
```bash
arping -c 1 -I enp0s1 192.168.65.14
```

`arping` is enough to wake `ft_malcolm`. It does not create a kernel neighbor entry, so `ip neigh` on VM2 stays empty after this step. The cache check is in 3.5.

### 3.4 Verify ft_malcolm Output on VM1

Expected output on VM1 after the ARP request is detected:
```
Found available interface: enp0s1
An ARP request has been broadcast.
mac address of request: aa:77:fb:2e:e0:aa
IP address of request: 192.168.65.15
Now sending an ARP reply to the target address with spoofed source, please wait...
Sent an ARP reply packet, you may now check the arp table on the target.
Exiting program...
```

The program should exit automatically after sending one reply.

### 3.5 Verify ARP Table on VM2

An empty `ip neigh show 192.168.65.14` on VM2 after `arping` is expected: `arping` talks to `ft_malcolm` from userspace, and the kernel does not install a neighbor from that reply (`arp_accept` is 0).

The kernel creates the entry only when it asks itself. On **VM1**, leave the program running in continuous mode (VM1's own kernel also answers for `192.168.65.14` with `aa:77:fb:2e:e0:ed`, so one reply is not enough):

```bash
sudo ./ft_malcolm -c 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
```

On **VM2**, while that process is still up:

```bash
ping -c 1 -W 1 192.168.65.14
ip neigh show 192.168.65.14
```

Expected on VM2:

```
192.168.65.14 dev enp0s1 lladdr de:ad:be:ef:00:01 REACHABLE
```

`aa:77:fb:2e:e0:ed` in that line means VM1's kernel answered last. Ping again while `-c` is still running.

### 3.6 Verify Filtering (Ignoring Irrelevant ARP)

On **VM1**:
```bash
sudo ./ft_malcolm 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
```

On **VM2** or a **third machine**, send ARP requests for a DIFFERENT IP:
```bash
arping -c 3 -I enp0s1 192.168.65.99
```

**ft_malcolm should NOT react** — it must ignore ARP requests that don't match the source IP.

Then send the correct request from VM2:
```bash
arping -c 1 -I enp0s1 192.168.65.14
```

Now ft_malcolm should respond and exit.

### 3.7 Ctrl+C and SIGTERM Handling

```bash
sudo ./ft_malcolm 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
# While it waits, press Ctrl+C
# Expected: clean exit with "Exiting program..." message, no crash
```

```bash
# In another terminal, send SIGTERM to the running process
sudo ./ft_malcolm 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa &
kill $!
# Expected: same clean exit with "Exiting program..." message
```

---

## Part 4: Bonus — Decimal IPv4 Notation

```bash
# 3232252174 = 192.168.65.14
# (192 << 24) + (168 << 16) + (65 << 8) + 14 = 3232252174
sudo ./ft_malcolm 3232252174 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
# Expected: program starts normally, treats 3232252174 as 192.168.65.14

# 3232252175 = 192.168.65.15
sudo ./ft_malcolm 192.168.65.14 de:ad:be:ef:00:01 3232252175 aa:77:fb:2e:e0:aa
# Expected: program starts normally, treats 3232252175 as 192.168.65.15
```

---

## Part 5: Bonus — Hostname Resolution

```bash
# Use a hostname instead of an IP address
sudo ./ft_malcolm localhost de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
# Expected: resolves "localhost" to 127.0.0.1, program starts

# Use the target's hostname if it's resolvable
sudo ./ft_malcolm 192.168.65.14 de:ad:be:ef:00:01 $(hostname) aa:77:fb:2e:e0:aa
# Expected: resolves hostname to IP, program starts

# Invalid hostname
sudo ./ft_malcolm 192.168.65.14 de:ad:be:ef:00:01 nonexistent.invalid.host aa:77:fb:2e:e0:aa
# Expected: ft_malcolm: unknown host or invalid IP address: (nonexistent.invalid.host).
```

---

## Part 6: Bonus — Verbose Mode

```bash
sudo ./ft_malcolm -v 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
```

Then trigger the ARP request from VM2. Expected: detailed output for each received packet including:
- Ethernet header info (src/dst MAC, ethertype)
- ARP header info (opcode, hw type, proto type)
- Sender/target MAC and IP
- Full hex dump of the packet

Long form must also work:

```bash
sudo ./ft_malcolm --verbose 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
```

---

## Part 7: Bonus — Additional Features

### 7.1 Continuous Mode (`-c` / `--continuous`)

```bash
sudo ./ft_malcolm -c 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
```

Send multiple ARP requests from VM2:
```bash
arping -c 5 -I enp0s1 192.168.65.14
```

Expected: ft_malcolm responds to **each** matching ARP request (does NOT exit after the first one). Use Ctrl+C to stop.

Long form:

```bash
sudo ./ft_malcolm --continuous 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
```

### 7.2 Gratuitous ARP (`-g` / `--gratuitous`)

```bash
sudo ./ft_malcolm -g 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
```

Expected: sends a gratuitous ARP immediately (broadcast, no waiting) and exits. On VM2 the kernel still ignores that broadcast unless an entry for `192.168.65.14` already exists. Confirm the cache with the `ping` check from section 3.5.

Long form:

```bash
sudo ./ft_malcolm --gratuitous 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
```

### 7.3 Interface Selection (`-i`)

```bash
# List interfaces
ip link show

# Specify an interface explicitly
sudo ./ft_malcolm -i enp0s1 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
# Expected: "Found available interface: enp0s1"

# Invalid interface
sudo ./ft_malcolm -i nonexistent0 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
# Expected: error about interface not found

# Missing argument for -i
sudo ./ft_malcolm -i
# Expected: error "ft_malcolm: -i requires an argument"
```

### 7.4 Combined Flags

```bash
sudo ./ft_malcolm -v -c -i enp0s1 192.168.65.14 de:ad:be:ef:00:01 192.168.65.15 aa:77:fb:2e:e0:aa
# Expected: verbose + continuous mode on interface enp0s1
```
