# doggocpp — Raw DNS Client (C++ port of doggo)

A zero-dependency C++ port of [doggo](https://github.com/mrkny/doggo) — a DNS client that constructs raw DNS packets over UDP. No libcurl, no external DNS libraries.

## Why doggocpp?

The original [doggo](https://github.com/mrkny/doggo) requires Go plus dozens of modules. doggocpp compiles with a single `make` using only C++17 and standard POSIX sockets.

## Quick Start

```bash
make
./doggocpp example.com
./doggocpp -t MX example.com
./doggocpp -n 1.1.1.1 example.com
./doggocpp -J example.com      # JSON output
```

## Features

- Raw DNS packet construction over UDP (no external DNS library)
- Supported record types: A, AAAA, NS, CNAME, SOA, MX, TXT
- Custom DNS server (`-n`)
- JSON output (`-J`)
- Plain-text output for non-TTY environments (`--no-color`)
- Colored table output with name, type, TTL, and value columns

## Note

This does **not** support DNS-over-HTTPS (DoH) or DNS-over-TLS (DoT). It sends raw UDP packets to port 53 — a pure DNS client, not an HTTP-based one.

## Build

```bash
make
```
Requires: GCC 10+ or Clang 12+, GNU Make (no external libraries)
