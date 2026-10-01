# struct ifaddrs и getifaddrs

`getifaddrs` спрашивает у ядра адреса локальных интерфейсов и возвращает их связным списком. Один узел этого списка — `struct ifaddrs`. В ft_malcolm список читает `find_interface` в `srcs/network.c`: оттуда берутся имя карты, её MAC и индекс.

Тип и функции объявлены в системном заголовке `<ifaddrs.h>`. В проекте своего определения нет.

---

## Содержание

1. [Что возвращает getifaddrs](#1-что-возвращает-getifaddrs)
2. [Поля struct ifaddrs](#2-поля-struct-ifaddrs)
3. [Один интерфейс — несколько узлов](#3-один-интерфейс--несколько-узлов)
4. [ifa_addr и sa_family](#4-ifa_addr-и-sa_family)
5. [Флаги ifa_flags](#5-флаги-ifa_flags)
6. [Вызов и освобождение](#6-вызов-и-освобождение)
7. [Пример списка](#7-пример-списка)
8. [Как список читает ft_malcolm](#8-как-список-читает-ft_malcolm)
9. [Команды](#9-команды)
10. [Ссылки](#10-ссылки)

---

## 1. Что возвращает getifaddrs

Функция снимает текущие адреса интерфейсов этой машины и складывает их в цепочку. Голова цепочки — адрес первого узла. У каждого узла поле `ifa_next` указывает на следующий. У последнего `ifa_next` равен `NULL`.

```c
int getifaddrs(struct ifaddrs **ifap);
void freeifaddrs(struct ifaddrs *ifa);
```

`ifap` в сигнатуре — указатель на указатель. Функция сама выделяет узлы и записывает адрес первого в переменную вызывающего:

```c
struct ifaddrs *ifap;

if (getifaddrs(&ifap) == -1)
    /* errno уже выставлен, списка нет */
```

До вызова `ifap` ни на что не указывает. После успешного вызова `ifap` — голова списка. Порядок узлов ядро не обещает: на машине с несколькими картами «первый подходящий» может оказаться не той картой, которую ждали.

Память одна на весь список: имена и адреса внутри узлов живут в том же блоке. Отдельный узел через `free` освобождать нельзя. Когда список больше не нужен, его целиком отдаёт `freeifaddrs(ifap)`. После этого недействительны и `ifap`, и все `ifa_name` / `ifa_addr`, которые из него торчали. Нужные байты копируют в свои массивы до `freeifaddrs`.

При ошибке `getifaddrs` возвращает `-1` и ставит `errno`. Списка в этом случае нет, `freeifaddrs` вызывать не нужно.

---

## 2. Поля struct ifaddrs

На Linux узел такой:

```c
struct ifaddrs {
    struct ifaddrs  *ifa_next;     /* следующий узел, у последнего NULL */
    char            *ifa_name;     /* имя интерфейса, например "eth0" */
    unsigned int     ifa_flags;    /* биты состояния, те же, что у SIOCGIFFLAGS */
    struct sockaddr *ifa_addr;     /* адрес этого узла; может быть NULL */
    struct sockaddr *ifa_netmask;  /* маска, если у адреса она есть */
    union {
        struct sockaddr *ifu_broadaddr; /* широковещательный адрес */
        struct sockaddr *ifu_dstaddr;   /* адрес другого конца point-to-point */
    } ifa_ifu;
#define ifa_broadaddr ifa_ifu.ifu_broadaddr
#define ifa_dstaddr   ifa_ifu.ifu_dstaddr
    void            *ifa_data;     /* данные, зависящие от семейства адреса */
};
```

| Поле | Тип | Что лежит |
|------|-----|-----------|
| `ifa_next` | `struct ifaddrs *` | следующий узел списка |
| `ifa_name` | `char *` | имя интерфейса: `"lo"`, `"eth0"`, `"enp0s1"` |
| `ifa_flags` | `unsigned int` | флаги интерфейса, см. [раздел 5](#5-флаги-ifa_flags) |
| `ifa_addr` | `struct sockaddr *` | один адрес. Какой именно — говорит `sa_family` |
| `ifa_netmask` | `struct sockaddr *` | маска этого адреса, например `255.255.255.0` |
| `ifa_broadaddr` | `struct sockaddr *` | broadcast, например `192.168.1.255`. Имеет смысл при флаге `IFF_BROADCAST` |
| `ifa_dstaddr` | `struct sockaddr *` | тот же union, другое имя: адрес соседа на point-to-point, флаг `IFF_POINTOPOINT` |
| `ifa_data` | `void *` | дополнительные данные семейства. Для разбора в этом проекте не используется |

ft_malcolm читает четыре вещи: `ifa_next`, `ifa_name`, `ifa_flags`, `ifa_addr`. Маску, broadcast и `ifa_data` он не трогает.

На macOS шестое поле объявлено сразу как `struct sockaddr *ifa_dstaddr`, без union. Смысл тот же. Проект собирается под Linux: там в списке есть записи `AF_PACKET`.

---

## 3. Один интерфейс — несколько узлов

Узел описывает один адрес, а не всю карту. У `eth0` с IPv4, IPv6 и MAC в списке три узла (или больше, если адресов несколько). Имя `ifa_name` у них одинаковое, `ifa_addr` разный.

```
узел: eth0, AF_PACKET   MAC и ifindex
  ifa_next
узел: eth0, AF_INET     192.168.1.10
  ifa_next
узел: eth0, AF_INET6    fe80::...
  ifa_next
...
```

Поэтому имя и MAC ищутся двумя проходами. Проход по `AF_INET` отвечает на вопрос «какой интерфейс живой и не loopback». Проход по `AF_PACKET` с тем же `ifa_name` отвечает на вопрос «какой у него MAC и индекс». В IPv4-узле MAC нет.

`ifa_addr` бывает `NULL`: интерфейс есть, адреса этого вида нет. Перед `sa_family` указатель проверяют.

---

## 4. ifa_addr и sa_family

`ifa_addr` всегда имеет тип `struct sockaddr *`. Это общая шапка адреса. Первое поле шапки — `sa_family`, номер семейства. По нему решают, какой структурой читать ту же память.

```c
struct sockaddr {
    sa_family_t sa_family;
    char        sa_data[14];
};
```

| `sa_family` | Как читать память | Что там для ft_malcolm |
|-------------|-------------------|------------------------|
| `AF_INET` | `struct sockaddr_in` | IPv4 в `sin_addr` |
| `AF_INET6` | `struct sockaddr_in6` | IPv6, проект эти узлы пропускает |
| `AF_PACKET` | `struct sockaddr_ll` | MAC в `sll_addr`, индекс в `sll_ifindex` |

`AF_PACKET` есть на Linux. Запись приводится так:

```c
struct sockaddr_ll *sll;

sll = (struct sockaddr_ll *)ifa->ifa_addr;
```

Поля, которые забирает `get_iface_mac`:

| Поле | Смысл |
|------|--------|
| `sll_addr` | массив из 8 байт. Первые 6 — MAC интерфейса |
| `sll_ifindex` | номер интерфейса в ядре. Его потом кладут в `sendto` |

На macOS канальный адрес приходит как `AF_LINK` (`struct sockaddr_dl`), не как `AF_PACKET`. Код `get_iface_mac` рассчитан на Linux.

Имя интерфейса и индекс — разные вещи. Имя можно сменить (`ip link set eth0 name lan0`), индекс выдаёт ядро. `SO_BINDTODEVICE` принимает имя, `sendto` на пакетном сокете принимает индекс.

---

## 5. Флаги ifa_flags

`ifa_flags` — битовая маска, та же, что у `SIOCGIFFLAGS` и у `ip link`. Проверка одного бита: `ifa_flags & IFF_UP`. Бит установлен, если результат ненулевой.

Флаги, которые встречаются в этом проекте и рядом с ним:

| Флаг | Бит значит |
|------|------------|
| `IFF_UP` | интерфейс административно включён |
| `IFF_RUNNING` | на линке есть несущая. В `ip link` это часто `LOWER_UP` |
| `IFF_LOOPBACK` | это `lo`. Кадры с него в сегмент не уходят |
| `IFF_BROADCAST` | у интерфейса есть широковещательный адрес |
| `IFF_POINTOPOINT` | соединение точка-точка, смотреть `ifa_dstaddr` |
| `IFF_MULTICAST` | интерфейс умеет групповые MAC |

`auto_detect_iface` требует сразу три условия: `sa_family == AF_INET`, бит `IFF_LOOPBACK` сброшен, бит `IFF_UP` установлен. `IFF_RUNNING` код не проверяет: интерфейс может быть `UP` и при этом без кабеля.

---

## 6. Вызов и освобождение

В `find_interface` схема такая:

```c
struct ifaddrs *ifap;

if (getifaddrs(&ifap) == -1) {
    /* strerror(errno), списка нет */
    return (-1);
}
/* ifap — голова. Список читают auto_detect_iface и get_iface_mac */
freeifaddrs(ifap);
```

`ifap` живёт только в `find_interface`. В помощники уходит тот же указатель, аргумент там тоже называется `ifap`. Текущую позицию при обходе держит отдельная локальная переменная `ifa`: её двигают по `ifa_next`, голова остаётся на месте. Иначе после цикла нечего было бы передать в `freeifaddrs` и во второй проход.

На каждой ветке с `return` список освобождают до выхода. Успешный путь освобождает его один раз, после того как имя, MAC и индекс уже скопированы в `t_malcolm`.

---

## 7. Пример списка

Четыре узла, связанные через `ifa_next`. Порядок показан таким, каким его мог вернуть `getifaddrs`; другой порядок тоже законен.

| # | `ifa_name` | `sa_family` | `ifa_flags` | что в адресе |
|---|------------|-------------|-------------|--------------|
| 1 | `lo` | `AF_PACKET` | `UP`, `LOOPBACK` | MAC `00:00:00:00:00:00`, index 1 |
| 2 | `lo` | `AF_INET` | `UP`, `LOOPBACK` | `127.0.0.1` |
| 3 | `eth0` | `AF_PACKET` | `UP` | MAC `aa:bb:cc:dd:ee:ff`, index 2 |
| 4 | `eth0` | `AF_INET` | `UP` | `192.168.1.10` |

`ifap` после `getifaddrs` указывает на узел 1.

Обход в `auto_detect_iface`, локальный `ifa` стартует с `ifap`:

1. Узел 1. `sa_family` равен `AF_PACKET`, не `AF_INET`. Пропуск.
2. Узел 2. Семейство `AF_INET`, но установлен `IFF_LOOPBACK`. Пропуск.
3. Узел 3. Снова `AF_PACKET`. Пропуск.
4. Узел 4. `AF_INET`, loopback нет, `IFF_UP` есть. `ft_strlcpy` копирует `ifa_name` в `ctx->iface_name`. Там становится `"eth0"`. Функция возвращает `0`.

Обход в `get_iface_mac` снова с головы, `ctx->iface_name` уже `"eth0"`:

1. Узел 1. `ifa_name` равен `"lo"`. Пропуск.
2. Узел 2. Имя снова `"lo"`. Пропуск.
3. Узел 3. Имя `"eth0"`, семейство `AF_PACKET`. `ifa_addr` читают как `struct sockaddr_ll`. Шесть байт из `sll_addr` копируются в `ctx->iface_mac` (`aa:bb:cc:dd:ee:ff`). `ctx->iface_index` становится `sll_ifindex`, то есть `2`.
4. Узел 4 не читается: MAC лежит только в узле `AF_PACKET`.

Дальше `freeifaddrs(ifap)` освобождает все четыре узла. В `ctx` остаются свои копии имени, MAC и индекса.

---

## 8. Как список читает ft_malcolm

`find_interface` делает три шага.

1. `getifaddrs(&ifap)` получает список. При `-1` печатает `getifaddrs: ...` и выходит. `ifap` в этом случае не освобождают.
2. Если пользователь не передал `-i`, поле `ctx->iface_set` равно `0`. Тогда `auto_detect_iface` пишет в `ctx->iface_name` имя первого узла с `AF_INET`, без `IFF_LOOPBACK`, с `IFF_UP`. Если такого узла нет, печатается `no suitable network interface found`.
3. Если `-i` был, `iface_set` равен `1` и имя уже лежит в `ctx->iface_name`. Автовыбор не вызывается. `get_iface_mac` ищет узел с этим именем и `AF_PACKET`, копирует MAC и индекс. Если узла нет, печатается `could not get interface info for <имя>`.

На успехе в stdout уходит строка из subject: `Found available interface: eth0`.

Индекс потом нужен `sendto`: пакетный сокет адресует интерфейс номером, не строкой имени. Имя нужно `SO_BINDTODEVICE` в `open_raw_socket`.

---

## 9. Команды

Те же данные, которые `getifaddrs` кладёт в узлы, видны из утилит:

```bash
ip link show          # имя, индекс, MAC, UP, LOOPBACK
ip addr show          # те же интерфейсы плюс IPv4 и IPv6
```

Строка `ip link` и узлы списка про одну карту:

```
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 state UP
    link/ether aa:bb:cc:dd:ee:ff brd ff:ff:ff:ff:ff:ff
    inet 192.168.1.10/24 brd 192.168.1.255 scope global eth0
```

| В выводе | Поле узла |
|----------|-----------|
| `2` | `sll_ifindex` узла `AF_PACKET` |
| `eth0` | `ifa_name` |
| `UP` | `IFF_UP` в `ifa_flags` |
| `LOWER_UP` | ближе к `IFF_RUNNING`; код его не проверяет |
| `link/ether aa:bb:...` | `sll_addr` узла `AF_PACKET` |
| `inet 192.168.1.10/24` | узел `AF_INET`: адрес в `ifa_addr`, маска в `ifa_netmask` |
| `brd 192.168.1.255` | `ifa_broadaddr` |

---

## 10. Ссылки

| Документ | О чём |
|----------|--------|
| [getifaddrs(3)](https://man7.org/linux/man-pages/man3/getifaddrs.3.html) | список, поля, `freeifaddrs` |
| [packet(7)](https://man7.org/linux/man-pages/man7/packet.7.html) | `AF_PACKET` и `struct sockaddr_ll` |
| [netdevice(7)](https://man7.org/linux/man-pages/man7/netdevice.7.html) | флаги `IFF_*`, индекс интерфейса |

Связанные доки:

- [NIC.md](NIC.md) — карта, имя, индекс, MAC на интерфейсе
- [MAC_IP.md](MAC_IP.md) — что такое MAC и IPv4, которые лежат в узлах
- [HOW_IT_WORKS.md](HOW_IT_WORKS.md) — шаг `find_interface` в общем ходе программы
