# Testing Guide

## Overview

ft_malcolm includes a unit test suite that covers the core logic:
IP/MAC validation and command-line argument parsing.
Network functions (raw sockets, ARP send/receive) require root privileges
and a live network interface, so they are tested manually.

## Running Unit Tests

```bash
make test
```

This compiles and runs the test binary, then removes it automatically.
To clean only test object files:

```bash
make tclean
```

## Test Structure

```
tests/
├── test.h              — Lightweight test framework (macros)
├── test_main.c         — Entry point, runs all suites
├── test_validate_ip.c  — IP address validation tests
├── test_validate_mac.c — MAC address validation tests
└── test_parsing.c      — Argument parsing tests
```

## Test Suites

### validate_ip (12 tests)

| Test | Description |
|------|-------------|
| ip_dotted_valid | Standard dotted-decimal (192.168.1.1) |
| ip_zero | All-zero address (0.0.0.0) |
| ip_broadcast | Broadcast address (255.255.255.255) |
| ip_loopback | Loopback (127.0.0.1) |
| ip_decimal_format | 32-bit decimal integer (3232235777) |
| ip_decimal_zero | Decimal zero (0) |
| ip_hostname_localhost | Hostname resolution (localhost) |
| ip_invalid_format | Out-of-range octets (256.1.1.1) |
| ip_invalid_chars | Non-numeric garbage |
| ip_empty_string | Empty string input |
| ip_too_many_octets | Five octets (1.2.3.4.5) |
| ip_trailing_dot | Trailing dot (1.2.3.) |

### validate_mac (12 tests)

| Test | Description |
|------|-------------|
| mac_valid_lowercase | Lowercase hex (aa:bb:cc:dd:ee:ff) |
| mac_valid_uppercase | Uppercase hex (AA:BB:CC:DD:EE:FF) |
| mac_valid_mixed_case | Mixed case (aA:Bb:cC:Dd:eE:Ff) |
| mac_all_zeros | All zeros (00:00:00:00:00:00) |
| mac_broadcast | Broadcast (ff:ff:ff:ff:ff:ff) |
| mac_too_short | Missing last octet |
| mac_too_long | Extra octet appended |
| mac_wrong_separator | Dash separator instead of colon |
| mac_invalid_hex | Non-hex characters (gg:hh:...) |
| mac_empty_string | Empty string input |
| mac_no_separators | No colons (aabbccddeeff) |
| mac_partial_invalid | Last octet invalid (zz) |

### parse_args (14 tests)

| Test | Description |
|------|-------------|
| parse_basic_args | Minimal valid invocation |
| parse_verbose_flag | Short flag -v |
| parse_verbose_long | Long flag --verbose |
| parse_continuous_flag | Short flag -c |
| parse_gratuitous_flag | Short flag -g |
| parse_interface_option | -i with interface name |
| parse_all_flags | All options combined |
| parse_too_few_args | Missing positional arguments |
| parse_too_many_args | Extra positional argument |
| parse_unknown_option | Unrecognized flag (-x) |
| parse_interface_missing_arg | -i without value |
| parse_invalid_source_ip | Invalid source IP rejected |
| parse_invalid_source_mac | Invalid source MAC rejected |
| parse_invalid_target_mac | Invalid target MAC rejected |

## Manual Testing (Network Functions)

These tests require **root privileges** and should be run inside a Docker
container or a VM with two hosts on the same network segment.

### Setup

```bash
docker compose up -d
docker exec -it malcolm_node1 bash
```

### Basic ARP Spoofing

From node1, run:

```bash
./ft_malcolm 10.0.0.1 aa:bb:cc:dd:ee:ff 10.0.0.2 11:22:33:44:55:66
```

From node2, trigger an ARP request:

```bash
arping -c 1 10.0.0.1
```

Verify on node2 that the ARP table now shows the spoofed MAC:

```bash
arp -n | grep 10.0.0.1
```

### Verbose Mode

```bash
./ft_malcolm -v 10.0.0.1 aa:bb:cc:dd:ee:ff 10.0.0.2 11:22:33:44:55:66
```

Expect hex dump and ARP field details printed for each packet.

### Continuous Mode

```bash
./ft_malcolm -c 10.0.0.1 aa:bb:cc:dd:ee:ff 10.0.0.2 11:22:33:44:55:66
```

Program keeps running after the first reply, spoofing every matching request.

### Gratuitous ARP

```bash
./ft_malcolm -g 10.0.0.1 aa:bb:cc:dd:ee:ff 10.0.0.2 11:22:33:44:55:66
```

Sends a single unsolicited ARP reply and exits immediately.

### Interface Selection

```bash
./ft_malcolm -i eth0 10.0.0.1 aa:bb:cc:dd:ee:ff 10.0.0.2 11:22:33:44:55:66
```

### Signal Handling

Send SIGINT (Ctrl+C) or SIGTERM while the program is listening.
It should print "Exiting program..." and terminate cleanly with exit code 0.

## Adding New Tests

1. Create a new file `tests/test_<module>.c`
2. Include `test.h` and `ft_malcolm.h`
3. Write test functions using the `TEST(name)` macro
4. Add a `run_<module>_tests()` function that calls `RUN(name)` for each test
5. Declare and call `run_<module>_tests()` in `test_main.c`
6. Add the new file to `TEST_SRCS` in the Makefile

## Test Framework Macros

| Macro | Description |
|-------|-------------|
| `TEST(name)` | Declare a test function |
| `ASSERT(cond)` | Fail if condition is false |
| `ASSERT_EQ(a, b)` | Fail if a != b (prints values) |
| `ASSERT_MEM_EQ(a, b, n)` | Fail if memory regions differ |
| `PASS()` | Mark test as passed (required at end) |
| `RUN(name)` | Execute a declared test |
| `TEST_SUITE(name)` | Print suite header |
| `TEST_SUMMARY()` | Print final results |
