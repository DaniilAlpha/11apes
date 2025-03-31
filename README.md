# pdp11emu

This is a simple pdp11 emulator written in C. Currently in active development.

## Run the roject yourself

There aren't any useful stuff it can do yet. Tests provide more comprehensive view on the project capabilities.

- Clone the repo and init submodules

```bash
git clone https://github.com/DaniilAlpha/sem4-osisp-coursework.git pdp11emu
cd pdp11emu
git submodule update --init
```

- Build the project

```bash
cmake -B build/ -S ./
cmake --build build/
```

- Now you can run the project

```bash
build/main
```

## Run (more useful) tests yourself
- Clone (same as in previous section)

- Cd into tests

```bash
cd test/
```

- Build (same as in previous section)

- Run

```bash
build/test
```
