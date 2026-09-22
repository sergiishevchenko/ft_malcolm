# ft_malcolm — Как работает проект

Подробное описание устройства программы: теория ARP, идея атаки, поток выполнения, структуры данных, каждый исходный файл и формат пакетов на проводе.

---

## Содержание

1. [Что это за проект](#1-что-это-за-проект)
   - [Зачем нужен](#зачем-нужен)
   - [Как работает](#как-работает)
2. [Теория: протокол ARP](#2-теория-протокол-arp)
3. [Идея атаки: ARP spoofing](#3-идея-атаки-arp-spoofing)
4. [Роли аргументов](#4-роли-аргументов)
5. [Общий поток выполнения](#5-общий-поток-выполнения)
6. [Структуры данных](#6-структуры-данных)
7. [Разбор по шагам](#7-разбор-по-шагам)
8. [Легитимный vs поддельный пакет](#8-легитимный-vs-поддельный-пакет)
9. [Формат пакета на проводе](#9-формат-пакета-на-проводе)
10. [Карта исходных файлов](#10-карта-исходных-файлов)
11. [Режимы работы](#11-режимы-работы)
12. [Обработка ошибок и сигналов](#12-обработка-ошибок-и-сигналов)
13. [Типичные сбои](#13-типичные-сбои)
14. [Сборка и проверка](#14-сборка-и-проверка)
15. [Связанные документы](#15-связанные-документы)

---

## 1. Что это за проект

**ft_malcolm** — программа на C, которая подменяет запись в ARP-таблице другого хоста в той же локальной сети.

Она не ходит в интернет, не ломает пароли и не перехватывает TCP. Она делает одну вещь: заставляет жертву поверить, что IP-адрес `source_ip` находится по MAC-адресу `source_mac`. После этого Ethernet-кадры жертвы к этому IP уезжают не к настоящему владельцу, а на подставленный MAC.

Работа идёт на канальном уровне (OSI L2). IP-стек ядра тут ни при чём: программа сама собирает Ethernet-заголовок + ARP-тело (42 байта) и шлёт/принимает кадры через raw-сокет `AF_PACKET` / `SOCK_RAW`. Нужны Linux и root: без `CAP_NET_RAW` такой сокет не открыть.

### Зачем нужен

В Ethernet кадр доставляется по **MAC**, а приложения адресуют хосты по **IP**. Мост между ними — протокол **ARP**: «кто имеет этот IP? — вот мой MAC». В ARP нет подписи и нет проверки, что отправитель Reply действительно владеет этим IP. Кто первым (или громче) ответил — того ядро и запишет в кэш.

Из-за этого ARP-таблица — слабое место всей локальной доставки. Пока запись верная, ping и TCP идут куда надо. Стоит подменить MAC — и весь последующий трафик к этому IP едет «не туда», ещё до IP-фаервола и до маршрутизации. Это и есть ARP spoofing (ARP poisoning): отравление кэша.

Зачем это на практике:

| Что подставить как `source_mac` | Эффект |
|---------------------------------|--------|
| MAC атакующего | кадры жертвы к `source_ip` приходят вам — база для MITM |
| несуществующий MAC | кадры уходят в никуда — по сути DoS до этого IP |
| чужой живой MAC | трафик уезжает третьему хосту |

Полноценный перехват сессии (смотреть чужой HTTP, подменять пакеты) этой программой **не делается**: нет IP-forwarding, нет отравления второй стороны, нет сниффера полезной нагрузки. ft_malcolm закрывает только первый шаг — вписать ложную пару `IP → MAC` в таблицу жертвы. Без этого шага MITM на L2 не начинается.

Режим `-g` (gratuitous ARP) нужен для другого случая: не ждать, пока жертва сама спросит, а сразу объявить сети «`source_ip` теперь здесь». Так обновляют кэш при смене MAC или при захвате адреса, если ОС принимает незапрошенный Reply.

### Как работает

Запуск:

```
sudo ./ft_malcolm 10.0.2.10 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
```

Смысл четырёх аргументов:

- `10.0.2.10` / `de:ad:be:ef:00:01` — **ложная** пара, которую запишем жертве;
- `10.0.2.20` / `08:00:27:dd:ee:ff` — **жертва**: чей Request ловим и кому слать unicast Reply.

Дальше один проход `main()`:

1. **Root.** `getuid() != 0` → сразу выход. Иначе `socket(AF_PACKET, …)` всё равно упадёт с permission denied.
2. **Аргументы** (`parsing.c`). Флаги `-v -c -g -i`, затем ровно четыре позиционных. IP принимается как `a.b.c.d`, как одно десятичное число или как hostname (`getaddrinfo`). MAC строго `XX:XX:XX:XX:XX:XX`. Всё кладётся в одну структуру `t_malcolm`.
3. **Сигналы.** `SIGINT`/`SIGTERM` только ставят `g_running = 0`. Блокирующий `recvfrom` прерывается с `EINTR`, цикл выходит без зависания.
4. **Интерфейс** (`network.c`). Без `-i` берётся первый UP non-loopback с IPv4. С него читаются MAC и `ifindex` (нужен для `sendto`). Печатается `Found available interface: eth0`.
5. **Сокет.** `socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP))` + `SO_BINDTODEVICE`. В сокет попадают только ARP-кадры и только с выбранного iface. Ядро отдаёт кадр целиком, с Ethernet-заголовком — поэтому буфер можно сразу кастить к `t_arp_packet *`.
6. **Две ветки:**
   - **`-g`:** собрать broadcast Reply (`sender_ip == target_ip == source_ip`), один `sendto`, выход. Request никто не ждёт.
   - **Обычный режим:** крутить `recvfrom`, пока не придёт подходящий Request. Фильтр в `is_matching_request`: Ethernet dest = `ff:ff:ff:ff:ff:ff`, opcode = REQUEST, `arp.target_ip` = `source_ip`, `arp.sender_ip` = `target_ip`. Чужие ARP (другие хосты, другие IP, Reply) отбрасываются молча.
7. **Поддельный Reply.** Ethernet dest = MAC жертвы, Ethernet src и ARP sender_mac = `source_mac`, ARP sender_ip = `source_ip`. Это и есть ложь: «я — владелец `source_ip`». Кадр уходит unicast на жертву через `sockaddr_ll` с `sll_ifindex`.
8. **Выход.** Без `-c` — после первого Reply. С `-c` — снова слушать, пока Ctrl+C: ядро жертвы периодически обновляет ARP, и разовый ответ со временем затрётся настоящим.

На стороне жертвы после шага 7 ядро пишет в кэш `10.0.2.10 → de:ad:be:ef:00:01`. Следующий кадр на `10.0.2.10` уже идёт на `de:ad:be:ef:00:01`, а не на реальный MAC владельца.

Связь кусков кода:

```
argv  →  t_malcolm (IP/MAC/флаги)
            │
            ├─ find_interface / open_raw_socket   L2-канал
            │
            ├─ listen_arp_request                 фильтр чужого Request
            └─ send_arp_reply / send_gratuitous_arp
                   42 байта packed-структуры → sendto
```

Ниже — разбор ARP как протокола, затем те же шаги по файлам и полям пакета.

---

## 2. Теория: протокол ARP

Развёрнуто: [ARP.md](ARP.md). Кратко — то, без чего не читается остальной документ.

### Зачем нужен ARP

В IPv4-сети пакет адресуется по IP, но Ethernet передаёт кадры по MAC. Если хост знает IP соседа, но не знает его MAC, он спрашивает это через **ARP** (Address Resolution Protocol, [RFC 826](https://datatracker.ietf.org/doc/html/rfc826)).

### ARP Request (запрос)

Хост A хочет узнать MAC хоста B (`192.168.1.20`):

```
Ethernet:
  dst = ff:ff:ff:ff:ff:ff   (широковещательная рассылка)
  src = MAC(A)
  type = 0x0806             (ARP)

ARP:
  opcode     = 1 (REQUEST)
  sender_mac = MAC(A)
  sender_ip  = IP(A)
  target_mac = 00:00:00:00:00:00  (пока неизвестен)
  target_ip  = IP(B)              («кто имеет этот IP?»)
```

Кадр видят все на сегменте L2. Отвечает только владелец `IP(B)`.

### ARP Reply (ответ)

Хост B отвечает **unicast** на MAC(A):

```
ARP:
  opcode     = 2 (REPLY)
  sender_mac = MAC(B)   ← «вот мой MAC»
  sender_ip  = IP(B)
  target_mac = MAC(A)
  target_ip  = IP(A)
```

Хост A кладёт пару `IP(B) → MAC(B)` в ARP-таблицу и дальше шлёт Ethernet-кадры уже по этому MAC.

### Gratuitous ARP

Незапрошенный ARP (часто Reply), где sender_ip == target_ip. Хост объявляет: «этот IP — мой, вот MAC». Соседи обновляют кэш, если запись уже была. Используется при смене MAC, после failover и т.п. ([RFC 5227](https://datatracker.ietf.org/doc/html/rfc5227)). В проекте режим `-g` отправляет именно такой пакет.

### Почему ARP уязвим

- Нет подписи и проверки «прав на IP»
- Любой может прислать Reply (даже без предшествующего Request — многие ОС всё равно обновляют кэш)
- Broadcast Request видит вся локальная сеть — атакующий тоже
- Последний принятый Reply часто перезаписывает кэш — отсюда гонка с настоящим владельцем

---

## 3. Идея атаки: ARP spoofing

```
Жертва (target)                    ft_malcolm (атакующий)
       |                                    |
       |--- ARP Request (broadcast) ------->|  «Кто имеет source_ip?»
       |                                    |
       |<-- Поддельный ARP Reply -----------|  «source_ip → source_mac»
       |                                    |
 [ARP-кэш отравлен]                  [выход или ожидание следующего]
```

После отравления жертва думает, что `source_ip` живёт по `source_mac`. Дальше её ядро при отправке IP-пакета на `source_ip` ставит Ethernet destination = этот MAC. Настоящий владелец IP кадр уже не получает (если только MAC не его).

### Что происходит после одного Reply

1. Жертва кэширует `source_ip → source_mac`.
2. Пока запись `REACHABLE` / не перезаписана — IP-трафик жертвы к `source_ip` идёт на этот MAC.
3. Настоящий владелец `source_ip` может сам ответить на тот же (или следующий) Request → запись станет снова правильной.
4. Жертва через минуты уйдёт в `STALE` / сделает Probe / новый Request — снова гонка.

Программа травит **одну** сторону и **один** IP. Чтобы стать посредником между жертвой и, например, шлюзом, обычно нужно: отравить ещё шлюз («жертва — это я»), включить форвардинг и разбирать чужой IP-трафик. Этого в ft_malcolm нет: успех — ложная строка в `arp -a` / `ip neigh` на жертве.

### Почему `-c` важен

Без `-c` программа шлёт один Reply и выходит. Если настоящий хост ответил раньше или жертва скоро переспросит — отравление короткое. С `-c` каждый новый подходящий Request снова получает ложный Reply, пока не Ctrl+C.

---

## 4. Роли аргументов

```
sudo ./ft_malcolm [опции] <source_ip> <source_mac> <target_ip> <target_mac>
```

| Аргумент     | Смысл |
|--------------|--------|
| `source_ip`  | IP, который **выдаём за свой** (тот, о котором спрашивает жертва) |
| `source_mac` | MAC, который **подставляем** в Reply как «MAC владельца source_ip» |
| `target_ip`  | IP **жертвы** — программа реагирует только на запросы от этого IP |
| `target_mac` | Реальный MAC жертвы — куда слать unicast Reply |

Путаница имён (частая на защите):

| Думай так | Не думай так |
|-----------|--------------|
| source = «что врём» | source ≠ обязательно IP атакующего |
| target = «кого травим» | target_mac нужен для unicast dest, не для фильтра Ethernet src |

Фильтр входящего Request смотрит на **ARP sender_ip / target_ip**, не на то, совпадает ли Ethernet src с `target_mac`. `target_mac` нужен при отправке Reply.

Фильтр (`is_matching_request` в `arp_listen.c`):

| Проверка            | Поле пакета        | Ожидание              |
|---------------------|--------------------|------------------------|
| Broadcast           | `eth.dest`         | `ff:ff:ff:ff:ff:ff`    |
| Это Request         | `arp.opcode`       | `1`                    |
| Спрашивают наш IP   | `arp.target_ip`    | `source_ip`            |
| Спрашивает жертва   | `arp.sender_ip`    | `target_ip`            |

Остальные ARP-пакеты игнорируются (при `-v` всё равно печатаются в дамп).

---

## 5. Общий поток выполнения

```
main()
  │
  ├─ 1. Проверка root                     getuid() == 0
  ├─ 2. Разбор аргументов                 parsing.c
  │      ├─ флаги                         -v -c -g -i
  │      └─ позиционные                   validate_ip / validate_mac
  ├─ 3. Обработчики сигналов              signal_handler.c
  ├─ 4. Поиск интерфейса                  network.c → find_interface()
  ├─ 5. Raw-сокет                         network.c → open_raw_socket()
  │
  ├─ [если -g]
  │      └─ 6a. Gratuitous ARP            arp_send.c → exit
  │
  └─ [обычный / -c]
         └─ 6b. цикл while (g_running):
                ├─ слушать Request        arp_listen.c
                ├─ слать поддельный Reply arp_send.c
                └─ если не -c → break
```

Точка входа — `srcs/main.c`. Контекст `t_malcolm` один на весь запуск; сокет закрывается перед выходом.

Поток данных между модулями:

```
parsing / validate_*
        │  заполняет source_*/target_*/флаги
        ▼
   t_malcolm
        │
        ├──────────────► network: iface_name, iface_index, sockfd
        │
        ├──────────────► arp_listen: сравнивает поля пакета с source_ip/target_ip
        │
        └──────────────► arp_send: кладёт source_* в sender, target_* в dest/target
```

---

## 6. Структуры данных

Все объявлены в `includes/ft_malcolm.h`.

### 6.1. `t_malcolm` — контекст программы

```
t_malcolm
├── source_ip[4]      IP для имперсонации (порядок байт сети)
├── source_mac[6]     поддельный MAC
├── target_ip[4]      IP жертвы
├── target_mac[6]     MAC жертвы
├── iface_name[]      имя интерфейса (например "eth0")
├── iface_mac[6]      MAC своего интерфейса (считывается в runtime)
├── iface_index       индекс интерфейса (для sockaddr_ll)
├── sockfd            дескриптор raw-сокета (-1 до открытия)
├── verbose           флаг -v
├── continuous        флаг -c
├── gratuitous        флаг -g
└── iface_set         1, если интерфейс задан через -i
```

`iface_mac` в mandatory-сценарии в Reply не подставляется: в Ethernet src и ARP sender_mac идёт `source_mac` из аргументов. Свой MAC интерфейса нужен скорее для отладки / будущих расширений.

### 6.2. Пакетные структуры (packed = без padding компилятора)

```
t_arp_packet (42 байта)
├── t_eth_hdr (14)
│   ├── dest[6]
│   ├── src[6]
│   └── ethertype     0x0806 = ARP
└── t_arp_hdr (28)
    ├── hw_type       1 = Ethernet
    ├── proto_type    0x0800 = IPv4
    ├── hw_len        6
    ├── proto_len     4
    ├── opcode        1 = REQUEST, 2 = REPLY
    ├── sender_mac[6]
    ├── sender_ip[4]
    ├── target_mac[6]
    └── target_ip[4]
```

Многобайтовые поля — network byte order (`htons` / `ntohs`). Благодаря `__attribute__((packed))` раскладка в памяти совпадает с байтами на проводе: буфер `recvfrom` можно кастить к `t_arp_packet *`.

Без `packed` компилятор мог бы выровнять `uint16_t` и раздуть структуру — тогда cast с провода дал бы мусор в полях.

### 6.3. `g_running`

```c
volatile sig_atomic_t g_running = 1;
```

Единственная глобальная переменная. Обработчик `SIGINT`/`SIGTERM` ставит `0`, цикл прослушивания корректно завершается.

`volatile` — чтобы компилятор каждый раз читал переменную из памяти, а не держал в регистре. `sig_atomic_t` — тип, который безопасно писать из обработчика сигнала.

На macOS в заголовке есть заглушки `AF_PACKET` / `sockaddr_ll` для компиляции; реально raw ARP через `AF_PACKET` рассчитан на Linux.

---

## 7. Разбор по шагам

### Шаг 1 — Root

**Файл:** `main.c`

```c
if (getuid() != 0) { /* ошибка и выход */ }
```

Без `CAP_NET_RAW` / root `socket(AF_PACKET, SOCK_RAW, ...)` не откроется. Проверка сразу даёт понятное сообщение вместо permission denied глубже по коду.

### Шаг 2 — Парсинг аргументов

**Файлы:** `parsing.c`, `validate_ip.c`, `validate_mac.c`

#### Фаза 1: опции (`parse_options`)

Идёт по `argv` с индекса 1, пока аргументы начинаются с `-`:

| Флаг | Поле | Эффект |
|------|------|--------|
| `-v` / `--verbose` | `verbose = 1` | подробный дамп каждого ARP |
| `-c` / `--continuous` | `continuous = 1` | не выходить после первого Reply |
| `-g` / `--gratuitous` | `gratuitous = 1` | сразу broadcast, без ожидания Request |
| `-i <name>` | `iface_name`, `iface_set = 1` | зафиксировать интерфейс |

Возвращает индекс начала позиционных аргументов или `-1` при ошибке.

Опции должны идти **до** позиционных: как только встретился аргумент без `-`, разбор флагов останавливается.

#### Фаза 2: позиционные (`parse_positional`)

Ровно 4 аргумента: `source_ip`, `source_mac`, `target_ip`, `target_mac`.

**IP** (`validate_ip`) — три стратегии по порядку:

1. `inet_pton(AF_INET)` — обычный dotted-decimal (`10.0.2.1`)
2. строка из одних цифр — 32-битное десятичное число (`167772161` → `10.0.0.1`)
3. `getaddrinfo(..., AF_INET)` — hostname (`localhost`, DNS)

Результат — 4 байта в network order.

**MAC** (`validate_mac`):

- ровно 17 символов;
- шаблон `XX:XX:XX:XX:XX:XX`;
- двоеточия на позициях 2, 5, 8, 11, 14;
- hex-цифры → 6 байт в `mac_out`.

Дефисы (`aa-bb-…`), точки, запись без разделителей — отказ.

### Шаг 3 — Сигналы

**Файл:** `signal_handler.c`

`sigaction` на `SIGINT` и `SIGTERM`. В обработчике только `g_running = 0` (без `printf` и аллокаций — они не async-signal-safe).

`sa_flags = 0` (без `SA_RESTART`): блокирующий `recvfrom` прерывается с `EINTR`. В `listen_arp_request` при `EINTR` цикл делает `continue`; если к этому моменту `g_running == 0`, внешний `while` в `main` / внутренний в listen завершается.

### Шаг 4 — Интерфейс

**Файл:** `network.c` → `find_interface()`

`getifaddrs()` перечисляет интерфейсы.

**Автовыбор** (если нет `-i`):

- `AF_INET`
- не loopback (`!IFF_LOOPBACK`)
- поднят (`IFF_UP`)

Берётся **первый** подходящий. На машине с несколькими NIC порядок списка может удивить — тогда нужен `-i`.

**Затем** `get_iface_mac`:

- ищется тот же интерфейс с `sa_family == AF_PACKET`;
- из `sockaddr_ll` читаются MAC и `sll_ifindex`.

Индекс нужен для `sendto`: пакетный сокет адресует не IP, а «iface N + MAC dest».

### Шаг 5 — Raw-сокет

**Файл:** `network.c` → `open_raw_socket()`

```c
socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
setsockopt(..., SO_BINDTODEVICE, iface_name, ...);
```

| Параметр | Зачем |
|----------|--------|
| `AF_PACKET` | работа на L2, доступ к Ethernet-заголовку |
| `SOCK_RAW` | кадр целиком (с Ethernet header) |
| `ETH_P_ARP` (0x0806) | в сокет попадают только ARP |
| `SO_BINDTODEVICE` | приём/передача только на выбранном iface |

Без bind к устройству можно было бы ловить ARP со всех iface; `sendto` всё равно требует `sll_ifindex`. Bind делает поведение предсказуемым и совпадает с subject («Found available interface: …»).

### Шаг 6a — Gratuitous ARP (`-g`)

**Файл:** `arp_send.c` → `send_gratuitous_arp()`

Цикл прослушивания не запускается. Один Reply на broadcast:

```
Ethernet:
  dest = ff:ff:ff:ff:ff:ff
  src  = source_mac
  type = 0x0806

ARP:
  opcode     = REPLY (2)
  sender_mac = source_mac
  sender_ip  = source_ip
  target_mac = ff:ff:ff:ff:ff:ff
  target_ip  = source_ip          ← совпадает с sender → gratuitous
```

Все на сегменте видят объявление. Кто уже держал `source_ip` в кэше — может обновить MAC. Сокет закрывается, программа выходит.

Ограничение: если у жертвы записи ещё не было, некоторые стеки **не создают** новую из одного незапрошенного Reply. Тогда надёжнее обычный режим (дождаться Request) или спровоцировать `arping` на жертве.

### Шаг 6b — Слушать и отвечать

#### Прослушивание — `listen_arp_request` (`arp_listen.c`)

В цикле `recvfrom` читает кадры:

1. длина ≥ `sizeof(t_arp_packet)` (42);
2. при `-v` — `print_verbose_pkt(..., out=0)`;
3. `is_matching_request` — все 4 условия из таблицы выше;
4. при совпадении печать MAC/IP отправителя и `return 0`.

Неподходящие пакеты пропускаются молча (кроме verbose).

Почему фильтр жёсткий:

- не отвечать на чужие Request (не наша жертва / не наш IP) — меньше шума и меньше случайного отравления;
- subject требует реагировать на Request **от target про source**.

#### Ответ — `send_arp_reply` (`arp_send.c`)

Сборка через `init_arp_hdr` + поля жертвы:

```
Ethernet:
  dest = target_mac     (unicast жертве)
  src  = source_mac
  type = 0x0806

ARP:
  opcode     = REPLY (2)
  sender_mac = source_mac   ← отравление кэша
  sender_ip  = source_ip
  target_mac = target_mac
  target_ip  = target_ip
```

`init_arp_hdr` заполняет общие константы (hw/proto/opcode) и sender-поля. Dest/target дописываются в `send_arp_reply` или `send_gratuitous_arp`.

Отправка:

```c
sockaddr_ll {
    sll_family  = AF_PACKET
    sll_ifindex = ctx->iface_index
    sll_halen   = 6
    sll_addr    = eth.dest   // target_mac (или broadcast при -g)
}
sendto(sockfd, pkt, sizeof(pkt), 0, &sll, ...);
```

Жертва принимает Reply и пишет в ARP-таблицу: `source_ip → source_mac`.

#### Управление циклом (`main.c`)

После успешного Reply:

- без `-c` — `break`, «Exiting program...», закрытие сокета;
- с `-c` — снова `listen_arp_request`; выход только по сигналу.

---

## 8. Легитимный vs поддельный пакет

Один и тот же Request жертвы. Сравним честный Reply владельца `10.0.2.10` (MAC `08:00:27:aa:bb:cc`) и Reply ft_malcolm с `source_mac=de:ad:be:ef:00:01`.

| Поле | Легитимный Reply | Поддельный Reply |
|------|------------------|------------------|
| eth.dest | MAC жертвы | MAC жертвы |
| eth.src | `08:00:27:aa:bb:cc` | `de:ad:be:ef:00:01` |
| opcode | REPLY | REPLY |
| sender_ip | `10.0.2.10` | `10.0.2.10` |
| sender_mac | `08:00:27:aa:bb:cc` | `de:ad:be:ef:00:01` |
| target_ip | IP жертвы | IP жертвы |
| target_mac | MAC жертвы | MAC жертвы |

Отличается только MAC в sender (и обычно Ethernet src). Для жертвы оба кадра — «валидный» ARP Reply про `10.0.2.10`. Протокол не даёт критерия, кому верить.

Если оба ответа пришли почти одновременно — побеждает тот, чей кадр ядро обработало последним. `-c` увеличивает шанс удерживать ложную запись.

---

## 9. Формат пакета на проводе

```
Offset  Size  Поле
──────  ────  ─────────────────────────────────
 0       6    Ethernet destination MAC
 6       6    Ethernet source MAC
12       2    EtherType (0x0806)
─────────────────────────────────────────────── конец Ethernet (14)
14       2    Hardware type (0x0001)
16       2    Protocol type (0x0800)
18       1    Hardware length (6)
19       1    Protocol length (4)
20       2    Opcode (1 Request / 2 Reply)
22       6    Sender MAC
28       4    Sender IP
32       6    Target MAC
38       4    Target IP
─────────────────────────────────────────────── конец ARP (28)
Итого: 42 байта
```

Минимальный Ethernet-кадр обычно добивается padding до 60 байт на уровне драйвера/NIC; программа шлёт ровно `sizeof(t_arp_packet)`. В `recvfrom` длина может быть ≥ 42 из‑за padding — отсюда проверка `len < sizeof(t_arp_packet)`, а не `!=`.

Подробный разбор полей и hex-пример: [ARP.md](ARP.md).

---

## 10. Карта исходных файлов

| Файл | Роль | Ключевые функции |
|------|------|------------------|
| `srcs/main.c` | точка входа, оркестрация, cleanup | `main()` |
| `srcs/parsing.c` | CLI | `parse_args` → `parse_options` + `parse_positional` |
| `srcs/validate_ip.c` | IP: dotted / decimal / hostname | `validate_ip()` |
| `srcs/validate_mac.c` | MAC `XX:XX:...` | `validate_mac()` |
| `srcs/network.c` | iface + raw socket | `find_interface()`, `open_raw_socket()` |
| `srcs/arp_listen.c` | фильтр и приём Request | `listen_arp_request()` |
| `srcs/arp_send.c` | Reply и gratuitous | `send_arp_reply()`, `send_gratuitous_arp()` |
| `srcs/signal_handler.c` | SIGINT / SIGTERM | `setup_signals()` |
| `srcs/utils.c` | печать MAC/IP | `print_mac()`, `print_ip()` |
| `srcs/verbose.c` | verbose + hex dump | `print_verbose_pkt()`, `print_hex_dump()` |
| `includes/ft_malcolm.h` | константы, структуры, прототипы | — |
| `libft/` | минимальная утилитарная библиотека | `ft_memcmp`, `ft_memcpy`, … |

```
ft_malcolm/
├── Makefile
├── includes/ft_malcolm.h
├── libft/
├── srcs/
│   ├── main.c
│   ├── parsing.c
│   ├── validate_ip.c
│   ├── validate_mac.c
│   ├── network.c
│   ├── arp_listen.c
│   ├── arp_send.c
│   ├── signal_handler.c
│   ├── utils.c
│   └── verbose.c
├── tests/                 # unit-тесты валидации и парсинга
└── docs/
    ├── ru/
    │   ├── ARP.md             # протокол ARP
    │   └── HOW_IT_WORKS.md    # этот файл
    ├── ARCHITECTURE.md
    ├── EVALUATION.md
    └── TESTING.md
```

---

## 11. Режимы работы

### Обычный (по умолчанию)

Ждать один подходящий Request → один Reply → выход.

```bash
sudo ./ft_malcolm 10.0.2.10 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff
```

Ожидаемый вывод после Request с жертвы:

```
Found available interface: eth0
An ARP request has been broadcast.
    mac address of request: 08:00:27:dd:ee:ff
    IP address of request: 10.0.2.20
Now sending an ARP reply to the target address with spoofed source, please wait...
Sent an ARP reply packet, you may now check the arp table on the target.
Exiting program...
```

### Continuous (`-c`)

Отвечать на каждый подходящий Request, пока не придёт Ctrl+C / SIGTERM. Нужен, когда:

- настоящий владелец тоже отвечает;
- жертва часто обновляет neighbor table;
- хочется удержать отравление дольше одного цикла ARP.

### Gratuitous (`-g`)

Не ждать Request: сразу broadcast и выход. Отравление «проактивное». Работает лучше, если запись у соседей уже была.

### Verbose (`-v`)

На каждый принятый/отправлённый ARP:

- Ethernet: src/dst MAC, EtherType  
- ARP: opcode, hw/proto type  
- sender/target MAC и IP  
- hex dump сырых байт  

Полезно сверять фильтр: видно, какие кадры пришли и почему не сматчились.

### Выбор интерфейса (`-i`)

Без `-i` — автодетект первого UP non-loopback IPv4. С `-i eth0` — только этот iface (ошибка, если MAC/index не найдены).

Флаги комбинируются: `-v -c -i eth0 ...`.

`-g` и цикл listen взаимоисключающи в `main`: при `-g` continuous/listen не запускаются.

---

## 12. Обработка ошибок и сигналов

- Функции возвращают `0` при успехе, ненулевое при ошибке; `main` проверяет и выходит с кодом `1`, закрывая сокет при необходимости.
- Сообщения в `stderr` с префиксом `ft_malcolm:`.
- Неверный ввод не должен приводить к segfault — только сообщение и выход.
- `recvfrom` при `EINTR` повторяется, если `g_running` ещё `1`; если `0` — чистый выход с «Exiting program...».
- `ctx.sockfd` изначально `-1`, чтобы не закрыть случайный fd `0` при раннем фейле (хотя в текущем `main` close вызывается только после успешного `open_raw_socket`).

---

## 13. Типичные сбои

| Симптом | Частая причина | Что проверить |
|---------|----------------|---------------|
| Висит на «Waiting» / молчит после Found interface | жертва не шлёт Request | на жертве: `arping -c 1 -I eth0 <source_ip>`; тот же L2-сегмент? |
| Request в tcpdump есть, программа не реагирует | фильтр не совпал | `-v`: sender_ip == target_ip? target_ip ARP == source_ip? broadcast dest? |
| Reply ушёл, в `arp -a` правильный MAC | настоящий хост ответил позже | `-c`; или выключить/изолировать настоящего владельца в лабе |
| Reply ушёл, записи нет | жертва отбросила / другой iface | тот же iface у жертвы? не VRF? static neigh? |
| `no suitable network interface` | только lo / iface down | `ip link`; `-i` |
| `could not get interface info` | `-i` на несуществующее имя | `ip link show` |
| `must be run as root` | забыли sudo | `sudo ./ft_malcolm ...` |
| Работает в bridged, молчит в NAT | VM не в одном L2 | internal/bridged сеть между двумя VM |

Диагностика на проводе (атакующий):

```bash
sudo tcpdump -i eth0 -n -e arp
```

Должны увидеть Request жертвы и сразу свой Reply с поддельным `is-at`.

---

## 14. Сборка и проверка

```bash
make          # libft + ft_malcolm
make clean    # объектники
make fclean   # объектники + бинарник
make re       # полная пересборка
make test     # unit-тесты (без raw-сокетов)
```

Флаги: `-Wall -Wextra -Werror`.

Требования для сетевой части:

- Linux (ядро с `AF_PACKET`)
- root
- два хоста (или VM) в одном L2-сегменте

Минимальный ручной сценарий:

```bash
# VM1 (атакующий)
sudo ./ft_malcolm -v 10.0.2.10 de:ad:be:ef:00:01 10.0.2.20 08:00:27:dd:ee:ff

# VM2 (жертва)
sudo ip neigh del 10.0.2.10 dev eth0 2>/dev/null
arping -c 1 -I eth0 10.0.2.10
ip neigh show
# ожидается: 10.0.2.10 ... de:ad:be:ef:00:01
```

Больше кейсов: [../TESTING.md](../TESTING.md), [../EVALUATION.md](../EVALUATION.md).

---

## 15. Связанные документы

| Документ | Содержание |
|----------|------------|
| [ARP.md](ARP.md) | протокол ARP: кэш, кадр, Request/Reply, уязвимость |
| [../../README.md](../../README.md) | обзор, usage, примеры (EN) |
| [../ARCHITECTURE.md](../ARCHITECTURE.md) | архитектурный walkthrough (EN) |
| [../EVALUATION.md](../EVALUATION.md) | чеклист для защиты / peer review |
| [../TESTING.md](../TESTING.md) | unit-тесты и ручные сетевые проверки |
| [../en.subject.pdf](../en.subject.pdf) | официальный subject проекта |

### Ссылки

- [RFC 826 — ARP](https://datatracker.ietf.org/doc/html/rfc826)
- [RFC 5227 — Address Conflict Detection / gratuitous ARP](https://datatracker.ietf.org/doc/html/rfc5227)
- [ARP spoofing (Wikipedia)](https://en.wikipedia.org/wiki/ARP_spoofing)
