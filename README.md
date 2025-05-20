# pdp11emu

This is a somewhat simple multithreaded PDP-11 emulator written in C for POSIX-compatible systems. Currently in development. It is almost 100% accurate, except some potential bugs and timing stuff.

## Run the project yourself

It can do only a part of what some of the originals PDP-11s could, especially late ones. The only peripherals are a teletype and a papertape reader, although new ones can be implemented easily. I am able to run BASIC-11 and `PRINT "HELLO WORLD"` with it, but for some reason neither input nor variables work correclty. There is no proper UI at this point, so eveyrthing is done though the Operator's Console and `tail`ing files, so it may seem a bit inconsistent. 

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

   - For any other GNU/Linux: find similar packages yourself, it's not that hard

   - For Windows: ever seen [this](https://endof10.org/)?

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

 - To load the Absolute Loader
   - `P` - to power up the system. CPU is initially halted.
   - `H` - to enter the halt mode on the console (unblocks interactions).
   - `B` - to automatically insert the bootloader (can be done manually, but generally just a waste of time).
   - `H` - to exit the halt mode on the console (won't start the cpu yet).
   - (`T`, `res/papertapes/absolute_loader.ptap`, `<Enter>` - load the absolute loader paper tape. Unnecessary, as it is preloaded by default.)
   - `S` - to reset the CPU and start from the last loaded address (in this case, beginning of the bootstrap loader).

Then you can watch several seconds of bootloader loading the Absolute Loader into memory. Then CPU halts, indicating that the read was completed. At this point you should be able to load any Absolute Format tape.

 - To load the Absolute Format tape (BASIC, in this case)
   - `T`, `res/papertapes/basic.ptap`, `<Enter>` - load the BASIC paper tape. 
   - `C` - to continue from the next address (in this case, beginning of the Absolute Loader).

Then you just wait for the program to load (may take around a dozen of seconds) and start automatically. In the case of BASIC, you'll see greet message in the TTY output. At this point you should be able to press `<Tab>` and write BASIC, but a big chunk of it won't work yet.

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
