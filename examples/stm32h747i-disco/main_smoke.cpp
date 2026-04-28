// PARITY: rlvgl/examples/stm32h747i-disco (CM7 boot path).
// LVGL:   N/A — pre-bring-up smoke target.
// DELTA:  none.
//
// PLAT-02a §5.6. The smallest possible main(): boot the CM7, land
// here, loop on WFE. Acceptance is "halt under probe-rs at the WFE
// and read $pc inside this symbol range".

int main() {
    for (;;) {
        asm volatile ("wfe");
    }
}
