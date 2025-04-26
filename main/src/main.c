#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <locale.h>
#include <ncursesw/ncurses.h>
#include <unistd.h>

#include "pdp11/pdp11_console.h"
#include "pdp11/pdp11_papertape_reader.h"

#define COLOR_PAIR_WHITE  1
#define COLOR_PAIR_RED    2
#define COLOR_PAIR_PURPLE 3

#define CUSTOM_COLOR_WHITE  10
#define CUSTOM_COLOR_RED    11
#define CUSTOM_COLOR_PURPLE 12

WINDOW *tty_win;
WINDOW *cons_win;
WINDOW *pr_win;

static inline int imax(int const x, int const y) { return x > y ? x : y; }

typedef struct Layout {
    int y, x, h, w;
} Layout;
typedef struct UiLayout {
    Layout tty, cons, pr;
} UiLayout;
UiLayout ui_layout(int const h, int const w) {
    Layout const
        tty = {.y = 1, .x = 1, .h = imax(h / 3 - 1, 1), .w = imax(w - 2, 1)},
        cons =
            {.y = tty.y + tty.h + 1,
             .x = 1,
             .h = imax(h - tty.y - tty.h - 1 - 1, 1),
             .w = imax(w * 3 / 4 - 1, 1)},
        pr = {
            .y = cons.y,
            .x = cons.x + cons.w + 1,
            .h = cons.h,
            .w = imax(w - cons.x - cons.w - 1 - 1, 1)
        };
    return (UiLayout){.tty = tty, .cons = cons, .pr = pr};
}

void draw_borders(UiLayout const *const layout) {
    mvvline(
        layout->cons.y,
        layout->cons.x + layout->cons.w,
        ACS_VLINE,
        layout->cons.h
    );
    mvhline(
        layout->tty.y + layout->tty.h,
        layout->tty.x,
        ACS_HLINE,
        layout->tty.w
    );
    box(stdscr, 0, 0);
    mvaddch(layout->cons.y - 1, layout->cons.x + layout->cons.w, ACS_TTEE);
    mvaddch(
        layout->cons.y + layout->cons.h,
        layout->cons.x + layout->cons.w,
        ACS_BTEE
    );
    mvaddch(layout->tty.y + layout->tty.h, layout->tty.x - 1, ACS_LTEE);
    mvaddch(
        layout->tty.y + layout->tty.h,
        layout->tty.x + layout->tty.w,
        ACS_RTEE
    );
}

int ui_init() {
    setlocale(LC_ALL, "");

    initscr();
    if (!has_colors()) {
        endwin();
        printf("colors are not supported!\n");
        return 0;
    }

    cbreak(), noecho();

    keypad(stdscr, TRUE);
    curs_set(0);

    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

    start_color();
    use_default_colors();
    if (can_change_color()) {
        init_color(
            CUSTOM_COLOR_WHITE,
            0xEE * 1000 / 0xFF,
            0xEE * 1000 / 0xFF,
            0xEE * 1000 / 0xFF
        );
        init_color(
            CUSTOM_COLOR_RED,
            0xCC * 1000 / 0xFF,
            0x44 * 1000 / 0xFF,
            0x44 * 1000 / 0xFF
        );
        init_color(
            CUSTOM_COLOR_PURPLE,
            0x88 * 1000 / 0xFF,
            0x44 * 1000 / 0xFF,
            0x55 * 1000 / 0xFF
        );

        init_pair(COLOR_PAIR_WHITE, CUSTOM_COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_PAIR_RED, CUSTOM_COLOR_RED, COLOR_BLACK);
        init_pair(COLOR_PAIR_PURPLE, CUSTOM_COLOR_PURPLE, COLOR_BLACK);
    } else {
        init_pair(COLOR_PAIR_WHITE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_PAIR_RED, COLOR_RED, COLOR_BLACK);
        init_pair(COLOR_PAIR_PURPLE, COLOR_MAGENTA, COLOR_BLACK);
    }

    UiLayout const layout = ui_layout(getmaxy(stdscr), getmaxx(stdscr));
    tty_win = newwin(layout.tty.h, layout.tty.w, layout.tty.y, layout.tty.x);
    cons_win =
        newwin(layout.cons.h, layout.cons.w, layout.cons.y, layout.cons.x);
    pr_win = newwin(layout.pr.h, layout.pr.w, layout.pr.y, layout.pr.x);

    keypad(tty_win, TRUE);
    keypad(cons_win, TRUE);
    keypad(pr_win, TRUE);

    draw_borders(&layout);

    mvwprintw(
        tty_win,
        0,
        0,
        "Hello from the pdp11emu, a PDP-11/20 emulator!\n"
        "Teletype is not implemented yet, so this placeholder is here.\n"
        "\n"
        "Keyboard shortcut list:\n"
        " - Q|q\t: quit\n"
    );

    wrefresh(tty_win);
    wrefresh(cons_win);
    wrefresh(pr_win);

    return 0;
}
void ui_uninit() {
    delwin(tty_win);
    delwin(cons_win);
    delwin(pr_win);

    nocbreak(), echo();
    endwin();
}

void ui_update() {
    touchwin(tty_win);
    touchwin(cons_win);
    touchwin(pr_win);
    wrefresh(tty_win);
    wrefresh(cons_win);
    wrefresh(pr_win);
}

int main() {
    Pdp11 pdp = {0};
    UNROLL(pdp11_init(&pdp));

    Pdp11Console console = {0};
    pdp11_console_init(&console, &pdp);

    Pdp11PapertapeReader pr = {0};
    pdp11_papertape_reader_init(
        &pr,
        &pdp.unibus,
        PDP11_PAPERTAPE_READER_ADDR,
        PDP11_PAPERTAPE_READER_INTR_VEC,
        PDP11_PAPERTAPE_READER_INTR_PRIORITY
    );
    pdp.unibus.devices[PDP11_FIRST_USER_DEVICE + 0] =
        pdp11_papertape_reader_ww_unibus_device(&pr);

    pdp11_papertape_reader_load(&pr, "res/papertapes/absolute_loader.ptap");

    {
        ui_init();
        bool do_run = true;
        while (do_run) {
            int const ch = getch();
            switch (ch) {
            case 'Q':
            case 'q': do_run = false; break;

            case KEY_MOUSE: {
                MEVENT event;
                if (getmouse(&event) != OK) break;

                // Handle mouse events here later
                // For now, just print event details for debugging
                mvwprintw(
                    tty_win,
                    7,
                    0,
                    "Mouse event (y:%i, x:%i): %x\n",
                    event.y,
                    event.x,
                    event.bstate
                );
                wrefresh(tty_win);
            } break;
            default: {
                // Handle keyboard input here later
                // For now, just print the key pressed for debugging
                mvwprintw(tty_win, 6, 0, "Key event: %i", ch);
                wrefresh(tty_win);
            } break;
            }

            ui_update();

            usleep(66 * 1000);
        }
        ui_uninit();
    }

    pdp11_papertape_reader_uninit(&pr);
    pdp11_uninit(&pdp);

    return 0;
}
