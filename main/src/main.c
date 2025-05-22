#include <stdlib.h>

#include <ctype.h>
#include <ncurses.h>
#include <signal.h>
#include <unistd.h>

#include "pdp11/pdp11.h"
#include "pdp11/pdp11_console.h"
#include "pdp11/pdp11_papertape_reader.h"
#include "pdp11/pdp11_teletype.h"

#define COLOR_PAIR_OFF      1
#define COLOR_PAIR_ON       2
#define COLOR_PAIR_LABEL    3
#define COLOR_PAIR_SELECTED 4
#define COLOR_PAIR_VALUE    8

typedef enum SelectableElement {
    SELECT_POWER,

    SELECT_SWITCH_REG_BIT_0,
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
    SELECT_SWITCH_REG_BIT_15,

    SELECT_LOAD_ADDR,
    SELECT_EXAMINE,
    SELECT_CONTINUE,
    SELECT_ENABLE,
    SELECT_START,
    SELECT_DEPOSIT,

    SELECT_COUNT,
} SelectableElement;

/*************
 ** helpers **
 *************/

void draw_bit(
    int const y,
    int const x,
    bool const is_on,
    bool const is_selected
) {
    attr_t const attr = is_selected
                          ? COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE
                      : is_on ? COLOR_PAIR(COLOR_PAIR_ON) | A_REVERSE
                              : COLOR_PAIR(COLOR_PAIR_OFF);
    attron(attr);
    mvprintw(y, x, is_on ? "(*)" : "( )");
    attroff(attr);
}

void draw_register(
    int const y,
    int const x,
    char const *const label,
    uint16_t const value
) {
    mvprintw(y - 1, x, "%s", label);
    attron(COLOR_PAIR(COLOR_PAIR_VALUE));
    printw(" = %06o", value);
    attroff(COLOR_PAIR(COLOR_PAIR_VALUE));

    for (int i = 0; i < 16; ++i) {
        bool const is_on = (value >> (15 - i)) & 1;
        draw_bit(y, x + i * 3 + (i + 2) / 3, is_on, false);
    }
}

void draw_switch_register(
    int const y,
    int const x,
    uint16_t const value,
    SelectableElement const current_selection
) {
    mvprintw(y - 2, x, "SWITCH REGISTER");
    attron(COLOR_PAIR(COLOR_PAIR_VALUE));
    printw(" = %06o", value);
    attroff(COLOR_PAIR(COLOR_PAIR_VALUE));

    for (unsigned i = 0; i < 16; ++i) {
        bool const is_on = (value >> (15 - i)) & 1;
        bool const is_selected =
            current_selection - SELECT_SWITCH_REG_BIT_0 == i;
        attr_t const attr = is_selected
                              ? COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE
                          : is_on ? COLOR_PAIR(COLOR_PAIR_ON)
                                  : COLOR_PAIR(COLOR_PAIR_OFF);
        mvprintw(y - 1, x + i * 3 + (i + 2) / 3 + 1, "%d", 15 - i);
        attron(attr);
        mvprintw(y, x + i * 3 + (i + 2) / 3, is_on ? "'-'" : "|_|");
        attroff(attr);
    }
}

void draw_status_lights(int y, int const x, Pdp11Console const *const console) {
    attr_t const attr_on = COLOR_PAIR(COLOR_PAIR_ON) | A_REVERSE;
    attr_t const attr_off = COLOR_PAIR(COLOR_PAIR_OFF);

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

void draw_power_switch(
    int const y,
    int const x,
    Pdp11ConsolePowerControl const power_state,
    bool const is_selected
) {
    mvprintw(y, x, "OFF");
    mvprintw(y - 1, x + 3, "PWR");
    mvprintw(y, x + 6, "LCK");

    attr_t const attr = COLOR_PAIR(COLOR_PAIR_SELECTED) | A_REVERSE;
    if (is_selected) attron(attr);
    switch (power_state) {
    case PDP11_CONSOLE_POWER_CONTROL_OFF: mvprintw(y, x + 3 + 1, "<"); break;
    case PDP11_CONSOLE_POWER_CONTROL_POWER: mvprintw(y, x + 3 + 1, "^"); break;
    case PDP11_CONSOLE_POWER_CONTROL_LOCK: mvprintw(y, x + 3 + 1, ">"); break;
    }
    if (is_selected) attroff(attr);
}

void draw_control_buttons(
    int const y,
    int x,
    Pdp11Console const *const console,
    SelectableElement const current_selection
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

/**********
 ** main **
 **********/

void run_console_ui(
    Pdp11 *const pdp,
    Pdp11PapertapeReader *const pr,
    Pdp11Teletype *const tty
) {
    initscr();
    if (!has_colors()) {
        endwin();
        fprintf(stderr, "Your terminal does not support colors!\n"),
            fflush(stderr);
        exit(1);
    }

    cbreak(), noecho();

    keypad(stdscr, TRUE);
    curs_set(0);

    timeout(100);

    start_color();
    use_default_colors();
    init_pair(COLOR_PAIR_OFF, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_PAIR_ON, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_PAIR_LABEL, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_PAIR_SELECTED, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(5, COLOR_WHITE, COLOR_WHITE);
    init_pair(6, COLOR_WHITE, COLOR_WHITE);
    init_pair(7, COLOR_WHITE, COLOR_WHITE);
    init_pair(COLOR_PAIR_VALUE, COLOR_RED, COLOR_BLACK);

    SelectableElement current_selection = SELECT_SWITCH_REG_BIT_15;
    bool quit = false;

    bool is_in_input_mode = false;
    while (!quit) {
        Pdp11ConsolePowerControl const power_state =
            pdp11_console_power_control(&pdp->console);
        uint16_t const switch_reg =
            pdp11_console_switch_register(&pdp->console);
        uint16_t const addr_reg =
            pdp11_console_address_indicator(&pdp->console);
        uint16_t const data_reg = pdp11_console_data_indicator(&pdp->console);

        clear();

        attron(A_BOLD | COLOR_PAIR(COLOR_PAIR_LABEL));
        mvprintw(0, (COLS - 28) / 2, "PDP-11/20 Operator Console");
        attroff(A_BOLD | COLOR_PAIR(COLOR_PAIR_LABEL));

        draw_power_switch(
            12,
            2,
            power_state,
            current_selection == SELECT_POWER
        );

        draw_register(3, 14, "ADDRESS REGISTER", addr_reg);
        draw_register(6, 14, "DATA", data_reg);
        draw_status_lights(3, 72, &pdp->console);

        draw_switch_register(12, 14, switch_reg, current_selection);

        draw_control_buttons(12, 72, &pdp->console, current_selection);

        attron(COLOR_PAIR(COLOR_PAIR_LABEL));
        mvprintw(
            LINES - 4,
            0,
            is_in_input_mode ? "\n\n\n"
                               " * Tab/^N/^I - back into normal mode\t"
                               " * Anykey - teletype input\n"
                             : " * Left/Right - move around\t"
                               " * Up/Down/Enter/Space - toggle switch\n"
                               " * O/P - dec/inc power lvl\t"
                               " * L, E, C, H, S, D - load, exam, cont, "
                               "enbl/halt, start, deposit\n"
                               " * B - autoinsert bootloader\t"
                               " * T - change paper tape;\n"
                               " * Tab/^I - into insert mode\t"
                               " * ^D - create memory dump\t"
                               " * Q - quit\n"
        );
        attroff(COLOR_PAIR(COLOR_PAIR_LABEL));

        refresh();

        int const ch = getch();
        if (is_in_input_mode) {
            switch (ch) {
            case 'I' & 0x1F:
            case 'N' & 0x1F: is_in_input_mode = false; break;

            case '\n':
            case KEY_ENTER:
                pdp11_teletype_putc(tty, '\r');
                pdp11_teletype_putc(tty, '\n');
                break;

            case '\b':
            case KEY_DL:
            case KEY_BACKSPACE: pdp11_teletype_putc(tty, 0x7F); break;

            case 'U' & 0x1F:
            case 'P' & 0x1F: pdp11_teletype_putc(tty, ch); break;

            case ' ' ... '~': pdp11_teletype_putc(tty, toupper(ch)); break;
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
                    ch == KEY_UP
                        ? pdp11_console_next_power_control(&pdp->console)
                        : pdp11_console_prev_power_control(&pdp->console);
                } else {
                    if (current_selection >= SELECT_SWITCH_REG_BIT_0 &&
                        current_selection <= SELECT_SWITCH_REG_BIT_15) {
                        int bit_index =
                            15 - (current_selection - SELECT_SWITCH_REG_BIT_0);
                        pdp11_console_toggle_control_switch(
                            &pdp->console,
                            bit_index
                        );
                    } else if (current_selection == SELECT_LOAD_ADDR) {
                        pdp11_console_press_load_addr(&pdp->console);
                    } else if (current_selection == SELECT_EXAMINE) {
                        pdp11_console_press_examine(&pdp->console);
                    } else if (current_selection == SELECT_DEPOSIT) {
                        pdp11_console_press_deposit(&pdp->console);
                    } else if (current_selection == SELECT_CONTINUE) {
                        pdp11_console_press_continue(&pdp->console);
                    } else if (current_selection == SELECT_ENABLE) {
                        pdp11_console_toggle_enable(&pdp->console);
                    } else if (current_selection == SELECT_START) {
                        pdp11_console_press_start(&pdp->console);
                    } else if (current_selection == SELECT_POWER) {
                        pdp11_console_next_power_control(&pdp->console);
                    }
                }
            } break;

            case 'Q':
            case 'q': quit = true; break;
            case 'P':
            case 'p': pdp11_console_next_power_control(&pdp->console); break;
            case 'O':
            case 'o': pdp11_console_prev_power_control(&pdp->console); break;

            case 'L':
            case 'l': pdp11_console_press_load_addr(&pdp->console); break;
            case 'E':
            case 'e': pdp11_console_press_examine(&pdp->console); break;
            case 'C':
            case 'c': pdp11_console_press_continue(&pdp->console); break;
            case 'H':
            case 'h': pdp11_console_toggle_enable(&pdp->console); break;
            case 'S':
            case 's': pdp11_console_press_start(&pdp->console); break;
            case 'D':
            case 'd': pdp11_console_press_deposit(&pdp->console); break;

            case 'B':
            case 'b': pdp11_console_insert_bootloader(&pdp->console); break;

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

                if (pdp11_ram_save(&pdp->ram) != Ok) {
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

void ignore(int _) {}
int main() {
    signal(SIGINT, ignore);

    Pdp11 pdp = {0};
    UNROLL(pdp11_init(&pdp));

    Pdp11PapertapeReader pr = {0};
    UNROLL_CLEANUP(
        pdp11_papertape_reader_init(
            &pr,
            &pdp.unibus,
            PDP11_PAPERTAPE_READER_ADDR,
            PDP11_PAPERTAPE_READER_INTR_VEC,
            PDP11_PAPERTAPE_READER_INTR_PRIORITY
        ),
        {
            pdp11_uninit(&pdp);
            fprintf(stderr, "error initializing papertape reader!\n"),
                fflush(stderr);
        }
    );

    Pdp11Teletype tty = {0};
    UNROLL_CLEANUP(
        pdp11_teletype_init(
            &tty,
            &pdp.unibus,
            PDP11_TELETYPE_ADDR,
            PDP11_TELETYPE_KEYBOARD_INTR_VEC,
            PDP11_TELETYPE_PRINTER_INTR_VEC,
            PDP11_TELETYPE_INTR_PRIORITY
        ),
        {
            pdp11_papertape_reader_uninit(&pr);
            pdp11_uninit(&pdp);
            fprintf(stderr, "error initializing papertape reader!\n"),
                fflush(stderr);
        }
    );

    pdp.periphs++[0] = pdp11_papertape_reader_ww_unibus_device(&pr);
    pdp.periphs++[0] = pdp11_teletype_ww_unibus_device(&tty);

    run_console_ui(&pdp, &pr, &tty);

    pdp11_teletype_uninit(&tty);
    pdp11_papertape_reader_uninit(&pr);
    pdp11_uninit(&pdp);

    return 0;
}
