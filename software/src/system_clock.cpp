
#include <Arduino.h>

extern "C" void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {};
    RCC_ClkInitTypeDef clk = {};
    RCC_PeriphCLKInitTypeDef periph = {};

    // Internal 16 MHz oscillator and main PLL
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;

    // 16 MHz / 2 * 12 = 96 MHz PLL VCO
    // PLL Q output = 96 MHz / 2 = 48 MHz
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM = 2;
    osc.PLL.PLLN = 12;
    osc.PLL.PLLQ = RCC_PLLQ_DIV2;
    osc.PLL.PLLR = RCC_PLLR_DIV2;

    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        while (1) {}
    }

    // Keep the CPU at 16 MHz
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_HCLK |
                    RCC_CLOCKTYPE_PCLK1 |
                    RCC_CLOCKTYPE_PCLK2;

    clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0) != HAL_OK) {
        while (1) {}
    }

    // Select 48 MHz PLL output for USB
    periph.PeriphClockSelection = RCC_PERIPHCLK_USB;
    periph.UsbClockSelection = RCC_USBCLKSOURCE_PLL;

    if (HAL_RCCEx_PeriphCLKConfig(&periph) != HAL_OK) {
        while (1) {}
    }
}
