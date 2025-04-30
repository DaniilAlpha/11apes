#include <stdlib.h>

#include <ncurses.h>
#include <unistd.h>

#include "pdp11/pdp11.h"
#include "pdp11/pdp11_console.h"
#include "pdp11/pdp11_papertape_reader.h"
#include "pdp11/pdp11_teletype.h"

// WARN shit code below!!!

#define COLOR_PAIR_OFF      1
#define COLOR_PAIR_ON       2
#define COLOR_PAIR_LABEL    3
#define COLOR_PAIR_SELECTED 4
#define COLOR_PAIR_VALUE    8

// UI Element Selection Enum
typedef enum {
    SELECT_POWER,

    SELECT_SWITCH_REG_BIT_0,  // Start of switch register bits
    SELECT_SWITCH_REG_BIT_1,
    SELECT_SWITCH_REG_BIT_2,
    SELECT_SWITCH_REG_BIT_3,
    SELECT_SWITCH_REG_BIT_4,
    SELECT_SWITCH_REG_BIT_5,
    SELECT_SWITCH_REG_BIT_6,
    SELECT_SWITCH_REG_BIT_7,
    SELECT_SWITCH_REG_BIT_8,
    SELECT_SWITCH_REG_BIT_9,
    SELECT_SWITCH_REG_BIT_10,
    SELECT_SWITCH_REG_BIT_11,
    SELECT_SWITCH_REG_BIT_12,
    SELECT_SWITCH_REG_BIT_13,
    SELECT_SWITCH_REG_BIT_14,
    SELECT_SWITCH_REG_BIT_15,  // End of switch register bits

    SELECT_LOAD_ADDR,
    SELECT_EXAMINE,
    SELECT_CONTINUE,
    SELECT_ENABLE,
    SELECT_START,
    SELECT_DEPOSIT,

    SELECT_COUNT  // Total number of selectable items
} SelectableElement;

// --- Helper Functions ---

// Function to draw a single bit (light or switch)
void draw_bit(int y, int x, bool is_on, bool is_selected) {
    attr_t attr = is_on ? COLOR_PAIR(COLOR_PAIR_ON) | A_REVERSE
                        : COLOR_PAIR(COLOR_PAIR_OFF);
    if (is_selected) { attr = COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE; }
    attron(attr);
    // Using different symbols for lights vs switches can be nice
    // For simplicity here, using [ ] representation
    mvprintw(y, x, is_on ? "(*)" : "( )");
    attroff(attr);
}

// Function to draw a register (Address or Data) with lights and octal value
void draw_register(int y, int x_start, char const *label, uint16_t value) {
    mvprintw(y - 1, x_start, "%s", label);
    attron(COLOR_PAIR(COLOR_PAIR_VALUE));
    printw(" = %06o", value);
    attroff(COLOR_PAIR(COLOR_PAIR_VALUE));

    for (int i = 0; i < 16; ++i) {
        bool is_on = (value >> (15 - i)) & 1;
        // Draw bits from left (15) to right (0)
        draw_bit(
            y,
            x_start + i * 3 + (i + 2) / 3,
            is_on,
            false
        );  // Not selectable directly
    }
}

// Function to draw the switch register
void draw_switch_register(
    int y,
    int x_start,
    uint16_t value,
    SelectableElement current_selection
) {
    mvprintw(y - 2, x_start, "SWITCH REGISTER");
    attron(COLOR_PAIR(COLOR_PAIR_VALUE));
    printw(" = %06o", value);  // Display octal value
    attroff(COLOR_PAIR(COLOR_PAIR_VALUE));

    for (unsigned i = 0; i < 16; ++i) {
        bool is_on = (value >> (15 - i)) & 1;
        bool is_selected = current_selection - SELECT_SWITCH_REG_BIT_0 == i;
        attr_t attr =
            is_on ? COLOR_PAIR(COLOR_PAIR_ON) : COLOR_PAIR(COLOR_PAIR_OFF);
        if (is_selected) { attr = COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE; }
        mvprintw(y - 1, x_start + i * 3 + (i + 2) / 3 + 1, "%d", 15 - i);
        attron(attr);
        mvprintw(
            y,
            x_start + i * 3 + (i + 2) / 3,
            is_on ? "'-'" : "|_|"
        );  // Down=1, Up=0
        attroff(attr);
    }
}

// Function to draw status lights
void draw_status_lights(int y, int x, Pdp11Console const *console) {
    attr_t attr_on = COLOR_PAIR(COLOR_PAIR_ON) | A_REVERSE;
    attr_t attr_off = COLOR_PAIR(COLOR_PAIR_OFF);

    mvprintw(y - 1, x + 2, "RUN");
    bool const run_light = pdp11_console_run_light(console);
    attron(run_light ? attr_on : attr_off);
    mvprintw(y, x, run_light ? "( *** )" : "(     )");
    attroff(run_light ? attr_on : attr_off);

    mvprintw(y - 1, x + 7 + 2, "BUS");
    bool const bus_light = pdp11_console_bus_light(console);
    attron(bus_light ? attr_on : attr_off);
    mvprintw(y, x + 7, bus_light ? "( *** )" : "(     )");
    attroff(bus_light ? attr_on : attr_off);

    mvprintw(y - 1, x + 7 + 7 + 2, "FETCH");
    bool const fetch_light = pdp11_console_fetch_light(console);
    attron(fetch_light ? attr_on : attr_off);
    mvprintw(y, x + 7 + 7 + 2, fetch_light ? "( * )" : "(   )");
    attroff(fetch_light ? attr_on : attr_off);

    mvprintw(y - 1, x + 7 + 7 + 2 + 5, " EXEC");
    bool const exec_light = pdp11_console_fetch_light(console);
    attron(exec_light ? attr_on : attr_off);
    mvprintw(y, x + 7 + 7 + 2 + 5, exec_light ? "( * )" : "(   )");
    attroff(exec_light ? attr_on : attr_off);

    y += 3;

    mvprintw(y - 1, x, "SOURCE ");
    bool const source_light = pdp11_console_source_light(console);
    attron(source_light ? attr_on : attr_off);
    mvprintw(y, x, source_light ? "( *** )" : "(     )");
    attroff(source_light ? attr_on : attr_off);

    mvprintw(y - 1, x + 7, "DESTINA");
    bool const destination_light = pdp11_console_destination_light(console);
    attron(destination_light ? attr_on : attr_off);
    mvprintw(y, x + 7, destination_light ? "( *** )" : "(     )");
    attroff(destination_light ? attr_on : attr_off);

    mvprintw(y - 1, x + 7 + 7 + 2 + 1, "ADDRESS ");
    unsigned const addr_mode = pdp11_console_address_light(console);

    attron(BIT(addr_mode, 1) ? attr_on : attr_off);
    mvprintw(y, x + 7 + 7 + 2, BIT(addr_mode, 1) ? "( ** " : "(    ");
    attroff(BIT(addr_mode, 1) ? attr_on : attr_off);
    attron(BIT(addr_mode, 0) ? attr_on : attr_off);
    mvprintw(y, x + 7 + 7 + 2 + 5, BIT(addr_mode, 0) ? " ** )" : "    )");
    attroff(BIT(addr_mode, 0) ? attr_on : attr_off);
}

// Function to draw the power switch
void draw_power_switch(
    int y,
    int x,
    Pdp11ConsolePowerControl power_state,
    bool is_selected
) {
    mvprintw(y, x, "OFF");
    mvprintw(y - 1, x + 3, "PWR");
    mvprintw(y, x + 6, "LCK");

    // Indicate active state clearly
    attr_t const attr = COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE;
    if (is_selected) attron(attr);
    switch (power_state) {
    case PDP11_CONSOLE_POWER_CONTROL_OFF: mvprintw(y, x + 3 + 1, "<"); break;
    case PDP11_CONSOLE_POWER_CONTROL_POWER: mvprintw(y, x + 3 + 1, "^"); break;
    case PDP11_CONSOLE_POWER_CONTROL_LOCK: mvprintw(y, x + 3 + 1, ">"); break;
    }
    if (is_selected) attroff(attr);
}

// Function to draw control buttons
void draw_control_buttons(
    int y,
    int x,
    Pdp11Console const *console,
    SelectableElement current_selection
) {
    bool selected;

    // LOAD ADDR
    selected = (current_selection == SELECT_LOAD_ADDR);
    mvprintw(y - 1, x + 1, "L");
    attron(
        selected ? (COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE)
                 : COLOR_PAIR(COLOR_PAIR_OFF)
    );
    mvprintw(y, x, "|-|");
    attroff(
        selected ? (COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE)
                 : COLOR_PAIR(COLOR_PAIR_OFF)
    );
    x += 3;

    // EXAMINE
    selected = (current_selection == SELECT_EXAMINE);
    mvprintw(y - 1, x + 1, "E");
    attron(
        selected ? (COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE)
                 : COLOR_PAIR(COLOR_PAIR_OFF)
    );
    mvprintw(y, x, "|-|");
    attroff(
        selected ? (COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE)
                 : COLOR_PAIR(COLOR_PAIR_OFF)
    );
    x += 3;

    // CONT
    selected = (current_selection == SELECT_CONTINUE);
    mvprintw(y - 1, x + 1, "C");
    attron(
        selected ? (COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE)
                 : COLOR_PAIR(COLOR_PAIR_OFF)
    );
    mvprintw(y, x, "|-|");
    attroff(
        selected ? (COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE)
                 : COLOR_PAIR(COLOR_PAIR_OFF)
    );
    x += 3;

    // ENABLE/HALT Toggle Switch
    selected = (current_selection == SELECT_ENABLE);
    mvprintw(y - 1, x, "E/H");
    bool const enable_state = pdp11_console_enable_switch(console);
    attron(
        selected ? (COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE)
                 : COLOR_PAIR(enable_state ? COLOR_PAIR_ON : COLOR_PAIR_OFF)
    );
    mvprintw(y, x, enable_state ? "'-'" : "|_|");
    attroff(
        selected ? (COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE)
                 : COLOR_PAIR(enable_state ? COLOR_PAIR_ON : COLOR_PAIR_OFF)
    );
    x += 3;

    // START
    selected = (current_selection == SELECT_START);
    mvprintw(y - 1, x + 1, "S");
    attron(
        selected ? (COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE)
                 : COLOR_PAIR(COLOR_PAIR_OFF)
    );
    mvprintw(y, x, "|-|");
    attroff(
        selected ? (COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE)
                 : COLOR_PAIR(COLOR_PAIR_OFF)
    );
    x += 3;

    x++;

    // DEPOSIT
    selected = (current_selection == SELECT_DEPOSIT);
    mvprintw(y - 1, x + 1, "D");
    attron(
        selected ? (COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE)
                 : COLOR_PAIR(COLOR_PAIR_OFF)
    );
    mvprintw(y, x, "|-|");
    attroff(
        selected ? (COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE)
                 : COLOR_PAIR(COLOR_PAIR_OFF)
    );
    x += 3;
}

// --- Main UI Function ---

void run_console_ui(
    Pdp11Console *const console,
    Pdp11PapertapeReader *const pr,
    Pdp11Teletype *const tty
) {
    initscr();  // Start curses mode
    clear();
    noecho();              // Don't echo() while we should be in control
    cbreak();              // Line buffering disabled, Pass on everything
    keypad(stdscr, TRUE);  // Enable function keys (arrows, etc.)
    curs_set(0);           // Hide the cursor
    timeout(100);          // Non-blocking input with 100ms timeout

    if (!has_colors()) {
        endwin();
        printf("Your terminal does not support color\n");
        exit(1);
    }

    start_color();
    // Define color pairs (foreground, background)
    init_pair(COLOR_PAIR_OFF, COLOR_WHITE, COLOR_BLACK);     // Off light/switch
    init_pair(COLOR_PAIR_ON, COLOR_RED, COLOR_BLACK);        // On light/switch
    init_pair(COLOR_PAIR_LABEL, COLOR_YELLOW, COLOR_BLACK);  // Labels
    init_pair(COLOR_PAIR_SELECTED, COLOR_YELLOW, COLOR_BLACK);  // Selected item
    init_pair(5, COLOR_WHITE, COLOR_WHITE);
    init_pair(6, COLOR_WHITE, COLOR_WHITE);
    init_pair(7, COLOR_WHITE, COLOR_WHITE);
    init_pair(COLOR_PAIR_VALUE, COLOR_RED, COLOR_BLACK);  // Octal values

    SelectableElement current_selection =
        SELECT_SWITCH_REG_BIT_15;  // Start selection at SR bit 15
    bool quit = false;

    bool is_in_input_mode = false;
    while (!quit) {
        // --- Get State ---
        Pdp11ConsolePowerControl power_state =
            pdp11_console_power_control(console);
        uint16_t switch_reg = pdp11_console_switch_register(console);
        uint16_t addr_reg = pdp11_console_address_indicator(console);
        uint16_t data_reg = pdp11_console_data_indicator(console);

        // --- Drawing ---
        erase();  // Clear screen efficiently

        // Title
        attron(A_BOLD | COLOR_PAIR(COLOR_PAIR_LABEL));
        mvprintw(0, (COLS - 28) / 2, "PDP-11/20 Operator Console");
        attroff(A_BOLD | COLOR_PAIR(COLOR_PAIR_LABEL));

        // Power Switch
        draw_power_switch(
            12,
            2,
            power_state,
            current_selection == SELECT_POWER
        );

        // Indicator Lights (Address, Data, Status)
        draw_register(3, 14, "ADDRESS REGISTER", addr_reg);
        draw_register(6, 14, "DATA", data_reg);
        draw_status_lights(3, 72, console);

        // Switch Register
        draw_switch_register(12, 14, switch_reg, current_selection);

        // Control Buttons
        draw_control_buttons(12, 72, console, current_selection);

        // Help Text
        attron(COLOR_PAIR(COLOR_PAIR_LABEL));
        mvprintw(
            LINES - 4,
            0,
            is_in_input_mode
                ? "\n\n\n"
                  " * ^N - back into normal mode\t"
                  " * Anykey - teletype input\n"
                : " * Left/Right - move around\t"
                  " * Up/Down/Enter/Space - toggle switch\n"
                  " * O/P - dec/inc power lvl\t"
                  " * L, E, C, H, S, D - load, exam, cont, enbl/halt, start, deposit\n"
                  " * B - autoinsert bootloader\t"
                  " * T - change paper tape;\n"
                  " * ^I - into insert mode\t"
                  " * ^D - create memory dump\t"
                  " * Q - quit\n"
        );
        attroff(COLOR_PAIR(COLOR_PAIR_LABEL));

        refresh();

        int const ch = getch();
        if (is_in_input_mode) {
            switch (ch) {
            case 'N' & 0x1F: is_in_input_mode = false; break;

            case '\n':
            case KEY_ENTER: pdp11_teletype_putc(tty, '\n'); break;
            case ' ' ... '~': pdp11_teletype_putc(tty, ch); break;
            }
        } else {
            switch (ch) {
            case 'I' & 0x1F: is_in_input_mode = true; break;

            case KEY_LEFT:
                current_selection = current_selection > 0
                                      ? current_selection - 1
                                      : SELECT_COUNT - 1;
                break;

            case KEY_RIGHT:
                current_selection = current_selection < SELECT_COUNT - 1
                                      ? current_selection + 1
                                      : 0;
                break;

            case KEY_UP:
            case KEY_DOWN:
            case ' ':
            case '\n':
            case KEY_ENTER: {
                if (current_selection == SELECT_POWER) {
                    ch == KEY_UP ? pdp11_console_next_power_control(console)
                                 : pdp11_console_prev_power_control(console);
                } else {
                    if (current_selection >= SELECT_SWITCH_REG_BIT_0 &&
                        current_selection <= SELECT_SWITCH_REG_BIT_15) {
                        int bit_index =
                            15 - (current_selection - SELECT_SWITCH_REG_BIT_0);
                        pdp11_console_toggle_control_switch(console, bit_index);
                    } else if (current_selection == SELECT_LOAD_ADDR) {
                        pdp11_console_press_load_addr(console);
                    } else if (current_selection == SELECT_EXAMINE) {
                        pdp11_console_press_examine(console);
                    } else if (current_selection == SELECT_DEPOSIT) {
                        pdp11_console_press_deposit(console);
                    } else if (current_selection == SELECT_CONTINUE) {
                        pdp11_console_press_continue(console);
                    } else if (current_selection == SELECT_ENABLE) {
                        pdp11_console_toggle_enable(console);
                    } else if (current_selection == SELECT_START) {
                        pdp11_console_press_start(console);
                    } else if (current_selection == SELECT_POWER) {
                        // Cycle power with space/enter as well
                        pdp11_console_next_power_control(console);
                    }
                }
            } break;

            case 'Q':
            case 'q': quit = true; break;
            case 'P':
            case 'p': pdp11_console_next_power_control(console); break;
            case 'O':
            case 'o': pdp11_console_prev_power_control(console); break;

            case 'L':
            case 'l': pdp11_console_press_load_addr(console); break;
            case 'E':
            case 'e': pdp11_console_press_examine(console); break;
            case 'C':
            case 'c': pdp11_console_press_continue(console); break;
            case 'H':
            case 'h': pdp11_console_toggle_enable(console); break;
            case 'S':
            case 's': pdp11_console_press_start(console); break;
            case 'D':
            case 'd': pdp11_console_press_deposit(console); break;

            case 'B':
            case 'b': pdp11_console_insert_bootloader(console); break;

            case 'T':
            case 't': {
                def_prog_mode();
                endwin();

                printf("Enter new papertape name to load: ");
                char papertape[256] = {0};
                while (scanf(" %[^\n]256s", papertape) != 1)
                    printf("invalid!\n"), fflush(stdin);

                if (pdp11_papertape_reader_load(pr, papertape) != Ok) {
                    printf(
                        "cannot open papertape: '%s'. continuing in several seconds...\n",
                        papertape
                    );
                    sleep(2);
                } else {
                    printf("tape loaded. continuing in a second...\n");
                    sleep(1);
                }

                reset_prog_mode();
                refresh();
            } break;

            case 'D' & 0x1F: {
                def_prog_mode();
                endwin();

                if (pdp11_ram_save(&console->_pdp11->ram) != Ok) {
                    printf(
                        "error saving memory dump. continuing in several seconds...\n"
                    );
                    sleep(2);
                } else {
                    printf("memory dump saved. continuing in a second...\n");
                    sleep(1);
                }

                reset_prog_mode();
                refresh();
            } break;

            case ERR:
            default: break;
            }
        }
    }

    endwin();
}

int main() {
    Pdp11 pdp = {0};
    UNROLL(pdp11_init(&pdp));

    Pdp11PapertapeReader pr = {0};
    pdp11_papertape_reader_init(
        &pr,
        &pdp.unibus,
        PDP11_PAPERTAPE_READER_ADDR,
        PDP11_PAPERTAPE_READER_INTR_VEC,
        PDP11_PAPERTAPE_READER_INTR_PRIORITY
    );

    Pdp11Teletype tty = {0};
    pdp11_teletype_init(
        &tty,
        &pdp.unibus,
        PDP11_TELETYPE_ADDR,
        PDP11_TELETYPE_KEYBOARD_INTR_VEC,
        PDP11_TELETYPE_PRINTER_INTR_VEC,
        PDP11_TELETYPE_INTR_PRIORITY,
        -1  // TODO
    );

    Pdp11Console console = {0};
    pdp11_console_init(&console, &pdp);

    pdp.periphs++[0] = pdp11_console_ww_unibus_device(&console);
    pdp.periphs++[0] = pdp11_papertape_reader_ww_unibus_device(&pr);
    pdp.periphs++[0] = pdp11_teletype_ww_unibus_device(&tty);

    pdp11_papertape_reader_load(&pr, "res/papertapes/absolute_loader.ptap");

    run_console_ui(&console, &pr, &tty);

    pdp11_teletype_uninit(&tty);
    pdp11_papertape_reader_uninit(&pr);
    pdp11_uninit(&pdp);

    return 0;
}
