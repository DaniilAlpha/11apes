#ifndef PDP11_UI_H
#define PDP11_UI_H

#include <stdbool.h>

#include "pdp11/pdp11_console.h"
#include "pdp11/pdp11_papertape_reader.h"

// --- Constants ---
#define PDP11_UI_QUIT       -1
#define PDP11_UI_LOAD_TAPE  -2
#define PDP11_UI_REDRAW     -3
#define PDP11_UI_INPUT_NONE 0

// --- Structures ---

// Opaque structure to hold UI state
typedef struct Pdp11Ui Pdp11Ui;

// --- Function Prototypes ---

/**
 * @brief Initializes the ncurses UI.
 *
 * Sets up ncurses, creates windows, defines colors, and enables mouse/keyboard
 * input.
 *
 * @param console Pointer to the Pdp11Console instance.
 * @param reader Pointer to the Pdp11PapertapeReader instance.
 * @return Pointer to the initialized Pdp11Ui structure, or NULL on failure.
 */
Pdp11Ui *pdp11_ui_init(Pdp11Console *console, Pdp11PapertapeReader *reader);

/**
 * @brief Cleans up and uninitializes the ncurses UI.
 *
 * Restores the terminal to its original state.
 *
 * @param ui Pointer to the Pdp11Ui structure.
 */
void pdp11_ui_uninit(Pdp11Ui *ui);

/**
 * @brief Draws the entire PDP-11 UI to the screen.
 *
 * Calls individual drawing functions for console, teletype, and paper tape
 * reader.
 *
 * @param ui Pointer to the Pdp11Ui structure.
 */
void pdp11_ui_draw(Pdp11Ui *ui);

/**
 * @brief Forces a full redraw of the UI on the next `pdp11_ui_draw` call.
 *
 * Useful after operations that might corrupt the screen, like external input.
 *
 * @param ui Pointer to the Pdp11Ui structure.
 */
void pdp11_ui_force_redraw(Pdp11Ui *ui);

/**
 * @brief Handles user input (keyboard and mouse).
 *
 * Processes input events, updates emulator state via console/reader functions,
 * and provides visual feedback for interactions.
 *
 * @param ui Pointer to the Pdp11Ui structure.
 * @return An integer code indicating the action taken or required:
 * - PDP11_UI_QUIT: User requested to quit.
 * - PDP11_UI_LOAD_TAPE: User requested to load a new paper tape.
 * - PDP11_UI_REDRAW: Screen resize detected, redraw needed.
 * - PDP11_UI_INPUT_NONE: No significant input processed.
 * - Any other positive value: A character input from the keyboard (potentially
 * for teletype).
 */
int pdp11_ui_handle_input(Pdp11Ui *ui);

/**
 * @brief Prompts the user to enter a filename for the paper tape.
 *
 * Temporarily exits ncurses mode to allow normal terminal input, then restores
 * it. IMPORTANT: Call `pdp11_ui_force_redraw` after this function returns.
 *
 * @param ui Pointer to the Pdp11Ui structure.
 * @param buffer Buffer to store the entered filename.
 * @param buffer_size Size of the filename buffer.
 * @return true if a filename was entered, false otherwise (e.g., user pressed
 * Enter without typing).
 */
bool pdp11_ui_get_tape_filename(Pdp11Ui *ui, char *buffer, size_t buffer_size);

/**
 * @brief Adds a line of text to the teletype output area.
 *
 * Handles scrolling if necessary.
 *
 * @param ui Pointer to the Pdp11Ui structure.
 * @param text The string to add to the teletype output.
 */
void pdp11_ui_add_teletype_line(Pdp11Ui *ui, char const *text);

#endif
