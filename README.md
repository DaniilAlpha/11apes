# pdp11emu

This is a somewhat simple multithreaded pdp11 emulator written in C for POSIX-compatible systems. Currently in active development. It is almost 100% accurate, except some rare error cases and timing stuff.

## Run the project yourself

It cannot do much yet. There is a teletype unit implemented, but i'm not sure if it's working. Papertape reader seems work correctly, but for some reason (not figured out yet) i cannot load anything except the Absolute Loader. To do something you'll need to interact with the Operator's Console.

To run the project follow these simple steps:

- Clone the repo and init submodules

```bash
git clone https://github.com/DaniilAlpha/sem4-osisp-coursework.git pdp11emu
cd pdp11emu
git submodule update --init
```

- Install the proper dependencies

  - For Debian/Ubuntu/Mint/anything with `apt`

    ```bash
    sudo apt install libncurses-dev tmux cmake gcc
    ```

  - For Fedora/anything with `dnf`

    ```bash
    sudo dnf install ncurses-devel tmux cmake gcc
    ```

  - For any other GNU/Linux: find these packages yourself, it's not that hard

  - For Windows: just use Linux

- Build the project

```bash
cmake -B build/ -S ./
cmake --build build/
```

- Now you can run the project

```bash
./run.sh # for a dedicate window, multiplexed with tmux 
```

```bash
build/main/main # for a simpler layout, only Operator's Console
```

## Run tests

- Clone (same as in previous section)

- Install the proper dependencies (same as well)

- Cd into tests

```bash
cd test/
```

- Build (same as well)

- Run tests

```bash
build/test
```
