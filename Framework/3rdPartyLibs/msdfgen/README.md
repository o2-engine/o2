# msdfgen (core)

Multi-channel signed distance field generator by Viktor Chlumsky, MIT license (`LICENSE.txt`).
Upstream: https://github.com/Chlumsky/msdfgen, release v1.13 (2025-11-30).

Only `core/` is vendored, without the file writers (`save-*`, `export-svg`); `msdfgen/msdfgen-config.h` replaces the
header CMake generates upstream. o2 compiles these sources into `o2Framework` (`SdfFont`, the distance field fonts).
