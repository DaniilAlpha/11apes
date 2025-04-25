#include "pdp11_ui.h"

#include <stdlib.h>
#include <string.h>

#include <locale.h>
#include <unistd.h>
#include <wchar.h>

#define NCURSES_WIDECHAR (1)
#include <ncursesw/ncurses.h>

// --- Constants ---
#define CONSOLE_MIN_WIDTH    85
#define CONSOLE_MIN_HEIGHT   18
#define TELETYPE_MIN_HEIGHT  5
#define PAPERTAPE_MIN_HEIGHT 5
#define SWITCH_WIDTH         5
#define INTERACTION_DELAY_MS 100  // Delay to show interaction feedback

// --- Color Pair Definitions ---
#define COLOR_PAIR_DEFAULT 1
#define COLOR_PAIR_WHITE   2  // #EEEEEE
#define COLOR_PAIR_RED     3  // #CC4455
#define COLOR_PAIR_PURPLE  4  // #884455
#define COLOR_PAIR_ACTIVE  5  // Highlight (e.g., Black on White)
#define COLOR_PAIR_LABEL   6  // Grey on Black (for labels)
#define COLOR_PAIR_YELLOW  7  // Added for Busy status
#define COLOR_PAIR_GREEN   8  // Added for Done status

// --- Internal Structures ---

// Structure to hold information about an interactive element (switch/button)
typedef struct {
    int y, x;           // Position within the parent window
    int height, width;  // Dimensions
    int id;             // Unique identifier for this element
    bool is_momentary;  // True if it's a button, false if it's a toggle switch
} UiElement;

// Structure to hold the state of the UI
struct Pdp11Ui {
    Pdp11Console *console;
    Pdp11PapertapeReader *reader;

    WINDOW *console_win;
    WINDOW *teletype_win;
    WINDOW *papertape_win;

    int term_height, term_width;
    bool needs_full_redraw;

    // Interactive elements (coordinates relative to console_win)
    UiElement power_switch[3];  // OFF, POWER, LOCK positions (only POWER is
                                // clickable)
    UiElement switch_reg_toggles[16];
    UiElement load_addr_button;
    UiElement exam_button;
    UiElement cont_button;
    UiElement enable_switch;
    UiElement start_button;
    UiElement deposit_button;

    // Interactive elements (coordinates relative to papertape_win)
    UiElement load_tape_button;

    // State for temporary highlighting
    int highlighted_element_id;
    struct timespec highlight_end_time;

    // Teletype buffer (simple ring buffer)
    char **teletype_buffer;
    int teletype_buffer_size;
    int teletype_buffer_head;
    int teletype_lines_written;

    // Paper tape filename
    char current_tape_file[256];
};

// Element IDs (must be unique)
enum ElementId {
    ID_NONE = 0,
    ID_POWER_SWITCH,
    ID_SWITCH_REG_0,
    ID_SWITCH_REG_1,
    ID_SWITCH_REG_2,
    ID_SWITCH_REG_3,
    ID_SWITCH_REG_4,
    ID_SWITCH_REG_5,
    ID_SWITCH_REG_6,
    ID_SWITCH_REG_7,
    ID_SWITCH_REG_8,
    ID_SWITCH_REG_9,
    ID_SWITCH_REG_10,
    ID_SWITCH_REG_11,
    ID_SWITCH_REG_12,
    ID_SWITCH_REG_13,
    ID_SWITCH_REG_14,
    ID_SWITCH_REG_15,
    ID_LOAD_ADDR,
    ID_EXAM,
    ID_CONT,
    ID_ENABLE,
    ID_START,
    ID_DEPOSIT,
    ID_LOAD_TAPE_BTN
};

// --- Forward Declarations ---
static void init_colors();
static void create_windows(Pdp11Ui *ui);
static void destroy_windows(Pdp11Ui *ui);
static void define_interactive_elements(Pdp11Ui *ui);
static void draw_console(Pdp11Ui *ui);
static void draw_teletype(Pdp11Ui *ui);
static void draw_papertape(Pdp11Ui *ui);
static void draw_switch(
    WINDOW *win,
    int y,
    int x,
    int width,
    bool state,
    bool is_active,
    chtype color_pair
);
static void draw_button(
    WINDOW *win,
    int y,
    int x,
    char const *label,
    bool is_active,
    chtype color_pair
);
static void draw_indicator_lights(
    WINDOW *win,
    int y,
    int x,
    uint16_t value,
    int bits,
    bool is_address
);
static void
draw_status_light(WINDOW *win, int y, int x, char const *label, bool state);
static void set_highlight(Pdp11Ui *ui, int element_id);
static bool is_highlighted(Pdp11Ui *ui, int element_id);
static int
get_element_at(Pdp11Ui *ui, int win_y, int win_x, WINDOW *target_win);
static void handle_console_interaction(Pdp11Ui *ui, int element_id);
static void handle_papertape_interaction(Pdp11Ui *ui, int element_id);
static void sleep_ms(long milliseconds);

// --- Global UI State (Pointer) ---
// This allows signal handlers (like SIGWINCH) to access the UI state if needed,
// though direct signal handling isn't implemented here for simplicity.
static Pdp11Ui *g_ui_instance = NULL;

// --- Function Implementations ---

Pdp11Ui *pdp11_ui_init(Pdp11Console *console, Pdp11PapertapeReader *reader) {
    if (!console || !reader) {
        fprintf(stderr, "Error: Console or Reader pointer is NULL.\n");
        return NULL;
    }

    setlocale(LC_ALL, "");  // For wide character support

    if (initscr() == NULL) {
        fprintf(stderr, "Error initializing ncurses.\n");
        return NULL;
    }

    Pdp11Ui *ui = calloc(1, sizeof(Pdp11Ui));
    if (!ui) {
        endwin();
        fprintf(stderr, "Error allocating memory for UI structure.\n");
        return NULL;
    }
    g_ui_instance = ui;  // Store globally

    ui->console = console;
    ui->reader = reader;
    ui->needs_full_redraw = true;
    ui->highlighted_element_id = ID_NONE;
    strcpy(ui->current_tape_file, "None");

    cbreak();              // Line buffering disabled
    noecho();              // Don't echo() while we should be in control
    keypad(stdscr, TRUE);  // Enable function keys, arrow keys, etc.
    curs_set(0);           // Hide cursor
    timeout(10);           // Non-blocking input with 10ms timeout

    // Initialize colors
    if (has_colors()) {
        start_color();
        if (can_change_color()) {
            init_colors();  // Initialize custom colors
        } else {
            // Use default colors if cannot change
            init_pair(COLOR_PAIR_DEFAULT, COLOR_WHITE, COLOR_BLACK);
            init_pair(COLOR_PAIR_WHITE, COLOR_WHITE, COLOR_BLACK);  // Approx
            init_pair(COLOR_PAIR_RED, COLOR_RED, COLOR_BLACK);
            init_pair(COLOR_PAIR_PURPLE, COLOR_MAGENTA, COLOR_BLACK);  // Approx
            init_pair(COLOR_PAIR_ACTIVE, COLOR_BLACK, COLOR_WHITE);
            init_pair(
                COLOR_PAIR_LABEL,
                COLOR_CYAN,
                COLOR_BLACK
            );  // Different fallback
            init_pair(
                COLOR_PAIR_YELLOW,
                COLOR_YELLOW,
                COLOR_BLACK
            );  // Fallback Yellow
            init_pair(
                COLOR_PAIR_GREEN,
                COLOR_GREEN,
                COLOR_BLACK
            );  // Fallback Green
        }
    } else {
        // Handle lack of color support (use attributes or monochrome)
        init_pair(COLOR_PAIR_DEFAULT, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_PAIR_WHITE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_PAIR_RED, COLOR_WHITE, COLOR_BLACK);     // Fallback
        init_pair(COLOR_PAIR_PURPLE, COLOR_WHITE, COLOR_BLACK);  // Fallback
        init_pair(COLOR_PAIR_ACTIVE, COLOR_BLACK, COLOR_WHITE);
        init_pair(COLOR_PAIR_LABEL, COLOR_WHITE, COLOR_BLACK);   // Fallback
        init_pair(COLOR_PAIR_YELLOW, COLOR_WHITE, COLOR_BLACK);  // Fallback
        init_pair(COLOR_PAIR_GREEN, COLOR_WHITE, COLOR_BLACK);   // Fallback
    }

    // Enable mouse events
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);  // Report mouse events immediately

    // Create windows based on initial terminal size
    getmaxyx(stdscr, ui->term_height, ui->term_width);
    create_windows(ui);
    define_interactive_elements(
        ui
    );  // Define elements after windows are created

    // Allocate teletype buffer
    // Ensure teletype_win is valid before getting size
    if (ui->teletype_win) {
        ui->teletype_buffer_size =
            getmaxy(ui->teletype_win) - 2;  // -2 for border
        if (ui->teletype_buffer_size < 1) ui->teletype_buffer_size = 1;
        ui->teletype_buffer = calloc(ui->teletype_buffer_size, sizeof(char *));
        ui->teletype_buffer_head = 0;
        ui->teletype_lines_written = 0;

        // Add initial teletype message only if buffer allocation succeeded
        if (ui->teletype_buffer) {
            pdp11_ui_add_teletype_line(ui, "Teletype Output Area");
            pdp11_ui_add_teletype_line(ui, "--------------------");
            pdp11_ui_add_teletype_line(ui, "Shortcuts:");
            pdp11_ui_add_teletype_line(ui, " 0-9,a-f: Toggle Switch Reg 0-15");
            pdp11_ui_add_teletype_line(
                ui,
                " L: Load Addr  E: Exam   X: Cont"
            );  // Changed C->X
            pdp11_ui_add_teletype_line(
                ui,
                " N: Enable     S: Start  D: Deposit"
            );
            pdp11_ui_add_teletype_line(
                ui,
                " P: Power      Z: Bootstrap Load"
            );  // Changed B->Z
            pdp11_ui_add_teletype_line(ui, " T: Load Tape File");
            pdp11_ui_add_teletype_line(ui, " F10: Quit");
        } else {
            // Handle buffer allocation failure if necessary
            mvwprintw(
                stdscr,
                ui->term_height - 1,
                0,
                "Error: Could not allocate TTY buffer."
            );
        }
    } else {
        // Handle window creation failure if necessary
        mvwprintw(
            stdscr,
            ui->term_height - 1,
            0,
            "Error: Could not create UI windows."
        );
        // No teletype buffer allocated
    }

    return ui;
}

void pdp11_ui_uninit(Pdp11Ui *ui) {
    if (!ui) return;

    // Free teletype buffer
    if (ui->teletype_buffer) {
        for (int i = 0; i < ui->teletype_buffer_size; ++i) {
            free(ui->teletype_buffer[i]);
        }
        free(ui->teletype_buffer);
    }

    destroy_windows(ui);
    mousemask(0, NULL);  // Disable mouse events
    curs_set(1);         // Show cursor
    nocbreak();
    echo();
    endwin();  // Restore terminal
    free(ui);
    g_ui_instance = NULL;
}

void pdp11_ui_force_redraw(Pdp11Ui *ui) {
    if (ui) {
        ui->needs_full_redraw = true;
        // Clear the physical screen immediately to avoid artifacts
        // when coming back from external input.
        clear();
        refresh();
    }
}

void pdp11_ui_draw(Pdp11Ui *ui) {
    if (!ui) return;

    // Check if highlight duration has expired
    if (ui->highlighted_element_id != ID_NONE) {
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        if (current_time.tv_sec > ui->highlight_end_time.tv_sec ||
            (current_time.tv_sec == ui->highlight_end_time.tv_sec &&
             current_time.tv_nsec >= ui->highlight_end_time.tv_nsec)) {
            ui->highlighted_element_id = ID_NONE;
            // No need to force redraw here, the next draw cycle will handle it
        }
    }

    // Check for terminal resize
    int h, w;
    getmaxyx(stdscr, h, w);
    if (h != ui->term_height || w != ui->term_width) {
        pdp11_ui_force_redraw(ui);  // Force redraw on resize
        // Update dimensions immediately for the current draw cycle if forced
        // redraw happens
        ui->term_height = h;
        ui->term_width = w;
    }

    if (ui->needs_full_redraw) {
        // Recreate windows and elements based on new size
        destroy_windows(ui);
        create_windows(ui);
        // Check if windows were created successfully before proceeding
        if (!ui->console_win || !ui->teletype_win || !ui->papertape_win) {
            // Handle error - maybe display a message and prevent further
            // drawing
            endwin();  // Exit ncurses cleanly
            fprintf(stderr, "Terminal too small to create UI windows.\n");
            // Exit or return an error state to the caller if possible
            // For now, just prevent further drawing by returning
            return;
        }

        define_interactive_elements(
            ui
        );  // Redefine elements for new window sizes

        // Reallocate teletype buffer if size changed
        int new_tty_buffer_size = getmaxy(ui->teletype_win) - 2;
        if (new_tty_buffer_size < 1) new_tty_buffer_size = 1;
        if (new_tty_buffer_size != ui->teletype_buffer_size) {
            // Basic reallocation - might lose some history on shrink
            char **new_buffer = calloc(new_tty_buffer_size, sizeof(char *));
            if (new_buffer) {
                int copy_count =
                    (new_tty_buffer_size < ui->teletype_buffer_size)
                        ? new_tty_buffer_size
                        : ui->teletype_buffer_size;
                // Simple copy, not preserving ring buffer order perfectly on
                // resize
                for (int i = 0; i < copy_count; ++i) {
                    // This copy logic is basic and might scramble order on
                    // resize. A more robust solution would involve copying
                    // based on head/written count.
                    int old_idx = (ui->teletype_buffer_head + i) %
                                  ui->teletype_buffer_size;
                    // Check if old buffer exists and has content at index
                    if (ui->teletype_buffer && ui->teletype_buffer[old_idx]) {
                        new_buffer[i] = strdup(ui->teletype_buffer[old_idx]);
                    }
                }
                // Free old buffer
                if (ui->teletype_buffer) {
                    for (int i = 0; i < ui->teletype_buffer_size; ++i) {
                        free(ui->teletype_buffer[i]);
                    }
                    free(ui->teletype_buffer);
                }

                ui->teletype_buffer = new_buffer;
                ui->teletype_buffer_size = new_tty_buffer_size;
                // Reset head/count after resize for simplicity here
                ui->teletype_buffer_head = 0;
                ui->teletype_lines_written =
                    (ui->teletype_lines_written > new_tty_buffer_size)
                        ? new_tty_buffer_size
                        : ui->teletype_lines_written;

            }  // else: allocation failed, keep old buffer (or handle error)
        }

        clear();  // Clear physical screen
        // Redraw borders immediately
        box(ui->console_win, 0, 0);
        box(ui->teletype_win, 0, 0);
        box(ui->papertape_win, 0, 0);
        wnoutrefresh(stdscr);  // Mark stdscr for refresh (borders)
        ui->needs_full_redraw = false;
    }

    // Ensure windows exist before drawing into them
    if (!ui->console_win || !ui->teletype_win || !ui->papertape_win) {
        return;  // Don't attempt to draw if windows are invalid
    }

    // Draw components into their windows
    werase(ui->console_win);
    werase(ui->teletype_win);
    werase(ui->papertape_win);

    box(ui->console_win, 0, 0);
    mvwprintw(ui->console_win, 0, 2, " PDP-11/20 Console ");
    draw_console(ui);

    box(ui->teletype_win, 0, 0);
    mvwprintw(ui->teletype_win, 0, 2, " Teletype ");
    draw_teletype(ui);

    box(ui->papertape_win, 0, 0);
    mvwprintw(ui->papertape_win, 0, 2, " Paper Tape Reader ");
    draw_papertape(ui);

    // Mark windows for refresh, then update the physical screen once
    wnoutrefresh(ui->console_win);
    wnoutrefresh(ui->teletype_win);
    wnoutrefresh(ui->papertape_win);
    doupdate();
}

int pdp11_ui_handle_input(Pdp11Ui *ui) {
    if (!ui) return PDP11_UI_QUIT;

    int ch = wgetch(stdscr);  // Read input (non-blocking due to timeout)

    switch (ch) {
    case KEY_F(10): return PDP11_UI_QUIT;

    case KEY_RESIZE:
        pdp11_ui_force_redraw(ui);
        return PDP11_UI_REDRAW;  // Indicate redraw needed

    // --- Keyboard Shortcuts for Console ---
    case 'p':
    case 'P':
        pdp11_console_next_power_control(ui->console);
        set_highlight(ui, ID_POWER_SWITCH);
        break;
    case '0':
        pdp11_console_toggle_control_switch(ui->console, 0);
        set_highlight(ui, ID_SWITCH_REG_0);
        break;
    case '1':
        pdp11_console_toggle_control_switch(ui->console, 1);
        set_highlight(ui, ID_SWITCH_REG_1);
        break;
    case '2':
        pdp11_console_toggle_control_switch(ui->console, 2);
        set_highlight(ui, ID_SWITCH_REG_2);
        break;
    case '3':
        pdp11_console_toggle_control_switch(ui->console, 3);
        set_highlight(ui, ID_SWITCH_REG_3);
        break;
    case '4':
        pdp11_console_toggle_control_switch(ui->console, 4);
        set_highlight(ui, ID_SWITCH_REG_4);
        break;
    case '5':
        pdp11_console_toggle_control_switch(ui->console, 5);
        set_highlight(ui, ID_SWITCH_REG_5);
        break;
    case '6':
        pdp11_console_toggle_control_switch(ui->console, 6);
        set_highlight(ui, ID_SWITCH_REG_6);
        break;
    case '7':
        pdp11_console_toggle_control_switch(ui->console, 7);
        set_highlight(ui, ID_SWITCH_REG_7);
        break;
    case '8':
        pdp11_console_toggle_control_switch(ui->console, 8);
        set_highlight(ui, ID_SWITCH_REG_8);
        break;
    case '9':
        pdp11_console_toggle_control_switch(ui->console, 9);
        set_highlight(ui, ID_SWITCH_REG_9);
        break;
    case 'a':
    case 'A':
        pdp11_console_toggle_control_switch(ui->console, 10);
        set_highlight(ui, ID_SWITCH_REG_10);
        break;
    case 'b':
    case 'B':
        pdp11_console_toggle_control_switch(ui->console, 11);
        set_highlight(ui, ID_SWITCH_REG_11);
        break;
    case 'c':
    case 'C':
        pdp11_console_toggle_control_switch(ui->console, 12);
        set_highlight(ui, ID_SWITCH_REG_12);
        break;
    // Note: 'd' and 'e' are used for buttons, 'f' is switch 15
    case 'f':
    case 'F':
        pdp11_console_toggle_control_switch(ui->console, 15);
        set_highlight(ui, ID_SWITCH_REG_15);
        break;

    case 'l':
    case 'L':
        pdp11_console_press_load_addr(ui->console);
        set_highlight(ui, ID_LOAD_ADDR);
        break;
    case 'e':
    case 'E':
        pdp11_console_press_examine(ui->console);
        set_highlight(ui, ID_EXAM);
        break;
    // Remapped 'C' shortcut for Continue
    case 'x':
    case 'X':  // Using 'X' for Continue
        pdp11_console_press_continue(ui->console);
        set_highlight(ui, ID_CONT);
        break;
    case 'n':
    case 'N':
        pdp11_console_toggle_enable(ui->console);
        set_highlight(ui, ID_ENABLE);
        break;
    case 's':
    case 'S':
        pdp11_console_press_start(ui->console);
        set_highlight(ui, ID_START);
        break;
    case 'd':
    case 'D':
        pdp11_console_press_deposit(ui->console);
        set_highlight(ui, ID_DEPOSIT);
        break;
    // Remapped 'B' shortcut for Bootstrap
    case 'z':
    case 'Z':  // Using 'Z' for Bootstrap Load
        pdp11_console_insert_bootstrap(ui->console);
        // Indicate success? Maybe flash lights? Add TTY message?
        pdp11_ui_add_teletype_line(ui, "Bootstrap loaded via shortcut.");
        // No specific element to highlight for this action
        break;

    // --- Keyboard Shortcut for Paper Tape ---
    case 't':
    case 'T':
        // Check if papertape window exists before highlighting
        if (ui->papertape_win) { set_highlight(ui, ID_LOAD_TAPE_BTN); }
        return PDP11_UI_LOAD_TAPE;  // Signal main loop to handle file loading

    // --- Mouse Event Handling ---
    case KEY_MOUSE: {
        MEVENT event;
        if (getmouse(&event) == OK) {
            // Check if click is within any of our *valid* windows
            if (ui->console_win &&
                wenclose(ui->console_win, event.y, event.x)) {
                int element_id =
                    get_element_at(ui, event.y, event.x, ui->console_win);
                if (element_id != ID_NONE && (event.bstate & BUTTON1_PRESSED ||
                                              event.bstate & BUTTON1_CLICKED)) {
                    handle_console_interaction(ui, element_id);
                }
            } else if (ui->papertape_win &&
                       wenclose(ui->papertape_win, event.y, event.x)) {
                int element_id =
                    get_element_at(ui, event.y, event.x, ui->papertape_win);
                if (element_id != ID_NONE && (event.bstate & BUTTON1_PRESSED ||
                                              event.bstate & BUTTON1_CLICKED)) {
                    // Only handle papertape interaction if the element is the
                    // load button
                    if (element_id == ID_LOAD_TAPE_BTN) {
                        handle_papertape_interaction(ui, element_id);
                        // Also signal main loop to load tape
                        return PDP11_UI_LOAD_TAPE;
                    }
                }
            }
            // Ignore clicks in teletype window for now
        }
    } break;

    case ERR:
        // No input, just timeout, do nothing
        break;

    default:
        // Potentially handle other key presses for teletype input later
        // For now, just return the character
        if (ch > 0 && ch < 256) {  // Basic check for printable ASCII range
            return ch;
        }
        break;
    }

    // If highlight was set by keyboard/mouse (and not ERR), redraw immediately
    // for feedback This check prevents unnecessary redraws every timeout cycle
    // if highlight expired naturally
    bool just_highlighted =
        ui->highlighted_element_id != ID_NONE && ch != ERR &&
        ch != KEY_MOUSE;  // Don't redraw+sleep for mouse release/move
    if (just_highlighted) {
        pdp11_ui_draw(ui);               // Draw with highlight
        doupdate();                      // Update screen
        sleep_ms(INTERACTION_DELAY_MS);  // Pause briefly
        // Highlight will be turned off naturally by time check in pdp11_ui_draw
        // or immediately if the action toggled the state (handled in draw
        // logic)
    }

    return PDP11_UI_INPUT_NONE;  // No special action required by main loop
}

bool pdp11_ui_get_tape_filename(Pdp11Ui *ui, char *buffer, size_t buffer_size) {
    if (!ui || !buffer || buffer_size == 0) return false;

    // Temporarily leave ncurses mode to get terminal input
    def_prog_mode();  // Save current ncurses modes
    endwin();         // Temporarily end ncurses mode

    // Clear the input buffer before prompting
    // Read and discard any pending characters in stdin
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    printf("\nEnter paper tape file path (max %zu chars): ", buffer_size - 1);
    fflush(stdout);  // Ensure prompt is displayed

    bool success = false;
    if (fgets(buffer, buffer_size, stdin) != NULL) {
        // Remove trailing newline, if present
        buffer[strcspn(buffer, "\n")] = 0;
        if (strlen(buffer) > 0) {
            success = true;
            strncpy(
                ui->current_tape_file,
                buffer,
                sizeof(ui->current_tape_file) - 1
            );
            ui->current_tape_file[sizeof(ui->current_tape_file) - 1] =
                '\0';  // Ensure null termination
        } else {
            strcpy(ui->current_tape_file, "None");  // Reset if empty input
        }
    } else {
        // fgets failed or EOF
        strcpy(ui->current_tape_file, "None");  // Reset on failure
        clearerr(stdin);  // Clear error/EOF flags if necessary
        printf("\nInput error or cancelled.\n");  // Provide feedback
        sleep_ms(1000);                           // Pause briefly
    }

    // Return to ncurses mode
    reset_prog_mode();  // Restore saved ncurses modes
    // No need to call refresh() here, caller handles redraw via force_redraw

    // Caller should call pdp11_ui_force_redraw after this returns.

    return success;
}

void pdp11_ui_add_teletype_line(Pdp11Ui *ui, char const *text) {
    // Ensure UI and buffer are valid
    if (!ui || !text || !ui->teletype_buffer || ui->teletype_buffer_size <= 0)
        return;

    // Free the oldest line if buffer is full and about to be overwritten
    if (ui->teletype_lines_written >= ui->teletype_buffer_size) {
        // The head points to the *next* slot to write, which is the oldest if
        // full
        free(ui->teletype_buffer[ui->teletype_buffer_head]);
        ui->teletype_buffer[ui->teletype_buffer_head] = NULL;
    }

    // Add the new line (duplicate the string)
    ui->teletype_buffer[ui->teletype_buffer_head] = strdup(text);
    if (!ui->teletype_buffer[ui->teletype_buffer_head]) {
        // Handle strdup failure if necessary
        return;  // Failed to add line
    }

    // Advance the head pointer (wraps around)
    ui->teletype_buffer_head =
        (ui->teletype_buffer_head + 1) % ui->teletype_buffer_size;

    // Increment lines written count, capped at buffer size
    if (ui->teletype_lines_written < ui->teletype_buffer_size) {
        ui->teletype_lines_written++;
    }
}

// --- Static Helper Functions ---

static void init_colors() {
    // Define colors (0-1000 range)
    // PDP-11 Colors
    init_color(
        COLOR_WHITE,
        933,
        933,
        933
    );  // #EEEEEE (Terminals might approximate)
    init_color(COLOR_RED, 800, 267, 333);  // #CC4455
    init_color(
        COLOR_MAGENTA,
        533,
        267,
        333
    );  // #884455 (Using MAGENTA slot for Purple)
    // Other colors
    init_color(COLOR_BLACK, 0, 0, 0);
    init_color(COLOR_GREEN, 0, 600, 0);     // Standard Green
    init_color(COLOR_YELLOW, 800, 800, 0);  // Standard Yellow
    init_color(COLOR_BLUE, 0, 0, 800);
    init_color(COLOR_CYAN, 0, 600, 600);  // For labels

    // Define color pairs
    init_pair(
        COLOR_PAIR_DEFAULT,
        COLOR_WHITE,
        COLOR_BLACK
    );  // Default ncurses white
    init_pair(
        COLOR_PAIR_WHITE,
        COLOR_WHITE,
        COLOR_BLACK
    );  // Our defined white (#EEEEEE)
    init_pair(
        COLOR_PAIR_RED,
        COLOR_RED,
        COLOR_BLACK
    );  // Our defined red (#CC4455)
    init_pair(
        COLOR_PAIR_PURPLE,
        COLOR_MAGENTA,
        COLOR_BLACK
    );  // Our defined purple (#884455)
    init_pair(COLOR_PAIR_ACTIVE, COLOR_BLACK, COLOR_WHITE);  // Highlight
    init_pair(COLOR_PAIR_LABEL, COLOR_CYAN, COLOR_BLACK);    // Labels
    init_pair(
        COLOR_PAIR_YELLOW,
        COLOR_YELLOW,
        COLOR_BLACK
    );                                                      // Added Yellow pair
    init_pair(COLOR_PAIR_GREEN, COLOR_GREEN, COLOR_BLACK);  // Added Green pair
}

static void create_windows(Pdp11Ui *ui) {
    int term_h = ui->term_height;
    int term_w = ui->term_width;

    // Check minimum total size needed
    if (term_h <
            CONSOLE_MIN_HEIGHT + TELETYPE_MIN_HEIGHT + PAPERTAPE_MIN_HEIGHT ||
        term_w < CONSOLE_MIN_WIDTH) {
        // Cannot create windows, set them to NULL
        ui->console_win = NULL;
        ui->teletype_win = NULL;
        ui->papertape_win = NULL;
        return;  // Indicate failure by leaving windows NULL
    }

    // Basic layout: Console takes most space, Teletype and Papertape below it.
    // Ensure minimum dimensions.
    int console_h = (term_h * 2) / 3;
    if (console_h < CONSOLE_MIN_HEIGHT) console_h = CONSOLE_MIN_HEIGHT;
    // Ensure enough space is left for the other windows
    if (console_h > term_h - TELETYPE_MIN_HEIGHT - PAPERTAPE_MIN_HEIGHT) {
        console_h = term_h - TELETYPE_MIN_HEIGHT - PAPERTAPE_MIN_HEIGHT;
    }
    // Re-check console min height after adjustment
    if (console_h < CONSOLE_MIN_HEIGHT) console_h = CONSOLE_MIN_HEIGHT;

    int remaining_h = term_h - console_h;
    int teletype_h = remaining_h / 2;
    if (teletype_h < TELETYPE_MIN_HEIGHT) teletype_h = TELETYPE_MIN_HEIGHT;

    int papertape_h = term_h - console_h - teletype_h;
    // If rounding/min height adjustments caused papertape_h to be too small,
    // adjust teletype
    if (papertape_h < PAPERTAPE_MIN_HEIGHT) {
        papertape_h = PAPERTAPE_MIN_HEIGHT;
        teletype_h = term_h - console_h - papertape_h;
        // Final check if teletype is now too small (shouldn't happen with
        // initial total check)
        if (teletype_h < TELETYPE_MIN_HEIGHT) {
            // This case indicates the terminal is fundamentally too small,
            // handled by the initial check. If logic error allows reaching
            // here, it might still fail.
            teletype_h = TELETYPE_MIN_HEIGHT;
        }
    }

    int console_w = term_w;  // Use full width for console

    // Create windows
    ui->console_win = newwin(console_h, console_w, 0, 0);
    ui->teletype_win = newwin(teletype_h, term_w, console_h, 0);
    ui->papertape_win = newwin(papertape_h, term_w, console_h + teletype_h, 0);

    // Check creation success again (paranoia)
    if (!ui->console_win || !ui->teletype_win || !ui->papertape_win) {
        destroy_windows(ui);  // Clean up any that were created
        return;               // Indicate failure
    }

    // Enable keypad and mouse for subwindows too
    keypad(ui->console_win, TRUE);
    keypad(ui->teletype_win, TRUE);
    keypad(ui->papertape_win, TRUE);
    // Mouse reporting is global via stdscr, no need to set per window
}

static void destroy_windows(Pdp11Ui *ui) {
    if (ui->console_win) {
        delwin(ui->console_win);
        ui->console_win = NULL;
    }
    if (ui->teletype_win) {
        delwin(ui->teletype_win);
        ui->teletype_win = NULL;
    }
    if (ui->papertape_win) {
        delwin(ui->papertape_win);
        ui->papertape_win = NULL;
    }
}

// Define positions and sizes of interactive elements relative to their windows
static void define_interactive_elements(Pdp11Ui *ui) {
    // Ensure windows exist before defining elements relative to them
    if (!ui || !ui->console_win || !ui->papertape_win) return;

    int console_h, console_w;
    getmaxyx(ui->console_win, console_h, console_w);

    // --- Console Elements ---
    int y_offset = 2;  // Starting row
    int x_offset = 3;  // Starting column

    // Power Switch (visual only, interaction TBD or via 'P' key)
    // For simplicity, mouse interaction only targets the 'POWER' state button
    // area
    ui->power_switch[0] = (UiElement
    ){y_offset, x_offset + 10, 1, 5, ID_NONE, false};  // OFF label pos
    ui->power_switch[1] = (UiElement){y_offset + 1,
                                      x_offset + 10,
                                      1,
                                      5,
                                      ID_POWER_SWITCH,
                                      false};  // POWER clickable area
    ui->power_switch[2] = (UiElement
    ){y_offset + 2, x_offset + 10, 1, 5, ID_NONE, false};  // LOCK label pos

    // Switch Register (16 switches)
    int switch_y = console_h - 4;  // Place near bottom
    int switch_x_start = x_offset + 1;
    int switch_spacing = SWITCH_WIDTH + 1;
    // Ensure switches fit within the window width
    int max_switches = (console_w - switch_x_start - x_offset) / switch_spacing;
    if (max_switches < 16) {
        // Handle case where window is too narrow for all switches
        // Option 1: Don't draw/define them (simplest)
        // Option 2: Draw fewer switches
        // Option 3: Use multiple rows (more complex)
        // For now, let's assume CONSOLE_MIN_WIDTH is sufficient.
    }

    for (int i = 0; i < 16; ++i) {
        ui->switch_reg_toggles[i] = (UiElement){
            switch_y,
            switch_x_start + i * switch_spacing,
            3,  // Height: includes label space above/below
            SWITCH_WIDTH,
            ID_SWITCH_REG_0 + i,
            false  // Toggle switch
        };
    }

    // Control Switches/Buttons (Right side)
    int control_x = console_w - 15;  // Position from right edge
    if (control_x < x_offset + 40)
        control_x = x_offset + 40;  // Prevent overlap if too narrow
    int control_y = y_offset + 5;
    int control_spacing = 2;

    ui->load_addr_button =
        (UiElement){control_y, control_x, 1, 10, ID_LOAD_ADDR, true};
    control_y += control_spacing;
    ui->exam_button = (UiElement){control_y, control_x, 1, 10, ID_EXAM, true};
    control_y += control_spacing;
    ui->deposit_button =
        (UiElement){control_y, control_x, 1, 10, ID_DEPOSIT, true};
    control_y += control_spacing;
    ui->cont_button = (UiElement){control_y, control_x, 1, 10, ID_CONT, true};
    control_y += control_spacing;
    ui->enable_switch =
        (UiElement){control_y, control_x, 1, 10, ID_ENABLE, false};
    control_y += control_spacing;
    ui->start_button = (UiElement){control_y, control_x, 1, 10, ID_START, true};

    // --- Paper Tape Elements ---
    int pt_h, pt_w;
    getmaxyx(ui->papertape_win, pt_h, pt_w);
    ui->load_tape_button =
        (UiElement){pt_h - 2, pt_w / 2 - 8, 1, 16, ID_LOAD_TAPE_BTN, true};
}

static void draw_console(Pdp11Ui *ui) {
    if (!ui || !ui->console_win) return;

    WINDOW *win = ui->console_win;
    int w;  // Height 'h' is not used in this function's logic
    getmaxyx(
        win,
        *(&w),
        w
    );  // Get only width, using a trick to satisfy getmaxyx signature

    // --- Static Labels and Branding ---
    wattron(win, COLOR_PAIR(COLOR_PAIR_WHITE) | A_BOLD);
    mvwprintw(win, 2, 3, "digital");  // Branding
    wattroff(win, COLOR_PAIR(COLOR_PAIR_WHITE) | A_BOLD);
    mvwprintw(win, 3, 5, "pdp11/20");

    // --- Power Switch Visual ---
    int power_x = ui->power_switch[0].x;  // Use defined position
    int power_y_off = ui->power_switch[0].y;
    int power_y_power = ui->power_switch[1].y;
    int power_y_lock = ui->power_switch[2].y;
    Pdp11ConsolePowerControl power_state =
        pdp11_console_power_control(ui->console);
    mvwprintw(win, power_y_off, power_x, " OFF ");
    mvwprintw(win, power_y_power, power_x, "POWER");
    mvwprintw(win, power_y_lock, power_x, " LOCK");
    // Indicate current state visually (e.g., box around it)
    int current_power_y = -1;
    switch (power_state) {
    case PDP11_CONSOLE_POWER_CONTROL_OFF: current_power_y = power_y_off; break;
    case PDP11_CONSOLE_POWER_CONTROL_POWER:
        current_power_y = power_y_power;
        break;
    case PDP11_CONSOLE_POWER_CONTROL_LOCK:
        current_power_y = power_y_lock;
        break;
    }
    if (current_power_y != -1) {
        bool is_active = is_highlighted(ui, ID_POWER_SWITCH);
        wattron(
            win,
            COLOR_PAIR(is_active ? COLOR_PAIR_ACTIVE : COLOR_PAIR_PURPLE)
        );
        // Draw a simple indicator next to the active label
        mvwaddch(win, current_power_y, power_x - 2, ACS_DIAMOND);
        wattroff(
            win,
            COLOR_PAIR(is_active ? COLOR_PAIR_ACTIVE : COLOR_PAIR_PURPLE)
        );
    }

    // --- Status Lights ---
    int light_y = 2;
    int light_x = power_x + 10;  // Position relative to power switch
    draw_status_light(
        win,
        light_y++,
        light_x,
        "RUN ",
        pdp11_console_run_light(ui->console)
    );
    draw_status_light(
        win,
        light_y++,
        light_x,
        "BUS ",
        pdp11_console_bus_light(ui->console)
    );
    light_y++;  // Gap
    draw_status_light(
        win,
        light_y++,
        light_x,
        "FETCH",
        pdp11_console_fetch_light(ui->console)
    );
    draw_status_light(
        win,
        light_y++,
        light_x,
        "EXEC ",
        pdp11_console_exec_light(ui->console)
    );
    light_y++;  // Gap
    draw_status_light(
        win,
        light_y++,
        light_x,
        "SOURCE",
        pdp11_console_source_light(ui->console)
    );
    draw_status_light(
        win,
        light_y++,
        light_x,
        "DEST",
        pdp11_console_destination_light(ui->console)
    );
    light_y++;  // Gap
    draw_status_light(
        win,
        light_y++,
        light_x,
        "ADDR",
        pdp11_console_address_light(ui->console)
    );

    // --- Address / Data Indicators ---
    int data_ind_y = light_y + 1;  // Below lights
    int data_ind_x = light_x + 8;
    int addr_ind_y = data_ind_y + 3;
    int addr_ind_x = data_ind_x;

    wattron(win, COLOR_PAIR(COLOR_PAIR_LABEL));
    mvwprintw(win, data_ind_y - 1, data_ind_x, "DATA");
    mvwprintw(win, addr_ind_y - 1, addr_ind_x, "ADDRESS");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_LABEL));

    uint16_t data_val = pdp11_console_data_indicator(ui->console);
    uint16_t addr_val = pdp11_console_address_indicator(ui->console);

    draw_indicator_lights(win, data_ind_y, data_ind_x, data_val, 16, false);
    draw_indicator_lights(
        win,
        addr_ind_y,
        addr_ind_x,
        addr_val,
        16,
        true
    );  // PDP-11/20 has 16 address lights

    // --- Switch Register ---
    uint16_t switch_reg = pdp11_console_switch_register(ui->console);
    wattron(win, COLOR_PAIR(COLOR_PAIR_LABEL));
    mvwprintw(
        win,
        ui->switch_reg_toggles[0].y - 1,
        ui->switch_reg_toggles[0].x,
        "SWITCH REGISTER"
    );
    wattroff(win, COLOR_PAIR(COLOR_PAIR_LABEL));

    for (int i = 0; i < 16; ++i) {
        bool state = (switch_reg >> i) & 1;
        UiElement el = ui->switch_reg_toggles[i];
        bool is_active = is_highlighted(ui, el.id);
        // Draw label below switch
        wattron(win, COLOR_PAIR(COLOR_PAIR_LABEL));
        mvwprintw(
            win,
            el.y + 2,
            el.x + el.width / 2 - (i < 10 ? 0 : 1),
            "%X",
            i
        );  // Adjust position slightly for 2-digit hex
        wattroff(win, COLOR_PAIR(COLOR_PAIR_LABEL));
        // Draw the switch itself
        draw_switch(
            win,
            el.y,
            el.x,
            el.width,
            state,
            is_active,
            COLOR_PAIR_PURPLE
        );
    }

    // --- Control Switches/Buttons ---
    wattron(win, COLOR_PAIR(COLOR_PAIR_LABEL));
    mvwprintw(
        win,
        ui->load_addr_button.y - 1,
        ui->load_addr_button.x,
        "CONTROLS"
    );
    wattroff(win, COLOR_PAIR(COLOR_PAIR_LABEL));

    draw_button(
        win,
        ui->load_addr_button.y,
        ui->load_addr_button.x,
        "LOAD ADR",
        is_highlighted(ui, ID_LOAD_ADDR),
        COLOR_PAIR_PURPLE
    );
    draw_button(
        win,
        ui->exam_button.y,
        ui->exam_button.x,
        "EXAM",
        is_highlighted(ui, ID_EXAM),
        COLOR_PAIR_PURPLE
    );
    draw_button(
        win,
        ui->deposit_button.y,
        ui->deposit_button.x,
        "DEPOSIT",
        is_highlighted(ui, ID_DEPOSIT),
        COLOR_PAIR_PURPLE
    );
    draw_button(
        win,
        ui->cont_button.y,
        ui->cont_button.x,
        "CONT",
        is_highlighted(ui, ID_CONT),
        COLOR_PAIR_PURPLE
    );
    draw_switch(
        win,
        ui->enable_switch.y,
        ui->enable_switch.x,
        ui->enable_switch.width,
        pdp11_console_enable_switch(ui->console),
        is_highlighted(ui, ID_ENABLE),
        COLOR_PAIR_PURPLE
    );
    mvwprintw(
        win,
        ui->enable_switch.y + 1,
        ui->enable_switch.x + ui->enable_switch.width + 1,
        "ENABLE"
    );  // Label for enable switch (aligned better)
    draw_button(
        win,
        ui->start_button.y,
        ui->start_button.x,
        "START",
        is_highlighted(ui, ID_START),
        COLOR_PAIR_PURPLE
    );
}

static void draw_teletype(Pdp11Ui *ui) {
    if (!ui || !ui->teletype_win || !ui->teletype_buffer) return;
    WINDOW *win = ui->teletype_win;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);
    int display_lines = max_y - 2;   // Available lines inside border
    if (display_lines <= 0) return;  // Cannot draw if no space

    wattron(win, COLOR_PAIR(COLOR_PAIR_WHITE));  // Use standard white for TTY

    // Draw lines from the ring buffer
    int lines_to_draw = (ui->teletype_lines_written < display_lines)
                          ? ui->teletype_lines_written
                          : display_lines;
    for (int i = 0; i < lines_to_draw; ++i) {
        // Calculate index in buffer, starting from the oldest visible line
        int buffer_idx = (ui->teletype_buffer_head + ui->teletype_buffer_size -
                          lines_to_draw + i) %
                         ui->teletype_buffer_size;
        if (ui->teletype_buffer[buffer_idx]) {
            // Truncate line if it's too long for the window width
            mvwprintw(
                win,
                i + 1,
                1,
                "%.*s",
                max_x - 2,
                ui->teletype_buffer[buffer_idx]
            );
        }
    }
    wattroff(win, COLOR_PAIR(COLOR_PAIR_WHITE));
}

static void draw_papertape(Pdp11Ui *ui) {
    if (!ui || !ui->papertape_win) return;
    WINDOW *win = ui->papertape_win;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    // Display current tape file
    wattron(win, COLOR_PAIR(COLOR_PAIR_LABEL));
    mvwprintw(win, 1, 2, "Current Tape:");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_LABEL));
    wattron(win, COLOR_PAIR(COLOR_PAIR_WHITE));
    mvwprintw(
        win,
        1,
        16,
        "%.*s",
        max_x - 18,
        ui->current_tape_file
    );  // Truncate if needed
    wattroff(win, COLOR_PAIR(COLOR_PAIR_WHITE));

    // Draw status indicators (Placeholder - requires access to reader status)
    // TODO: Replace these flags with actual calls to reader status functions
    // Example: bool busy = pdp11_papertape_reader_is_busy(ui->reader);
    //          bool done = pdp11_papertape_reader_is_done(ui->reader);
    //          bool error = pdp11_papertape_reader_has_error(ui->reader);
    // For now, using placeholder variables:
    bool busy = false;
    bool done = false;
    bool error = false;
    // You'll need to add functions to pdp11_papertape_reader.h/c to get these
    // states. e.g., bool pdp11_papertape_reader_is_busy(Pdp11PapertapeReader
    // const * const self);

    int status_y = 2;
    int status_x = 2;
    if (error) {
        wattron(win, COLOR_PAIR(COLOR_PAIR_RED) | A_BOLD);
        mvwprintw(win, status_y, status_x, "[ERROR]");
        wattroff(win, COLOR_PAIR(COLOR_PAIR_RED) | A_BOLD);
    } else if (busy) {
        wattron(
            win,
            COLOR_PAIR(COLOR_PAIR_YELLOW) | A_BLINK
        );  // Use defined yellow pair
        mvwprintw(win, status_y, status_x, "[BUSY] ");
        wattroff(win, COLOR_PAIR(COLOR_PAIR_YELLOW) | A_BLINK);
    } else if (done) {
        wattron(win, COLOR_PAIR(COLOR_PAIR_GREEN));  // Use defined green pair
        mvwprintw(win, status_y, status_x, "[DONE] ");
        wattroff(win, COLOR_PAIR(COLOR_PAIR_GREEN));
    } else {
        wattron(win, COLOR_PAIR(COLOR_PAIR_WHITE));
        mvwprintw(win, status_y, status_x, "[IDLE] ");
        wattroff(win, COLOR_PAIR(COLOR_PAIR_WHITE));
    }

    // Draw "Load New Tape" button
    UiElement btn = ui->load_tape_button;
    // Ensure button position is valid before drawing
    if (btn.y < max_y && btn.x + btn.width < max_x) {
        draw_button(
            win,
            btn.y,
            btn.x,
            "[ Load Tape (T) ]",
            is_highlighted(ui, ID_LOAD_TAPE_BTN),
            COLOR_PAIR_PURPLE
        );
    }
}

// Helper to draw a single toggle switch
static void draw_switch(
    WINDOW *win,
    int y,
    int x,
    int width,
    bool state,
    bool is_active,
    chtype color_pair
) {
    chtype attr = COLOR_PAIR(is_active ? COLOR_PAIR_ACTIVE : color_pair);
    wattron(win, attr);
    // Simple representation: [^^^] or [vvv]
    // Ensure drawing within bounds
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);
    if (y + 2 >= max_y || x + width >= max_x) {
        wattroff(win, attr);
        return;  // Cannot draw switch, out of bounds
    }

    mvwhline(win, y, x, ACS_HLINE, width);
    mvwhline(win, y + 2, x, ACS_HLINE, width);
    mvwaddch(win, y + 1, x, ACS_VLINE);
    mvwaddch(win, y + 1, x + width - 1, ACS_VLINE);

    // Calculate center position for symbols
    int symbol_x = x + (width - 3) / 2;
    if (symbol_x < x + 1) symbol_x = x + 1;  // Ensure within bounds

    // Clear middle area before drawing symbols
    mvwprintw(win, y + 1, x + 1, "%*s", width - 2, "");

    if (state) {                                 // Up position
        mvwaddstr(win, y + 1, symbol_x, "^^^");  // Centered caret-like symbol
    } else {                                     // Down position
        mvwaddstr(win, y + 1, symbol_x, "vvv");  // Centered 'v' symbol
    }
    wattroff(win, attr);
}

// Helper to draw a momentary button
static void draw_button(
    WINDOW *win,
    int y,
    int x,
    char const *label,
    bool is_active,
    chtype color_pair
) {
    chtype attr = COLOR_PAIR(is_active ? COLOR_PAIR_ACTIVE : color_pair);
    int len = strlen(label);
    int display_width = len + 2;  // Add padding for brackets

    // Ensure drawing within bounds
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);
    if (y >= max_y || x + display_width >= max_x) {
        return;  // Cannot draw button, out of bounds
    }

    wattron(win, attr);
    mvwprintw(win, y, x, "[%s]", label);
    wattroff(win, attr);
}

// Helper to draw indicator lights (like Address/Data)
static void draw_indicator_lights(
    WINDOW *win,
    int y,
    int x,
    uint16_t value,
    int bits,
    bool is_address
) {
    // Use wide characters for circles if available, otherwise fallback
    // ● and ○
    wchar_t *const on_char = L"\u25CF\0", *const off_char = L"\u25CB\0";
    char const *const on_fallback = "O", *const off_fallback = ".";

    bool wide_supported = true;  // Assume true initially

    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    for (int i = 0; i < bits; ++i) {
        int bit_pos = bits - 1 - i;  // Display MSB on the left
        bool is_on = (value >> bit_pos) & 1;
        int current_x = x + i * 2;  // Spacing for circles

        // Check bounds before drawing
        if (y + 1 >= max_y || current_x + 1 >= max_x)
            continue;  // Skip if out of bounds

        // Add labels below every 4 bits for readability
        if (i % 4 == 0) {
            wattron(win, COLOR_PAIR(COLOR_PAIR_LABEL));
            mvwprintw(win, y + 1, current_x, "%d", bit_pos);  // Show bit number
            wattroff(win, COLOR_PAIR(COLOR_PAIR_LABEL));
        }

        wattron(win, COLOR_PAIR(is_on ? COLOR_PAIR_RED : COLOR_PAIR_DEFAULT));
        if (is_on) {
            wattron(win, A_BOLD);  // Make 'on' lights bolder
        }

        // Attempt to add wide char, fallback if error
        cchar_t wc;
        if (wide_supported && setcchar(
                                  &wc,
                                  is_on ? on_char : off_char,
                                  is_on ? A_BOLD : A_NORMAL,
                                  is_on ? COLOR_PAIR_RED : COLOR_PAIR_DEFAULT,
                                  NULL
                              ) == OK) {
            mvwadd_wch(win, y, current_x, &wc);
        } else {
            wide_supported = false;  // Fallback for the rest if one fails
            // Apply bold attribute separately for fallback
            if (is_on) wattron(win, A_BOLD);
            mvwprintw(win, y, current_x, is_on ? on_fallback : off_fallback);
            if (is_on) wattroff(win, A_BOLD);
        }

        if (is_on) {
            wattroff(
                win,
                A_BOLD
            );  // Ensure bold is off if applied via setcchar or fallback
        }
        wattroff(win, COLOR_PAIR(is_on ? COLOR_PAIR_RED : COLOR_PAIR_DEFAULT));
    }
}

// Helper to draw a single status light (like RUN, BUS)
static void
draw_status_light(WINDOW *win, int y, int x, char const *label, bool state) {
    int label_len = strlen(label);
    wchar_t *const light_char = state ? L"\u25CF\0" : L"\u25CB\0";  // ● or ○
    char const *const fallback_char = state ? "O" : ".";
    cchar_t wc;

    // Check bounds
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);
    if (y >= max_y || x + label_len + 2 >= max_x)
        return;  // Need space for label, space, light

    wattron(win, COLOR_PAIR(COLOR_PAIR_LABEL));
    mvwprintw(win, y, x, "%s", label);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_LABEL));

    wattron(win, COLOR_PAIR(state ? COLOR_PAIR_RED : COLOR_PAIR_DEFAULT));
    attr_t attrs = state ? A_BOLD : A_NORMAL;

    if (setcchar(
            &wc,
            light_char,
            attrs,
            state ? COLOR_PAIR_RED : COLOR_PAIR_DEFAULT,
            NULL
        ) == OK) {
        mvwadd_wch(win, y, x + label_len + 1, &wc);
    } else {
        // Apply attributes manually for fallback
        if (state) wattron(win, A_BOLD);
        mvwprintw(win, y, x + label_len + 1, fallback_char);
        if (state) wattroff(win, A_BOLD);
    }

    wattroff(win, COLOR_PAIR(state ? COLOR_PAIR_RED : COLOR_PAIR_DEFAULT));
}

// Set highlight state for an element
static void set_highlight(Pdp11Ui *ui, int element_id) {
    if (!ui || element_id == ID_NONE) return;  // Don't highlight "NONE"
    ui->highlighted_element_id = element_id;
    // Set highlight end time
    clock_gettime(CLOCK_MONOTONIC, &ui->highlight_end_time);
    long nsec =
        ui->highlight_end_time.tv_nsec + (INTERACTION_DELAY_MS * 1000000L);
    ui->highlight_end_time.tv_sec += nsec / 1000000000L;
    ui->highlight_end_time.tv_nsec = nsec % 1000000000L;
}

// Check if an element is currently highlighted
static bool is_highlighted(Pdp11Ui *ui, int element_id) {
    if (!ui || element_id == ID_NONE) return false;
    // Check ID and if current time is before the end time
    if (ui->highlighted_element_id == element_id) {
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        return current_time.tv_sec < ui->highlight_end_time.tv_sec ||
               (current_time.tv_sec == ui->highlight_end_time.tv_sec &&
                current_time.tv_nsec < ui->highlight_end_time.tv_nsec);
    }
    return false;
}

// Find which interactive element (if any) is at the given window coordinates
static int
get_element_at(Pdp11Ui *ui, int event_y, int event_x, WINDOW *target_win) {
    if (!ui || !target_win) return ID_NONE;  // Check target_win validity

    int win_y, win_x;
    // Convert screen coordinates to window coordinates
    // Check if the event coordinates are actually within the window first
    if (!wenclose(target_win, event_y, event_x)) { return ID_NONE; }
    // Now get relative coordinates
    getbegyx(target_win, win_y, win_x);  // Should succeed if wenclose was true
    int rel_y = event_y - win_y;
    int rel_x = event_x - win_x;

    // Check elements within the target window
    if (target_win == ui->console_win) {
        // Check Power switch (only POWER position is clickable)
        UiElement pwr = ui->power_switch[1];
        if (rel_y >= pwr.y && rel_y < pwr.y + pwr.height && rel_x >= pwr.x &&
            rel_x < pwr.x + pwr.width)
            return pwr.id;

        // Check Switch Register toggles
        for (int i = 0; i < 16; ++i) {
            UiElement el = ui->switch_reg_toggles[i];
            // Check slightly larger area for easier clicking around the visual
            // switch
            if (rel_y >= el.y && rel_y < el.y + el.height && rel_x >= el.x &&
                rel_x < el.x + el.width)
                return el.id;
        }

        // Check Control buttons/switches
        UiElement controls[] = {
            ui->load_addr_button,
            ui->exam_button,
            ui->deposit_button,
            ui->cont_button,
            ui->enable_switch,
            ui->start_button
        };
        for (size_t i = 0; i < sizeof(controls) / sizeof(controls[0]); ++i) {
            UiElement el = controls[i];
            // Check the line the button/switch is on
            if (rel_y == el.y && rel_x >= el.x &&
                rel_x < el.x + el.width +
                            2) {  // Allow clicking slightly past label
                // Special check for enable switch label area
                if (el.id == ID_ENABLE && rel_x >= el.x + el.width) {
                    // Check if click is on the "ENABLE" label text next to the
                    // switch
                    if (rel_y == el.y + 1 &&
                        rel_x < el.x + el.width + 1 + (int)strlen("ENABLE")) {
                        return el.id;
                    }
                } else if (rel_x <
                           el.x + el.width) {  // Click on button/switch itself
                    return el.id;
                }
            }
        }

    } else if (target_win == ui->papertape_win) {
        UiElement btn = ui->load_tape_button;
        if (rel_y == btn.y && rel_x >= btn.x && rel_x < btn.x + btn.width)
            return btn.id;
    }

    return ID_NONE;  // No element found at this position
}

// Handle interaction for console elements
static void handle_console_interaction(Pdp11Ui *ui, int element_id) {
    if (!ui || !ui->console) return;

    set_highlight(ui, element_id);  // Set highlight immediately

    if (element_id >= ID_SWITCH_REG_0 && element_id <= ID_SWITCH_REG_15) {
        pdp11_console_toggle_control_switch(
            ui->console,
            element_id - ID_SWITCH_REG_0
        );
    } else {
        switch (element_id) {
        case ID_POWER_SWITCH:
            // Cycle power (assuming click always means 'next state')
            pdp11_console_next_power_control(ui->console);
            break;
        case ID_LOAD_ADDR: pdp11_console_press_load_addr(ui->console); break;
        case ID_EXAM: pdp11_console_press_examine(ui->console); break;
        case ID_DEPOSIT: pdp11_console_press_deposit(ui->console); break;
        case ID_CONT: pdp11_console_press_continue(ui->console); break;
        case ID_ENABLE: pdp11_console_toggle_enable(ui->console); break;
        case ID_START:
            pdp11_console_press_start(ui->console);
            break;
            // Add other console elements if needed
        }
    }
    // Highlight feedback is handled in the main input loop after this returns
}

// Handle interaction for papertape elements
static void handle_papertape_interaction(Pdp11Ui *ui, int element_id) {
    if (!ui) return;
    set_highlight(ui, element_id);  // Set highlight immediately

    switch (element_id) {
    case ID_LOAD_TAPE_BTN:
        // Action is triggered by the return value PDP11_UI_LOAD_TAPE
        // in pdp11_ui_handle_input. No direct action here needed,
        // just setting the highlight.
        break;
    }
    // Highlight feedback is handled in the main input loop after this returns
}

// Simple cross-platform sleep function (milliseconds)
static void sleep_ms(long milliseconds) { usleep(milliseconds * 1000); }
