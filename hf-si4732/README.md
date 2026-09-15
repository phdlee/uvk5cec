# UV-K5 CEC HF (SI4732-A10) source

This folder is the **HF full-band receive** firmware line (`CEC_0.51HF`).

It is separate from the main CEC firmware in the repository root (`CEC_0.3V`).

## Hardware note

Requires the SI4732-A10 hardware modification (BK1080 removed / replaced).  
See: http://www.hamskey.com/

## Build

This tree reuses the parent `external/` directory.

### Windows (cmd)

```bat
mklink /J external ..\external
set PATH=C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin;%PATH%
set PATH=C:\Program Files (x86)\GnuWin32\bin;%PATH%
make
```

### Linux / macOS

```bash
ln -s ../external external
make
```

Verified build on 2026-09-15 (produces `firmware.bin`).
