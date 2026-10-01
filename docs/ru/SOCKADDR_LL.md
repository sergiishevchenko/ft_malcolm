# struct sockaddr_ll

`struct sockaddr_ll` — адрес канального уровня для сокета `AF_PACKET`. В нём лежат номер интерфейса и MAC. Буквы `sll` — сокращение sockaddr link layer.

В ft_malcolm структура встречается дважды. `get_iface_mac` в `srcs/network.c` читает её из списка `getifaddrs` и забирает MAC своей карты и её индекс. `do_send` в `srcs/arp_send.c` заполняет свою копию и передаёт её в `sendto`, чтобы кадр ушёл с этой карты на конкретный MAC.

На Linux тип объявлен в `<netpacket/packet.h>`. Проект подключает этот заголовок только под `__linux__`. На других системах в `includes/ft_malcolm.h` лежит такая же структура-заглушка, чтобы файл компилировался. Рабочий raw-сокет `AF_PACKET` рассчитан на Linux.

---

## Содержание

1. [Зачем отдельная структура](#1-зачем-отдельная-структура)
2. [Поля](#2-поля)
3. [Почему в sll_addr восемь байт](#3-почему-в-sll_addr-восемь-байт)
4. [Две роли в ft_malcolm](#4-две-роли-в-ft_malcolm)
5. [Пример: чтение MAC карты](#5-пример-чтение-mac-карты)
6. [Пример: адрес для sendto](#6-пример-адрес-для-sendto)
7. [Приведение типа](#7-приведение-типа)
8. [Команды](#8-команды)
9. [Ссылки](#9-ссылки)

---

## 1. Зачем отдельная структура

Обычный UDP- или TCP-сокет адресует хост парой «IPv4 + порт». Для этого есть `struct sockaddr_in`. Пакетный сокет `AF_PACKET` работает на уровень ниже: он отдаёт кадр конкретной сетевой карте и на конкретный MAC. IP в этом адресе нет.

`struct sockaddr` — общая шапка любого адреса сокета. Первое поле — `sa_family`. Когда оно равно `AF_PACKET`, ту же память читают как `struct sockaddr_ll`.

Список `getifaddrs` кладёт канальный адрес интерфейса в узел с `sa_family == AF_PACKET`. Поле `ifa_addr` там имеет тип `struct sockaddr *`, а на деле это `struct sockaddr_ll`. Как устроен сам список: [IFADDRS.md](IFADDRS.md).

---

## 2. Поля

```c
struct sockaddr_ll {
    unsigned short sll_family;   /* всегда AF_PACKET */
    unsigned short sll_protocol; /* EtherType, в сетевом порядке байт */
    int            sll_ifindex;  /* номер интерфейса в ядре */
    unsigned short sll_hatype;   /* тип железа, для Ethernet это 1 */
    unsigned char  sll_pkttype;  /* кому предназначался кадр */
    unsigned char  sll_halen;    /* сколько байт адреса лежит в sll_addr */
    unsigned char  sll_addr[8];  /* сам адрес; у Ethernet заняты первые 6 */
};
```

| Поле | Тип | Что лежит |
|------|-----|-----------|
| `sll_family` | `unsigned short` | семейство адреса. Для этой структуры всегда `AF_PACKET` |
| `sll_protocol` | `unsigned short` | протокол кадра, например `ETH_P_ARP` (`0x0806`), в сетевом порядке байт. Имеет смысл при `bind`. `ft_malcolm` при отправке оставляет его нулём: сокет уже открыт с `htons(ETH_P_ARP)` |
| `sll_ifindex` | `int` | индекс интерфейса. Тот же номер, что в `ip link` стоит слева от имени: `2: eth0` |
| `sll_hatype` | `unsigned short` | тип канального адреса по ARP. Для Ethernet ядро ставит `ARPHRD_ETHER`, значение `1`. Проект это поле не пишет и не читает |
| `sll_pkttype` | `unsigned char` | вид принятого кадра. Заполняет ядро на приёме. Проект поле не читает |
| `sll_halen` | `unsigned char` | длина адреса в `sll_addr`. Для MAC это `6`, в коде константа `MAC_LEN` |
| `sll_addr` | `unsigned char[8]` | MAC. Значимы первые `sll_halen` байт |

`sll_pkttype` на приёме бывает таким:

| Значение | Имя | Смысл |
|----------|-----|--------|
| 0 | `PACKET_HOST` | кадр адресован MAC этой карты |
| 1 | `PACKET_BROADCAST` | широковещательный MAC `ff:ff:ff:ff:ff:ff` |
| 2 | `PACKET_MULTICAST` | групповой MAC |
| 3 | `PACKET_OTHERHOST` | чужой unicast, карта его всё же отдала наверх |
| 4 | `PACKET_OUTGOING` | кадр, который эта машина сама отправила |

`recvfrom` в проекте адрес отправителя не разбирает: в буфер попадают байты кадра, а `sockaddr_ll` на приёме не используется.

---

## 3. Почему в sll_addr восемь байт

Массив `sll_addr` рассчитан не только на Ethernet. У некоторых каналов адрес длиннее 6 байт, потолок в этой структуре — 8. Сколько байт реально занято, говорит `sll_halen`.

MAC Ethernet — 6 байт. `get_iface_mac` копирует ровно `MAC_LEN`:

```c
ft_memcpy(ctx->iface_mac, sll->sll_addr, MAC_LEN);
```

`ctx->iface_mac` — массив `uint8_t` из 6 байт в `t_malcolm`. Седьмой и восьмой байты `sll_addr` для Ethernet не несут адрес. Их не копируют.

---

## 4. Две роли в ft_malcolm

Одна и та же структура описывает разное в зависимости от того, кто её заполнил.

**Чтение из `getifaddrs`.** Узел `AF_PACKET` описывает саму карту. В `sll_addr` — MAC интерфейса, в `sll_ifindex` — его номер. Это ответ на вопрос «кто я на проводе и какой у меня индекс».

**Запись в `sendto`.** Локальная переменная `sll` в `do_send` описывает один исходящий кадр. `sll_ifindex` — с какой карты слать. `sll_addr` — MAC получателя этого кадра, тот же, что в Ethernet-поле destination. Это уже не MAC своей карты.

Индекс и имя — разные вещи. Имя (`"eth0"`) нужно `SO_BINDTODEVICE`. `sendto` на пакетном сокете имя не принимает: он принимает `sll_ifindex`.

---

## 5. Пример: чтение MAC карты

После автовыбора `ctx->iface_name` равен `"eth0"`. В списке `getifaddrs` есть узел:

| поле узла | значение |
|-----------|----------|
| `ifa_name` | `"eth0"` |
| `sa_family` | `AF_PACKET` |
| дальше в той же памяти | `struct sockaddr_ll` |

`get_iface_mac` приводит `ifa_addr` к `struct sockaddr_ll *` и кладёт указатель в локальную переменную `sll`. Для этого `eth0` поля такие:

| поле | значение |
|------|----------|
| `sll_family` | `AF_PACKET` |
| `sll_ifindex` | `2` |
| `sll_halen` | `6` |
| `sll_addr` | `aa bb cc dd ee ff`, затем два неиспользуемых байта |

Шесть байт копируются в `ctx->iface_mac`. `ctx->iface_index` становится `2`. Функция возвращает `0`.

Узел того же `eth0` с `AF_INET` и адресом `192.168.1.10` этой структурой не является. Его `sa_family` другой, условие в `get_iface_mac` его пропускает.

---

## 6. Пример: адрес для sendto

`do_send` собирает адрес на стеке. `ft_bzero` обнуляет все поля, затем заполняются четыре:

```c
struct sockaddr_ll sll;

ft_bzero(&sll, sizeof(sll));
sll.sll_family = AF_PACKET;
sll.sll_ifindex = ctx->iface_index;
sll.sll_halen = MAC_LEN;
ft_memcpy(sll.sll_addr, pkt->eth.dest, MAC_LEN);
```

`sll` здесь — сама структура, не указатель. `ctx->iface_index` равен `2`, это значение из примера выше. `pkt->eth.dest` — MAC в заголовке кадра, 6 байт.

Обычный ARP Reply кладёт туда MAC жертвы, например `11:22:33:44:55:66`. Тогда `sll` выглядит так:

| поле | значение |
|------|----------|
| `sll_family` | `AF_PACKET` |
| `sll_protocol` | `0`, поле не заполняли |
| `sll_ifindex` | `2` |
| `sll_hatype` | `0` |
| `sll_pkttype` | `0` |
| `sll_halen` | `6` |
| `sll_addr` | `11 22 33 44 55 66 00 00` |

`sendto` принимает адрес как `struct sockaddr *`, поэтому указатель на `sll` приводят к этому типу. Последний аргумент — размер всей структуры, `sizeof(sll)`.

При `-g` destination кадра — широковещательный MAC `ff:ff:ff:ff:ff:ff`. В `sll_addr` копируются те же шесть байт `0xff`. Индекс карты остаётся `2`: broadcast уходит в сегмент этой карты, а не «во все интерфейсы».

---

## 7. Приведение типа

`ifa_addr` и аргумент `sendto` объявлены как `struct sockaddr *`. Компилятор по этому типу видит только `sa_family` и общий хвост `sa_data`. Поля `sll_ifindex` и `sll_addr` у `struct sockaddr` не видны.

Приведение говорит читать ту же память другим типом:

```c
sll = (struct sockaddr_ll *)ifa->ifa_addr;
```

Оно законно, когда `sa_family` уже проверен и равен `AF_PACKET`. Память при этом не копируется: `sll` указывает внутрь узла `getifaddrs`. После `freeifaddrs` этот указатель использовать нельзя, поэтому MAC и индекс перед освобождением списка копируют в поля `ctx`.

В `do_send` направление обратное. Своя структура `sll` приводится к `(struct sockaddr *)&sll` только на время вызова `sendto`.

---

## 8. Команды

Индекс и MAC, которые ядро кладёт в `sockaddr_ll` узла `AF_PACKET`:

```bash
ip link show
```

```text
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 state UP
    link/ether aa:bb:cc:dd:ee:ff brd ff:ff:ff:ff:ff:ff
```

| В выводе | Поле |
|----------|------|
| `2` | `sll_ifindex` |
| `link/ether aa:bb:cc:dd:ee:ff` | первые 6 байт `sll_addr` |
| `brd ff:ff:ff:ff:ff:ff` | широковещательный MAC сегмента. В `sockaddr_ll` карты его нет; при `-g` программа сама пишет его в `sll_addr` исходящего кадра |

---

## 9. Ссылки

| Документ | О чём |
|----------|--------|
| [packet(7)](https://man7.org/linux/man-pages/man7/packet.7.html) | `AF_PACKET`, поля `sockaddr_ll`, `sendto` |
| [netdevice(7)](https://man7.org/linux/man-pages/man7/netdevice.7.html) | индекс интерфейса |

Связанные доки:

- [IFADDRS.md](IFADDRS.md) — список, в котором узел `AF_PACKET` хранит эту структуру
- [NIC.md](NIC.md) — карта, её MAC и индекс
- [MAC_IP.md](MAC_IP.md) — что за 6 байт лежат в `sll_addr`
- [HOW_IT_WORKS.md](HOW_IT_WORKS.md) — `get_iface_mac` и `sendto` в общем ходе программы
