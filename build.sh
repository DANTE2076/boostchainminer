#!/bin/bash
set -e

if [ "$OS" = "Windows_NT" ]; then
    ./mingw64.sh
    exit 0
fi

make clean || echo clean

rm -f config.status
./autogen.sh

# macOS build (auto-detect Homebrew prefixes for Intel/Apple Silicon)
if [[ "$OSTYPE" == "darwin"* ]]; then
    # Detect brew prefixes if available
    if command -v brew >/dev/null 2>&1; then
        OPENSSL_PREFIX="$(brew --prefix openssl@3 2>/dev/null || brew --prefix openssl 2>/dev/null || true)"
        CURL_PREFIX="$(brew --prefix curl 2>/dev/null || true)"
        JANSSON_PREFIX="$(brew --prefix jansson 2>/dev/null || true)"
    else
        OPENSSL_PREFIX=""
        CURL_PREFIX=""
        JANSSON_PREFIX=""
    fi

    # Help configure find headers/libs (especially OpenSSL on macOS)
    CPPFLAGS_LOCAL=""
    LDFLAGS_LOCAL=""

    if [ -n "$OPENSSL_PREFIX" ] && [ -d "$OPENSSL_PREFIX" ]; then
        CPPFLAGS_LOCAL="$CPPFLAGS_LOCAL -I$OPENSSL_PREFIX/include"
        LDFLAGS_LOCAL="$LDFLAGS_LOCAL -L$OPENSSL_PREFIX/lib"
    fi

    if [ -n "$CURL_PREFIX" ] && [ -d "$CURL_PREFIX" ]; then
        CPPFLAGS_LOCAL="$CPPFLAGS_LOCAL -I$CURL_PREFIX/include"
        LDFLAGS_LOCAL="$LDFLAGS_LOCAL -L$CURL_PREFIX/lib"
    fi

    if [ -n "$JANSSON_PREFIX" ] && [ -d "$JANSSON_PREFIX" ]; then
        CPPFLAGS_LOCAL="$CPPFLAGS_LOCAL -I$JANSSON_PREFIX/include"
        LDFLAGS_LOCAL="$LDFLAGS_LOCAL -L$JANSSON_PREFIX/lib"
    fi

    export CPPFLAGS="$CPPFLAGS_LOCAL $CPPFLAGS"
    export LDFLAGS="$LDFLAGS_LOCAL $LDFLAGS"

    ./nomacro.pl

    CONFIG_ARGS=()
    if [ -n "$OPENSSL_PREFIX" ] && [ -d "$OPENSSL_PREFIX" ]; then
        CONFIG_ARGS+=( "--with-crypto=$OPENSSL_PREFIX" )
    fi
    if [ -n "$CURL_PREFIX" ] && [ -d "$CURL_PREFIX" ]; then
        CONFIG_ARGS+=( "--with-curl=$CURL_PREFIX" )
    fi

    ./configure \
        CFLAGS="-march=native -O2 -Ofast -flto -DUSE_ASM -pg" \
        "${CONFIG_ARGS[@]}"

    make -j"$(sysctl -n hw.ncpu)"

    # Strip the correct binary name
    if [ -f ./boostchainminer ]; then
        strip ./boostchainminer
    elif [ -f ./cpuminer ]; then
        strip ./cpuminer
    fi

    exit 0
fi

# Linux build

# Ubuntu 10.04 (gcc 4.4)
# extracflags="-O3 -march=native -Wall -D_REENTRANT -funroll-loops -fvariable-expansion-in-unroller -fmerge-all-constants -fbranch-target-load-optimize2 -fsched2-use-superblocks -falign-loops=16 -falign-functions=16 -falign-jumps=16 -falign-labels=16"

# Debian 7.7 / Ubuntu 14.04 (gcc 4.7+)
extracflags="$extracflags -Ofast -flto -fuse-linker-plugin -ftree-loop-if-convert-stores"

if [ -f /proc/cpuinfo ] && [ ! "0" = "$(grep -c avx /proc/cpuinfo || true)" ]; then
    # march native doesn't always work, ex. some Pentium Gxxx (no avx)
    extracflags="$extracflags -march=native"
fi

./configure --with-crypto --with-curl CFLAGS="-O2 $extracflags -DUSE_ASM -pg"

make -j 4

strip -s boostchainminer
