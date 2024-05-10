# Character Pipeline April 2001 Libraries

Decompilation of the Character Pipeline Apr 2001 library files. **This repository does not provide a complete copy of that version of this component.** This repo is focusing on the built code libraries within this version and is *not a complete replacement.*

## Compatibility

Currently this repository supports building the following libraries:

:heavy_check_mark: = 100% Complete
:warning: = In Progress, at least 1 or more files done.
:x: = No code yet

| Library Name | Progress |
| ------------ | ---------- |
| actor        | :x: |
| anim         | :warning: |
| control      | :heavy_check_mark: |
| geoPalette   | :heavy_check_mark: |
| lighting     | :heavy_check_mark: |
| shader       | :heavy_check_mark: |
| skinning     | :heavy_check_mark: |

## Preparation

After cloning the repo, you can copy your copies of the .a files to baserom/ and run `make extract` to disassemble these files to source files with DTK.

The build process requires the following packages:

- build-essential
- binutils-powerpc-linux-gnu

Under Debian / Ubunutu you can install them with the following commands:

```bash
sudo apt update
sudo apt install build-essential
```

If building any files you can install binutils-powerpc-linux-gnu with:

```bash
sudo apt install binutils-powerpc-linux-gnu
```

Running `make` will then setup DTK if necessary and build all supported libraries.
