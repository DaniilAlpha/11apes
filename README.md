# pdp11emu

This is a somewhat simple multithreaded pdp11 emulator written in C for POSIX-compatible systems. Currently in active development. It is almost 100% accurate, except some rare error cases and timing stuff.

## Run the project yourself

It cannot do much yet. There is a teletype unit implemented, but i'm not sure if it's working correctly. On the other hand, papertape reader works, and is able to load paper tape programs such as Absolute Loader or BASIC-11. There is no proper UI at this point, so eveyrthing is done though the Operator's Console and `tail`ing the files, so it may seem a bit laggy. 

You can try runnint the BASIC, but i believe there is a bug in teletype implementation, so it may not work as expected.

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

At this point i only was able to got to the BASIC's greet message, and any input just halts it, but it is something.


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

Then you just wait for the program to load (may take several dozens of seconds) and start automatically. In case of BASIC, you'll see greet message in the TTY output. At this point you should be able to press `^I` and write BASIC, but you cannot, probably because of some bug in teletype implementation.

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
