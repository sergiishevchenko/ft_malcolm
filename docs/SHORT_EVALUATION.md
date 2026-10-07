# Short evaluation

Script for the EvalHub scale. Four mandatory items are Yes/No. The bonus is scored from 0 to 5, one point per relevant bonus. IPv6 would be worth two points; this project has none, so the cap here is 5 from the other bonuses.

This scale has no separate checks for the Makefile, the single global, the allowed function list, or Ctrl+C. Skip them unless someone asks.

Lab addresses. During the defense, substitute whatever `ifconfig` / `ip addr` prints.

| On the scale | In this lab |
|---|---|
| VM ip (source) | `192.168.65.14` |
| host IP (target) | `192.168.65.15` |
| host mac | `aa:77:fb:2e:e0:aa` |
| spoof mac | `aa:bb:cc:dd:ee:ff` |
| interface | `enp0s1` |

The MAC in the arguments is `aa:bb:cc:dd:ee:ff`: the checkbox asks to see that value in `arp -a`.

Before the evaluator sits down: a second terminal on VM1 is already running `tcpdump`, and the host neighbor table is empty.

```bash
sudo tcpdump -vv -i enp0s1 arp
```

```bash
sudo ip neigh flush all
```

---

## Repository

This can zero the grade. It is not its own point. Clone into an empty folder, confirm the repository belongs to the student, and confirm the project is `ft_malcolm`. There are no aliases:

```bash
type ft_malcolm
alias | grep -E 'ft_malcolm|tcpdump|arp|make'
```

From here on, any segfault or other uncontrolled exit means a final grade of 0. Do not edit files during the defense.

The project has no `malloc` of its own. `getifaddrs` / `getaddrinfo` are released with `freeifaddrs` / `freeaddrinfo`. If a leak check is requested, an immediate exit is enough:

```bash
sudo valgrind --leak-check=full ./ft_malcolm
```

---

## 1. User permissions

Without root, the program says root is required and exits. A usage line is appreciated here and is not required.

```bash
./ft_malcolm 192.168.65.14 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

Expected: `ft_malcolm: must be run as root`.

As root with no arguments, it prints usage and exits.

```bash
sudo ./ft_malcolm
```

Expected: `Usage: ft_malcolm [-v] [-c] [-g] [-i interface] <source_ip> <source_mac> <target_ip> <target_mac>`.

---

## 2 and 3. Spoof and behavior on the wire

One run covers both checkboxes. The program waits for an ARP request, then sends a reply and exits. `tcpdump` shows no reply before that request. `ping` or `arping` can produce the request; nobody has to build a frame by hand. The program answers only when the sender IP in the request is the host IP from the arguments, and the request asks for the VM ip.

On VM1:

```bash
sudo ./ft_malcolm 192.168.65.14 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

On the host:

```bash
arping -c 1 -I enp0s1 192.168.65.14
```

On VM1: the broadcast-request text, one reply, `Exiting program...`. In `tcpdump`: who-has first, then `is-at aa:bb:cc:dd:ee:ff`.

`arping` does not install a neighbor entry in the host kernel. The host kernel creates one when it asks for the MAC itself. The VM kernel also answers for its own IP with its real MAC, so a single reply can lose that race. Until the checkbox is satisfied, keep the process alive with `-c`, get the right line, and only then exit. After it exits, do not ping again: the VM kernel will write its real MAC back.

On VM1, leave it running:

```bash
sudo ./ft_malcolm -c 192.168.65.14 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

On the host:

```bash
ping -c 1 -W 1 192.168.65.14
arp -a | grep 192.168.65.14
```

Expected: `aa:bb:cc:dd:ee:ff`. If the line shows the VM interface MAC, run `ping` again. Then Ctrl+C on VM1 and immediately run `arp -a` once more, with no new ping. That is the check "after the program finishes".

---

## 4. Error management

Two commands. The program exits at once, prints a short message, and does not crash.

```bash
sudo ./ft_malcolm 10.11.11.1111 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
sudo ./ft_malcolm 192.168.65.14 aaa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

Expected: `unknown host or invalid IP address` and `invalid mac address`.

---

## Bonus

Score these only when all four mandatory items are Yes. Each item below is one point. The scale stops at 5. Go from top to bottom until time runs out.

Decimal IPv4. `3232252174` is `192.168.65.14`. It starts and waits: Ctrl+C.

```bash
sudo ./ft_malcolm 3232252174 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

Hostname. An unknown name fails immediately.

```bash
sudo ./ft_malcolm 192.168.65.14 aa:bb:cc:dd:ee:ff nonexistent.invalid.host aa:77:fb:2e:e0:aa
```

Verbose. On VM1, `-v`. On the host, one `arping -c 1 -I enp0s1 192.168.65.14`. The output shows Ethernet, ARP, and a hex dump, then the program exits.

```bash
sudo ./ft_malcolm -v 192.168.65.14 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

Continuous is already visible in section 2: several `ping` / `arping` runs, and the process does not exit on its own. If they ask for it separately, use `--continuous`.

Gratuitous: a broadcast reply with no wait for a request, then the process ends by itself. The `tcpdump` that is already open shows `aa:bb:cc:dd:ee:ff > Broadcast` and `Reply 192.168.65.14 is-at aa:bb:cc:dd:ee:ff`.

```bash
sudo ./ft_malcolm -g 192.168.65.14 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

Interface selection, if a minute is left:

```bash
sudo ./ft_malcolm -i enp0s1 192.168.65.14 aa:bb:cc:dd:ee:ff 192.168.65.15 aa:77:fb:2e:e0:aa
```

Ctrl+C. The line will be `Found available interface: enp0s1`.
