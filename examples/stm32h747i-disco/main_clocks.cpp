// PARITY: rlvgl/examples/stm32h747i-disco BOOT.md § "Boot Sequence
//         Snapshot" steps 1–5 (modulo SDRAM init, which is PLAT-02c).
// LVGL:   N/A.
// DELTA:  none.
//
// PLAT-02b §12 acceptance smoke target. Calls disco::clocks::init()
// then disco::pinmux::mux_fmc_pins(); each writes its own breadcrumb
// at completion. main() then loops on WFE — a halted target should
// read 0xA11C_0007 at 0x3800_0300.

#include "disco/clocks.hpp"
#include "disco/pinmux.hpp"

int main() {
    lvglpp::disco::clocks::init();         // breadcrumb 0xA11C_0005
    lvglpp::disco::pinmux::mux_fmc_pins(); // breadcrumb 0xA11C_0007

    for (;;) {
        asm volatile ("wfe");
    }
}
