# pdp11emu

This is a somewhat simple multithreaded pdp11 emulator written in C for POSIX-compatible systems. Currently in active development. It is almost 100% accurate, except some rare error cases and timing stuff.

## Run the project yourself

It cannot do much yet. There is a teletype unit implemented, but i'm not sure if it's working. Papertape reader seems work correctly, but for some reason (not figured out yet) i cannot load anything except the Absolute Loader. To do something you'll need to interact with the Operator's Console.

To run the project follow these simple steps:

 - Clone the repo, switch to this branch and init submodules

```bash
git clone https://github.com/DaniilAlpha/sem4-osisp-coursework.git pdp11emu
cd pdp11emu
git checkout percentage-3
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
chmod +x ./run.sh
./run.sh # for a dedicate window, multiplexed with tmux 
```

```bash
build/main/main # for a simpler layout, only Operator's Console
```

## Usage

Currently the only piece of the emulator that can be actively interacted with is the Operator's Console. The following actions show how to run an Absolute Loader:

 - `P` - to power up the system. CPU is initially halted.
 - `H` - to enter the halt mode on the console (unblocks interactions).
 - `B` - to automatically insert the bootloader (can be done manually, but generally just a waste of time).
 - `H` - to exit the halt mode on the console (won't start the cpu yet).
 - (`T`, `res/papertapes/absolute_loader.ptap` - load the absolute loader paper tape. Unnecessary, as it is preloaded by default.)
 - `S` - to reset the CPU and start from the last loaded address (in this case, the beginning of the bootstrap loader.

Then you can watch several seconds of bootloader loading the Absolute Loader into memory. Then CPU halts, indicating that the read was completed. At this point you should be able to "insert" any other Absolute Format tape, and press `C` to continue. Your second program then will be loaded into memory, but won't be able to run because of some stupid bug. 

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
