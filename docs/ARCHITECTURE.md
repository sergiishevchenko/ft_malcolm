# ft_malcolm — Architecture & Program Flow

A detailed walkthrough of how the program works internally: every stage from launch to exit, the data structures involved, and what each source file is responsible for.

---

## High-Level Execution Flow

```
main()
  │
  ├─ 1. Root check                    (getuid)
  ├─ 2. Parse arguments               (parsing.c)
  │      ├─ Parse option flags         (-v, -c, -g, -i)
  │      └─ Validate positional args   (validation.c)
  │           ├─ Validate/resolve IPs  (inet_pton / decimal / getaddrinfo)
  │           └─ Validate MACs         (format check + hex parse)
  ├─ 3. Install signal handlers        (signal_handler.c)
  ├─ 4. Find network interface         (network.c)
  │      ├─ Auto-detect or use -i
  │      └─ Retrieve MAC + ifindex
  ├─ 5. Open raw socket                (network.c)
  │      ├─ socket(AF_PACKET, SOCK_RAW, ETH_P_ARP)
  │      └─ setsockopt(SO_BINDTODEVICE)
  │
  ├─ [if --gratuitous]
  │      └─ 6a. Send gratuitous ARP    (arp.c) → exit
  │
  └─ [default / --continuous]
         └─ 6b. Loop:
                ├─ Listen for ARP request   (arp.c)
                │    ├─ recvfrom() raw frames
                │    ├─ Filter: broadcast + REQUEST + matching IPs
                │    └─ [if -v] print verbose packet dump
                ├─ Send forged ARP reply    (arp.c)
                │    ├─ Build Ethernet + ARP headers
                │    ├─ [if -v] print verbose packet dump
                │    └─ sendto() on raw socket
                └─ [if not --continuous] break
```

---

## Data Structures

### `t_malcolm` — Program Context

Defined in `ft_malcolm.h`. A single instance is created in `main()` and threaded through every function. It holds all parsed input and runtime state.

```
t_malcolm
├── source_ip[4]        IP to impersonate (network byte order)
├── source_mac[6]       Spoofed MAC to advertise
├── target_ip[4]        Victim's IP (network byte order)
├── target_mac[6]       Victim's real MAC
├── iface_name[16]      Network interface name (e.g. "eth0")
├── iface_mac[6]        Interface's own MAC (retrieved at runtime)
├── iface_index          Interface index (for sockaddr_ll)
├── sockfd               Raw socket file descriptor
├── verbose              Flag: -v / --verbose
├── continuous           Flag: -c / --continuous
├── gratuitous           Flag: -g / --gratuitous
└── iface_set            Flag: whether -i was explicitly provided
```

### Packet Structures (packed, match wire format)

```
t_arp_packet (42 bytes)
├── t_eth_hdr (14 bytes)          Ethernet II frame header
│   ├── dest[6]                   Destination MAC
│   ├── src[6]                    Source MAC
│   └── ethertype (uint16)        0x0806 for ARP
└── t_arp_hdr (28 bytes)          ARP payload
    ├── hw_type (uint16)          1 = Ethernet
    ├── proto_type (uint16)       0x0800 = IPv4
    ├── hw_len (uint8)            6
    ├── proto_len (uint8)         4
    ├── opcode (uint16)           1 = REQUEST, 2 = REPLY
    ├── sender_mac[6]
    ├── sender_ip[4]
    ├── target_mac[6]
    └── target_ip[4]
```

All multi-byte fields are stored in network byte order (big-endian) and converted with `htons()`/`ntohs()`.

### `g_running` — Global Signal Flag

```c
volatile sig_atomic_t g_running = 1;
```

The only global variable. Set to `0` by the signal handler to make the main loop (`recvfrom`) exit cleanly.

---

## Step-by-Step Breakdown

### Step 1 — Root Privilege Check

**File:** `main.c`

```
getuid() != 0  →  "must be run as root"  →  exit(1)
```

Raw sockets (`AF_PACKET` + `SOCK_RAW`) require `CAP_NET_RAW` or root. The program checks this upfront to give a clear error before doing anything else.

### Step 2 — Argument Parsing

**File:** `parsing.c`, `validation.c`

Parsing happens in two phases:

#### Phase 1: Option flags (`parse_options`)

Scans `argv` from index 1 while arguments start with `-`. Each recognized flag sets a field in `t_malcolm`:

| Flag | Field Set | Effect |
|---|---|---|
| `-v` / `--verbose` | `ctx->verbose = 1` | Print every ARP packet with hex dump |
| `-c` / `--continuous` | `ctx->continuous = 1` | Don't exit after first reply |
| `-g` / `--gratuitous` | `ctx->gratuitous = 1` | Send gratuitous ARP instead of listening |
| `-i <name>` | `ctx->iface_name`, `ctx->iface_set = 1` | Use specific interface |

Returns the index where positional arguments start. Returns `-1` on error (unknown option, missing `-i` argument).

#### Phase 2: Positional arguments (`parse_positional`)

Expects exactly 4 remaining arguments: `source_ip`, `source_mac`, `target_ip`, `target_mac`.

**IP validation** (`validate_ip`) tries three strategies in order:

```
1. inet_pton(AF_INET, str)    →  standard dotted-decimal ("10.0.2.1")
2. is_all_digits(str)          →  decimal notation ("167772161" → 10.0.0.1)
3. getaddrinfo(str, AF_INET)   →  hostname resolution ("myhost" → DNS lookup)
```

All three paths write the result as 4 raw bytes into `ip_out[]` in network byte order.

**MAC validation** (`validate_mac`) enforces strict format:
- Exactly 17 characters
- Pattern: `XX:XX:XX:XX:XX:XX` where `X` is a hex digit
- Colons at positions 2, 5, 8, 11, 14
- Each hex pair is converted to a byte and stored in `mac_out[]`

### Step 3 — Signal Handler Setup

**File:** `signal_handler.c`

Installs a handler for `SIGINT` (Ctrl+C) and `SIGTERM` using `sigaction()`. The handler simply sets `g_running = 0`.

This causes the blocking `recvfrom()` in the main loop to return with `errno == EINTR`. The loop then checks `g_running`, sees it is `0`, and exits cleanly.

Key decisions:
- `sa_flags = 0` (no `SA_RESTART`) — ensures `recvfrom` is interrupted, not restarted
- `sig_atomic_t` — guarantees atomic writes from signal context
- No `printf` or allocation in the handler — only an assignment to the global flag

### Step 4 — Network Interface Discovery

**File:** `network.c` → `find_interface()`

Uses `getifaddrs()` to enumerate all network interfaces.

**If `-i` was not provided** (`auto_detect_iface`):
- Walks the linked list of interfaces
- Picks the first one that is: IPv4 (`AF_INET`), not loopback (`!IFF_LOOPBACK`), and up (`IFF_UP`)
- Copies its name into `ctx->iface_name`

**Then** (`get_iface_mac`):
- Walks the list again looking for the chosen interface name with `sa_family == AF_PACKET`
- Extracts the MAC address and interface index from `sockaddr_ll`
- Stores them in `ctx->iface_mac` and `ctx->iface_index`

Both values are needed later: `iface_index` for `sockaddr_ll` when sending, `iface_mac` is available for verbose output.

### Step 5 — Raw Socket Creation

**File:** `network.c` → `open_raw_socket()`

```c
socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP))
```

- `AF_PACKET` — operates at L2 (Data Link layer), giving access to raw Ethernet frames
- `SOCK_RAW` — includes the Ethernet header in received/sent data
- `ETH_P_ARP` (0x0806) — only receive frames with ARP EtherType

Then binds the socket to the chosen interface:

```c
setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, iface_name, ...)
```

This ensures the socket only receives frames from (and sends frames on) the selected interface.

### Step 6a — Gratuitous ARP Path

**File:** `arp.c` → `send_gratuitous_arp()`

If `-g` was specified, the program skips the listen loop entirely and sends a single unsolicited ARP reply to the broadcast address:

```
Ethernet header:
  dest  = ff:ff:ff:ff:ff:ff   (broadcast)
  src   = <source_mac>         (spoofed)
  type  = 0x0806               (ARP)

ARP header:
  opcode     = REPLY (2)
  sender_mac = <source_mac>    (spoofed)
  sender_ip  = <source_ip>     (IP to claim)
  target_mac = ff:ff:ff:ff:ff:ff
  target_ip  = <source_ip>     (same as sender — gratuitous)
```

Every host on the network segment receives this broadcast. Hosts that already have an ARP entry for `source_ip` will update it with the spoofed MAC. The program then closes the socket and exits.

### Step 6b — Listen & Reply Loop

This is the default (non-gratuitous) path. The loop runs while `g_running == 1`.

#### Listening (`listen_arp_request`)

**File:** `arp.c`

Calls `recvfrom()` in a loop, reading raw Ethernet frames from the socket. For each frame:

1. **Size check**: frame must be at least `sizeof(t_arp_packet)` (42 bytes)
2. **Verbose dump**: if `-v`, prints full packet details via `print_verbose_pkt()`
3. **Match filter** (`is_matching_request`): all four conditions must be true:

| Check | Field | Expected Value |
|---|---|---|
| Broadcast | `eth.dest` | `ff:ff:ff:ff:ff:ff` |
| ARP request | `arp.opcode` | `1` (REQUEST) |
| Asking for our source | `arp.target_ip` | `ctx->source_ip` |
| Sent by our target | `arp.sender_ip` | `ctx->target_ip` |

Non-matching packets are silently skipped. When a match is found, the sender's MAC and IP are printed and the function returns `0`.

#### Sending the Reply (`send_arp_reply`)

**File:** `arp.c`

Constructs a forged ARP reply packet (`build_arp_reply`):

```
Ethernet header:
  dest  = <target_mac>        (unicast to victim)
  src   = <source_mac>        (spoofed)
  type  = 0x0806

ARP header:
  opcode     = REPLY (2)
  sender_mac = <source_mac>   (spoofed — this is what poisons the cache)
  sender_ip  = <source_ip>    (IP being impersonated)
  target_mac = <target_mac>   (victim)
  target_ip  = <target_ip>    (victim)
```

Sends it via `sendto()` with a `sockaddr_ll` destination:

```c
sockaddr_ll {
    sll_family  = AF_PACKET
    sll_ifindex = ctx->iface_index
    sll_halen   = 6
    sll_addr    = <target_mac>
}
```

The victim receives this reply and updates its ARP cache: it now believes `source_ip` has MAC address `source_mac` (the spoofed one).

#### Loop Control

After sending, the loop checks `ctx->continuous`:
- **Not set** (default): `break` — program prints "Exiting program..." and exits
- **Set** (`-c`): loops back to `listen_arp_request()`, waits for the next matching request

In continuous mode the only way to exit is via `SIGINT`/`SIGTERM`.

---

## Source File Map

| File | Role | Key Functions |
|---|---|---|
| `main.c` | Entry point, orchestration, cleanup | `main()` |
| `parsing.c` | CLI argument parsing | `parse_args()` → `parse_options()` + `parse_positional()` |
| `validation.c` | IP and MAC format validation | `validate_ip()`, `validate_mac()` |
| `network.c` | Interface discovery, raw socket | `find_interface()`, `open_raw_socket()` |
| `arp.c` | ARP listen, reply, gratuitous send | `listen_arp_request()`, `send_arp_reply()`, `send_gratuitous_arp()` |
| `signal_handler.c` | SIGINT/SIGTERM handling | `setup_signals()` |
| `utils.c` | MAC/IP formatting | `print_mac()`, `print_ip()` |
| `verbose.c` | Detailed packet + hex dump output | `print_verbose_pkt()`, `print_hex_dump()` |
| `ft_malcolm.h` | Structs, constants, prototypes | `t_malcolm`, `t_arp_packet`, all function declarations |

---

## Packet Layout on the Wire

For reference, this is the exact byte layout of an ARP-over-Ethernet frame as constructed by the program:

```
Offset  Size  Field
──────  ────  ─────────────────────────
 0       6    Ethernet destination MAC
 6       6    Ethernet source MAC
12       2    EtherType (0x0806)
──────────────────────────────────────── ← end of Ethernet header (14 bytes)
14       2    Hardware type (0x0001 = Ethernet)
16       2    Protocol type (0x0800 = IPv4)
18       1    Hardware address length (6)
19       1    Protocol address length (4)
20       2    Opcode (1 = Request, 2 = Reply)
22       6    Sender hardware address (MAC)
28       4    Sender protocol address (IP)
32       6    Target hardware address (MAC)
38       4    Target protocol address (IP)
──────────────────────────────────────── ← end of ARP payload (28 bytes)
Total: 42 bytes
```

The `__attribute__((packed))` on the structs ensures the compiler does not insert padding, so the struct layout matches the wire format exactly.

---

## Error Handling Strategy

Every function that can fail returns `int`: `0` for success, `-1` or `1` for failure. `main()` checks each return value and exits on error, closing the socket if it was opened.

Errors are printed to `stderr` with the program name prefix (`ft_malcolm: ...`), following Unix convention.

The program never crashes on bad input — invalid arguments produce specific error messages and a clean exit with code `1`.
