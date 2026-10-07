# Short evaluation

| | |
|---|---|
| VM ip (source) | `192.168.65.14` |
| host IP (target) | `192.168.65.15` |
| host mac | `aa:77:fb:2e:e0:aa` |
| spoof mac | `aa:bb:cc:dd:ee:ff` |
| interface | `enp0s1` |

Second terminal on VM1:

```bash
sudo tcpdump -vv -i enp0s1 arp
```

On the host:

```bash
sudo ip neigh flush all
```

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

The program waits for an ARP request, sends one reply, and exits. `tcpdump` shows the reply after the request. The request comes from `ping` or `arping`. The program answers when the sender IP is the host IP and the request asks for the VM ip.

On VM1:

```bash
sudo ./ft_malcolm 192.168.65.14 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

On the host:

```bash
arping -c 1 -I enp0s1 192.168.65.14
```

VM1 prints the broadcast request, one reply, and `Exiting program...`. `tcpdump` shows who-has, then `is-at aa:bb:cc:dd:ee:ff`.

`arping` does not install a neighbor entry. The host kernel creates one when it asks for the MAC itself. The VM kernel also answers for its own IP with its real MAC, so one reply can lose that race. With `-c` the process stays up until the table shows the spoofed MAC. After exit, another ping lets the VM kernel write its real MAC back.

On VM1, leave it running:

```bash
sudo ./ft_malcolm -c 192.168.65.14 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

On the host:

```bash
ping -c 1 -W 1 192.168.65.14
arp -a | grep 192.168.65.14
```

The line contains `aa:bb:cc:dd:ee:ff`. If it shows the VM interface MAC, run `ping` again. Then Ctrl+C on VM1 and run `arp -a` once more, with no new ping.

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
