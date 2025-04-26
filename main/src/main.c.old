#include "conviniences.h"
#include "pdp11/pdp11.h"
#include "pdp11/pdp11_console.h"
#include "pdp11/pdp11_papertape_reader.h"
#include "pdp11_ui.h"

int main() {
    Pdp11 pdp = {0};
    UNROLL(pdp11_init(&pdp));

    Pdp11Console console = {0};
    pdp11_console_init(&console, &pdp);

    // Pdp11Rom rom = {0};
    // FILE *const file = fopen("res/m9342-248f1.bin", "r");
    // if (!file) return 1;
    // UNROLL(pdp11_rom_init_file(&rom, 0077744, file));
    // fclose(file);
    // pdp.unibus.devices[PDP11_FIRST_USER_DEVICE + 0] =
    //     pdp11_rom_ww_unibus_device(&rom);

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
        Pdp11Ui *const ui = pdp11_ui_init(&console, &pr);
        if (ui) {
            bool do_run = true;
            while (do_run) {
                pdp11_ui_draw(ui);

                int const action = pdp11_ui_handle_input(ui);
                switch (action) {
                case PDP11_UI_QUIT: do_run = false; break;
                case PDP11_UI_LOAD_TAPE: {
                    char buf[4096] = {0};
                    if (!pdp11_ui_get_tape_filename(ui, buf, lenof(buf))) break;
                    pdp11_papertape_reader_load(&pr, buf);
                    pdp11_ui_force_redraw(ui);
                } break;
                case PDP11_UI_REDRAW: continue;
                default:;  // TODO teletype action
                }
            }
        }
        pdp11_ui_uninit(ui);
    }

    pdp11_papertape_reader_uninit(&pr);
    // pdp11_rom_uninit(&rom);
    pdp11_uninit(&pdp);

    return 0;
}
