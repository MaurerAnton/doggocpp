# doggocpp — DNS Client (C++ port of doggo)

A zero-dependency C++ port of [doggo](https://github.com/mrkny/doggo) — a modern DNS client with colorful output and support for DNS-over-HTTPS (DoH) and DNS-over-TLS (DoT).

## Why doggocpp?

The original [doggo](https://github.com/mrkny/doggo) requires the Go toolchain plus dozens of modules. doggocpp compiles with a single `make` using only C++17 and standard Linux headers.

## Quick Start

```bash
make
./doggocpp example.com
```

## Features

- Query A, AAAA, MX, NS, TXT, CNAME, SOA, and more record types
- DNS-over-HTTPS (DoH) and DNS-over-TLS (DoT) support
- Custom DNS resolver specification
- Colored, human-readable output
- JSON output for scripting
- Short answers mode
- Reverse DNS lookups

## Build

```bash
make
```
Requires: GCC 10+ or Clang 12+, GNU Make, libcurl
