# BoostChainMiner

**BoostChainMiner** is a BoostChain-focused CPU miner based on **cpuminer-multi** (tpruvot), itself a fork of **pooler’s cpuminer**.

This repository **does not hide its upstream origins** — see the `AUTHORS` file and the upstream links below.

---

## What’s different in this fork

### BoostChain branding & packaging

* **Binary name:** `boostchainminer`
* Autotools package metadata updated (name, version, URLs)
* Startup banner clearly states fork and upstream attribution

### BoostChain algorithm

* Added **x11r** support
  (randomized X11 order derived from the previous block hash)
  for **BoostChain mining**

> **Note**
> This fork still contains many algorithms inherited from upstream.
> BoostChainMiner users are expected to mine BoostChain using the **x11r** algorithm.

---

## Quick start (BoostChain)

### Example (stratum)

```bash
./boostchainminer -a x11r \
  -o stratum+tcp://YOUR_POOL_HOST:PORT \
  -u YOUR_BOOST_ADDRESS.WORKER \
  -p x \
  -t $(nproc)
```

### Debug mode

```bash
./boostchainminer -a x11r \
  -o stratum+tcp://YOUR_POOL_HOST:PORT \
  -u YOUR_BOOST_ADDRESS.WORKER \
  -p x \
  -t $(nproc) \
  -D
```

### Show all options

```bash
./boostchainminer --help
```

---

## Dependencies

* **libcurl**
* **jansson**
  (jansson source may be included in-tree depending on build)
* **OpenSSL (libcrypto)**
* **pthreads**
* **zlib** (for curl / ssl)

---

## Build

### Basic Linux / macOS build

```bash
./build.sh
```

### Manual build

```bash
./autogen.sh
./configure --with-crypto --with-curl CFLAGS="-O2 -march=native"
make -j$(nproc)
```

### Debian / Ubuntu packages

```bash
apt-get install automake autoconf pkg-config \
  libcurl4-openssl-dev libjansson-dev \
  libssl-dev zlib1g-dev make g++
```

---

## Windows

This repository includes legacy upstream Windows project files.
Most users build using **MinGW64**:

```bash
./mingw64.sh
```

---

## Upstream and credits

This project is based on:

* **cpuminer-multi** (tpruvot)
  [https://github.com/tpruvot/cpuminer-multi](https://github.com/tpruvot/cpuminer-multi)
* **pooler’s cpuminer**
  [https://github.com/pooler/cpuminer](https://github.com/pooler/cpuminer)

All contributors are listed in the `AUTHORS` file.

---

## Donations (upstream)

If you want to support the **upstream project** (cpuminer-multi), please refer to the donation addresses provided in the upstream repository.

---

## License

**GPLv2**
See the `COPYING` file for details.
