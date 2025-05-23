# pdp11emu

This is a bare-bones multithreaded PDP-11 simulator written in C for POSIX-compatible systems. Currently in development. It is pretty accurate, except some rare error handling and timing stuff.

It offers an intwraction through the operator's console, a teletype and a papaertape reader. Though, the console is not simulated in its entirety and UI in general do suck a bit. 

## Run the project yourself

I tried to run different absolute format programs, but because of non-compliant error handling or some other bug i was able to run and use successfuly only the BASIC-11 tape. Apart from that, original diagnostic tapes for the PDP-11 seems work, but it is usually hard to tell as they keep running in a loop until the error occures.

There is no proper UI at this point, so eveyrthing is done though the Operator's Console and `tail`ed files, brougth together in `tmux`, so it may seem a bit inconsistent. 

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

   - For Windows: out of luck - go use GNU/Linux (or at least macOS)

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
   - `T`, `res/papertapes/absolute_loader.ptap`, `Enter` - load the absolute loader paper tape.
   - `S` - to reset the CPU and start from the last loaded address (in this case, beginning of the bootstrap loader).

Then you can watch several seconds-ish of bootloader loading the Absolute Loader into memory. Then CPU halts, indicating that the read was completed. At this point you should be able to load any Absolute Format tape.

 - To load the Absolute Format tape (BASIC-11, in this case)
   - `T`, `res/papertapes/basic.ptap`, `<Enter>` - load the BASIC-11 paper tape. 
   - `C` - to continue from the next address (in this case, beginning of the Absolute Loader).

Then you just wait for the program to load (may take around a dozen of seconds) and start automatically. In the case of BASIC-11, you'll see a greeting message in the TTY output. At this point you should be able to press `^I` (or `Tab` if you like) and write BASIC.

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
