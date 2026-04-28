// PARITY: rlvgl/examples/stm32h747i-disco (cortex-m-rt Reset).
// LVGL:   N/A — bring-up code below the LVGL surface.
// DELTA:  C++20 path; reset handler additionally calls
//         __libc_init_array() so static constructors run.
//
// PLAT-02a §5.3, §5.5. Vector table + reset sequence + default
// fault handlers for the STM32H747I-DISCO CM7.

#include <cstdint>

extern "C" {

// Linker-provided.
extern std::uint32_t _estack;
extern std::uint32_t __sidata;
extern std::uint32_t __sdata;
extern std::uint32_t __edata;
extern std::uint32_t __sbss;
extern std::uint32_t __ebss;

int main();

// --- Reset / fault handlers -----------------------------------------

[[noreturn]] void __lvglpp_disco_reset_main() {
    // 1. .data: copy LMA -> VMA.
    for (std::uint32_t* dst = &__sdata, *src = &__sidata; dst < &__edata; )
        *dst++ = *src++;

    // 2. .bss: zero.
    for (std::uint32_t* p = &__sbss; p < &__ebss; )
        *p++ = 0;

    // 3. C++ static constructors. Walk .init_array directly so we
    //    don't pull in newlib's __libc_init_array → _init dependency
    //    chain (we're freestanding; there is no _init).
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();
    for (auto* p = __init_array_start; p < __init_array_end; ++p)
        (*p)();

    // 4. main(). The pragma silences -Wpedantic's "ISO C++ forbids
    //    taking address of function `::main`" — calling main from a
    //    freestanding reset path is exactly what cortex-m-rt does in
    //    Rust, and is unambiguous on a hosted-but-no-libc target.
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    main();
    #pragma GCC diagnostic pop
    for (;;) asm volatile ("wfe");
}

[[gnu::naked, noreturn]] void Reset_Handler() {
    asm volatile (
        "ldr  r0, =_estack                   \n"
        "msr  msp, r0                        \n"
        "bl   __lvglpp_disco_reset_main      \n"
        "b    .                              \n"
    );
}

[[noreturn]] void Default_Handler() {
    // Faults break in place — never reset. Mirrors rlvgl BOOT.md
    // § "Fault trapping" recommendation.
    for (;;) asm volatile ("bkpt 0");
}

// Each named handler is weak-aliased to Default_Handler so a
// later sub-phase can override (e.g. PLAT-02f overrides
// USART1_IRQHandler) without touching this file. The aliased
// declaration must repeat [[noreturn]] to match Default_Handler;
// otherwise -Wmissing-attributes fires under -Wall.
#define WEAK_DEFAULT [[gnu::weak, gnu::alias("Default_Handler"), gnu::nothrow, noreturn]]

WEAK_DEFAULT void NMI_Handler();
WEAK_DEFAULT void HardFault_Handler();
WEAK_DEFAULT void MemManage_Handler();
WEAK_DEFAULT void BusFault_Handler();
WEAK_DEFAULT void UsageFault_Handler();
WEAK_DEFAULT void SVC_Handler();
WEAK_DEFAULT void DebugMon_Handler();
WEAK_DEFAULT void PendSV_Handler();
WEAK_DEFAULT void SysTick_Handler();

// STM32H7 peripheral IRQ slots (RM0399 §11.1.4 vector table).
// All 224 slots default to Default_Handler. Sub-phases override.
#define IRQ(name) WEAK_DEFAULT void name##_IRQHandler()

IRQ(WWDG);                IRQ(PVD_AVD);            IRQ(TAMP_STAMP);
IRQ(RTC_WKUP);            IRQ(FLASH);              IRQ(RCC);
IRQ(EXTI0);               IRQ(EXTI1);              IRQ(EXTI2);
IRQ(EXTI3);               IRQ(EXTI4);              IRQ(DMA1_Stream0);
IRQ(DMA1_Stream1);        IRQ(DMA1_Stream2);       IRQ(DMA1_Stream3);
IRQ(DMA1_Stream4);        IRQ(DMA1_Stream5);       IRQ(DMA1_Stream6);
IRQ(ADC);                 IRQ(FDCAN1_IT0);         IRQ(FDCAN2_IT0);
IRQ(FDCAN1_IT1);          IRQ(FDCAN2_IT1);         IRQ(EXTI9_5);
IRQ(TIM1_BRK);            IRQ(TIM1_UP);            IRQ(TIM1_TRG_COM);
IRQ(TIM1_CC);             IRQ(TIM2);               IRQ(TIM3);
IRQ(TIM4);                IRQ(I2C1_EV);            IRQ(I2C1_ER);
IRQ(I2C2_EV);             IRQ(I2C2_ER);            IRQ(SPI1);
IRQ(SPI2);                IRQ(USART1);             IRQ(USART2);
IRQ(USART3);              IRQ(EXTI15_10);          IRQ(RTC_Alarm);
IRQ(TIM8_BRK_TIM12);      IRQ(TIM8_UP_TIM13);      IRQ(TIM8_TRG_COM_TIM14);
IRQ(TIM8_CC);             IRQ(DMA1_Stream7);       IRQ(FMC);
IRQ(SDMMC1);              IRQ(TIM5);               IRQ(SPI3);
IRQ(UART4);               IRQ(UART5);              IRQ(TIM6_DAC);
IRQ(TIM7);                IRQ(DMA2_Stream0);       IRQ(DMA2_Stream1);
IRQ(DMA2_Stream2);        IRQ(DMA2_Stream3);       IRQ(DMA2_Stream4);
IRQ(ETH);                 IRQ(ETH_WKUP);           IRQ(FDCAN_CAL);
IRQ(CM7_SEV_IT);          IRQ(CM4_SEV_IT);         IRQ(DMA2_Stream5);
IRQ(DMA2_Stream6);        IRQ(DMA2_Stream7);       IRQ(USART6);
IRQ(I2C3_EV);             IRQ(I2C3_ER);            IRQ(OTG_HS_EP1_OUT);
IRQ(OTG_HS_EP1_IN);       IRQ(OTG_HS_WKUP);        IRQ(OTG_HS);
IRQ(DCMI);                IRQ(CRYP);               IRQ(HASH_RNG);
IRQ(FPU);                 IRQ(UART7);              IRQ(UART8);
IRQ(SPI4);                IRQ(SPI5);               IRQ(SPI6);
IRQ(SAI1);                IRQ(LTDC);               IRQ(LTDC_ER);
IRQ(DMA2D);               IRQ(SAI2);               IRQ(QUADSPI);
IRQ(LPTIM1);              IRQ(CEC);                IRQ(I2C4_EV);
IRQ(I2C4_ER);             IRQ(SPDIF_RX);           IRQ(OTG_FS_EP1_OUT);
IRQ(OTG_FS_EP1_IN);       IRQ(OTG_FS_WKUP);        IRQ(OTG_FS);
IRQ(DMAMUX1_OVR);         IRQ(HRTIM1_Master);      IRQ(HRTIM1_TIMA);
IRQ(HRTIM1_TIMB);         IRQ(HRTIM1_TIMC);        IRQ(HRTIM1_TIMD);
IRQ(HRTIM1_TIME);         IRQ(HRTIM1_FLT);         IRQ(DFSDM1_FLT0);
IRQ(DFSDM1_FLT1);         IRQ(DFSDM1_FLT2);        IRQ(DFSDM1_FLT3);
IRQ(SAI3);                IRQ(SWPMI1);             IRQ(TIM15);
IRQ(TIM16);               IRQ(TIM17);              IRQ(MDIOS_WKUP);
IRQ(MDIOS);               IRQ(JPEG);               IRQ(MDMA);
IRQ(SDMMC2);              IRQ(HSEM1);              IRQ(ADC3);
IRQ(DMAMUX2_OVR);         IRQ(BDMA_Channel0);      IRQ(BDMA_Channel1);
IRQ(BDMA_Channel2);       IRQ(BDMA_Channel3);      IRQ(BDMA_Channel4);
IRQ(BDMA_Channel5);       IRQ(BDMA_Channel6);      IRQ(BDMA_Channel7);
IRQ(COMP);                IRQ(LPTIM2);             IRQ(LPTIM3);
IRQ(LPTIM4);              IRQ(LPTIM5);             IRQ(LPUART1);
IRQ(WWDG_RST);            IRQ(CRS);                IRQ(ECC);
IRQ(SAI4);                IRQ(WAKEUP_PIN);

#undef IRQ
#undef WEAK_DEFAULT

// --- Vector table ---------------------------------------------------
//
// 16 system entries + STM32H7 peripheral IRQ slots. RM0399 §11.1.4.
// `mmio: owned by RM0399 §11.1.4; never freed.`
//
// Slot 0 is the initial MSP (a stack-top *value*, not a callable);
// every other slot is a function pointer. The Cortex-M boot ROM
// loads word 0 into MSP and word 1 into PC, so the type-pun via a
// union is benign — the CPU only cares about the underlying word
// pattern at the FLASH address.

union vector_entry {
    void (*fn)();
    std::uint32_t word;
};

[[gnu::section(".isr_vector"), gnu::used]]
const vector_entry g_vector_table[] = {
    // 0..15: ARMv7-M system entries.
    { .word = reinterpret_cast<std::uint32_t>(&_estack) },
    { .fn = &Reset_Handler },
    { .fn = &NMI_Handler },
    { .fn = &HardFault_Handler },
    { .fn = &MemManage_Handler },
    { .fn = &BusFault_Handler },
    { .fn = &UsageFault_Handler },
    { .word = 0 }, { .word = 0 }, { .word = 0 }, { .word = 0 },
    { .fn = &SVC_Handler },
    { .fn = &DebugMon_Handler },
    { .word = 0 },
    { .fn = &PendSV_Handler },
    { .fn = &SysTick_Handler },

    // 16+: STM32H7 peripheral IRQs.
    { .fn = &WWDG_IRQHandler },
    { .fn = &PVD_AVD_IRQHandler },
    { .fn = &TAMP_STAMP_IRQHandler },
    { .fn = &RTC_WKUP_IRQHandler },
    { .fn = &FLASH_IRQHandler },
    { .fn = &RCC_IRQHandler },
    { .fn = &EXTI0_IRQHandler },
    { .fn = &EXTI1_IRQHandler },
    { .fn = &EXTI2_IRQHandler },
    { .fn = &EXTI3_IRQHandler },
    { .fn = &EXTI4_IRQHandler },
    { .fn = &DMA1_Stream0_IRQHandler },
    { .fn = &DMA1_Stream1_IRQHandler },
    { .fn = &DMA1_Stream2_IRQHandler },
    { .fn = &DMA1_Stream3_IRQHandler },
    { .fn = &DMA1_Stream4_IRQHandler },
    { .fn = &DMA1_Stream5_IRQHandler },
    { .fn = &DMA1_Stream6_IRQHandler },
    { .fn = &ADC_IRQHandler },
    { .fn = &FDCAN1_IT0_IRQHandler },
    { .fn = &FDCAN2_IT0_IRQHandler },
    { .fn = &FDCAN1_IT1_IRQHandler },
    { .fn = &FDCAN2_IT1_IRQHandler },
    { .fn = &EXTI9_5_IRQHandler },
    { .fn = &TIM1_BRK_IRQHandler },
    { .fn = &TIM1_UP_IRQHandler },
    { .fn = &TIM1_TRG_COM_IRQHandler },
    { .fn = &TIM1_CC_IRQHandler },
    { .fn = &TIM2_IRQHandler },
    { .fn = &TIM3_IRQHandler },
    { .fn = &TIM4_IRQHandler },
    { .fn = &I2C1_EV_IRQHandler },
    { .fn = &I2C1_ER_IRQHandler },
    { .fn = &I2C2_EV_IRQHandler },
    { .fn = &I2C2_ER_IRQHandler },
    { .fn = &SPI1_IRQHandler },
    { .fn = &SPI2_IRQHandler },
    { .fn = &USART1_IRQHandler },
    { .fn = &USART2_IRQHandler },
    { .fn = &USART3_IRQHandler },
    { .fn = &EXTI15_10_IRQHandler },
    { .fn = &RTC_Alarm_IRQHandler },
    { .word = 0 }, // gap (RM0399 §11.1.4 row 41 reserved on H7)
    { .fn = &TIM8_BRK_TIM12_IRQHandler },
    { .fn = &TIM8_UP_TIM13_IRQHandler },
    { .fn = &TIM8_TRG_COM_TIM14_IRQHandler },
    { .fn = &TIM8_CC_IRQHandler },
    { .fn = &DMA1_Stream7_IRQHandler },
    { .fn = &FMC_IRQHandler },
    { .fn = &SDMMC1_IRQHandler },
    { .fn = &TIM5_IRQHandler },
    { .fn = &SPI3_IRQHandler },
    { .fn = &UART4_IRQHandler },
    { .fn = &UART5_IRQHandler },
    { .fn = &TIM6_DAC_IRQHandler },
    { .fn = &TIM7_IRQHandler },
    { .fn = &DMA2_Stream0_IRQHandler },
    { .fn = &DMA2_Stream1_IRQHandler },
    { .fn = &DMA2_Stream2_IRQHandler },
    { .fn = &DMA2_Stream3_IRQHandler },
    { .fn = &DMA2_Stream4_IRQHandler },
    { .fn = &ETH_IRQHandler },
    { .fn = &ETH_WKUP_IRQHandler },
    { .fn = &FDCAN_CAL_IRQHandler },
    { .fn = &CM7_SEV_IT_IRQHandler },
    { .fn = &CM4_SEV_IT_IRQHandler },
    { .word = 0 }, { .word = 0 },
    { .fn = &DMA2_Stream5_IRQHandler },
    { .fn = &DMA2_Stream6_IRQHandler },
    { .fn = &DMA2_Stream7_IRQHandler },
    { .fn = &USART6_IRQHandler },
    { .fn = &I2C3_EV_IRQHandler },
    { .fn = &I2C3_ER_IRQHandler },
    { .fn = &OTG_HS_EP1_OUT_IRQHandler },
    { .fn = &OTG_HS_EP1_IN_IRQHandler },
    { .fn = &OTG_HS_WKUP_IRQHandler },
    { .fn = &OTG_HS_IRQHandler },
    { .fn = &DCMI_IRQHandler },
    { .fn = &CRYP_IRQHandler },
    { .fn = &HASH_RNG_IRQHandler },
    { .fn = &FPU_IRQHandler },
    { .fn = &UART7_IRQHandler },
    { .fn = &UART8_IRQHandler },
    { .fn = &SPI4_IRQHandler },
    { .fn = &SPI5_IRQHandler },
    { .fn = &SPI6_IRQHandler },
    { .fn = &SAI1_IRQHandler },
    { .fn = &LTDC_IRQHandler },
    { .fn = &LTDC_ER_IRQHandler },
    { .fn = &DMA2D_IRQHandler },
    { .fn = &SAI2_IRQHandler },
    { .fn = &QUADSPI_IRQHandler },
    { .fn = &LPTIM1_IRQHandler },
    { .fn = &CEC_IRQHandler },
    { .fn = &I2C4_EV_IRQHandler },
    { .fn = &I2C4_ER_IRQHandler },
    { .fn = &SPDIF_RX_IRQHandler },
    { .fn = &OTG_FS_EP1_OUT_IRQHandler },
    { .fn = &OTG_FS_EP1_IN_IRQHandler },
    { .fn = &OTG_FS_WKUP_IRQHandler },
    { .fn = &OTG_FS_IRQHandler },
    { .fn = &DMAMUX1_OVR_IRQHandler },
    { .fn = &HRTIM1_Master_IRQHandler },
    { .fn = &HRTIM1_TIMA_IRQHandler },
    { .fn = &HRTIM1_TIMB_IRQHandler },
    { .fn = &HRTIM1_TIMC_IRQHandler },
    { .fn = &HRTIM1_TIMD_IRQHandler },
    { .fn = &HRTIM1_TIME_IRQHandler },
    { .fn = &HRTIM1_FLT_IRQHandler },
    { .fn = &DFSDM1_FLT0_IRQHandler },
    { .fn = &DFSDM1_FLT1_IRQHandler },
    { .fn = &DFSDM1_FLT2_IRQHandler },
    { .fn = &DFSDM1_FLT3_IRQHandler },
    { .fn = &SAI3_IRQHandler },
    { .fn = &SWPMI1_IRQHandler },
    { .fn = &TIM15_IRQHandler },
    { .fn = &TIM16_IRQHandler },
    { .fn = &TIM17_IRQHandler },
    { .fn = &MDIOS_WKUP_IRQHandler },
    { .fn = &MDIOS_IRQHandler },
    { .fn = &JPEG_IRQHandler },
    { .fn = &MDMA_IRQHandler },
    { .word = 0 },
    { .fn = &SDMMC2_IRQHandler },
    { .fn = &HSEM1_IRQHandler },
    { .word = 0 },
    { .fn = &ADC3_IRQHandler },
    { .fn = &DMAMUX2_OVR_IRQHandler },
    { .fn = &BDMA_Channel0_IRQHandler },
    { .fn = &BDMA_Channel1_IRQHandler },
    { .fn = &BDMA_Channel2_IRQHandler },
    { .fn = &BDMA_Channel3_IRQHandler },
    { .fn = &BDMA_Channel4_IRQHandler },
    { .fn = &BDMA_Channel5_IRQHandler },
    { .fn = &BDMA_Channel6_IRQHandler },
    { .fn = &BDMA_Channel7_IRQHandler },
    { .fn = &COMP_IRQHandler },
    { .fn = &LPTIM2_IRQHandler },
    { .fn = &LPTIM3_IRQHandler },
    { .fn = &LPTIM4_IRQHandler },
    { .fn = &LPTIM5_IRQHandler },
    { .fn = &LPUART1_IRQHandler },
    { .fn = &WWDG_RST_IRQHandler },
    { .fn = &CRS_IRQHandler },
    { .fn = &ECC_IRQHandler },
    { .fn = &SAI4_IRQHandler },
    { .word = 0 }, { .word = 0 },
    { .fn = &WAKEUP_PIN_IRQHandler },
};

} // extern "C"
