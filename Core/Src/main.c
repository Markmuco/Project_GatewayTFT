/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "fdcan.h"
#include "app_filex.h"
#include "i2c.h"
#include "icache.h"
#include "iwdg.h"
#include "octospi.h"
#include "rtc.h"
#include "spi.h"
#include "usart.h"
#include "app_usbx_device.h"
#include "usb_otg.h"
#include "gpio.h"
#include "fmc.h"
#include "app_touchgfx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/*
 * SSD1963 on the FMC: the block transfer and VSync are in Core/Src/TouchGFX_DataTransfer.c
 */
#include "ssd1963.h"
#include "TouchGFX_DataTransfer.h"
#include "home_can.h"
#include "can_functions.h"
#include "mx25l.h"
#include "uart_sci.h"
#include "shell.h"
#include "ms_timers.h"
#include "BH1750.h"
#include "m24xx.h"
#include "LD2420.h"
#include "dimmer.h"
#include "logging.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define VSYNC_PERIOD_MS	20		// TouchGFX frame rate 50 Hz, no TE pin connected

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
gvars_t gvars;


const app_info_t __attribute__((section (".mySection"))) c_app_info = { .key =
		APP_KEY,				// Key
		.version = VERSION,		// Version
		.crc32 = 0xFFFFFFFF,		// CCITT-CRC32
		.size = 0xFFFFFFFF,		// File size
		.can_serial = CAN_SERIAL,
		.build_date = __DATE__,	// Build date
		.build_time = __TIME__,	// Build time
		.dummy = { 0xFF, 0xFF, 0xFF } // Dummy[3]
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

	extern uint32_t _main_app_start_address;
	uint32_t AppAddr = (uint32_t) &_main_app_start_address;
	uint32_t *ram_key = (uint32_t*) RAM_KEY;

	uint32_t *p_serial = (uint32_t*) RAM_SERIAL; // application fills the serial
	// My serial on fixed RAM address
	*p_serial = CAN_SERIAL;

	// Vector relocate
	SCB->VTOR = AppAddr;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

	__enable_irq();

	// Backup SRAM (2 kB, kept on VBAT): graph history (Model.cpp), before the TouchGFX Model is made
	__HAL_RCC_PWR_CLK_ENABLE();
	HAL_PWR_EnableBkUpAccess();
	__HAL_RCC_BKPSRAM_CLK_ENABLE();
	HAL_PWREx_EnableBkupRAMRetention();	// backup regulator on: retention on VBAT

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_FDCAN1_Init();
  MX_FMC_Init();
  MX_OCTOSPI1_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_FileX_Init();
  MX_USBX_Device_Init();
  MX_ICACHE_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_IWDG_Init();
  MX_TouchGFX_Init();
  /* USER CODE BEGIN 2 */
	uint32_t vsync_tick = HAL_GetTick();

	init_sci();

	timer_open();

	SHELL_OPEN_HEX(CAN_SERIAL);

	// reset cause (flags survive the bootloader if it does not clear them)
	tty_printf("Reset:%s%s%s%s%s\r\n",
			__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) ? " IWDG" : "",
			__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) ? " WWDG" : "",
			__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) ? " SOFT" : "",
			__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST) ? " BOR" : "",
			__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) ? " PIN" : "");
	__HAL_RCC_CLEAR_RESET_FLAGS();

	// EEPROM driver first: init_vars() reads the settings via I2C
	if (eeprom_init(&hi2c1) == HAL_OK)
		tty_printf("EEPROM ok\r\n");
	else
		tty_printf("Error EEPROM\r\n");

	// get EEROM
	init_vars();

	home_can_start();

	SSD1963_Init();


	// MX25L3233F, must be ready before the USB mass storage starts
	gvars.mx25l_ok = mx25l_init();
	tty_printf("Flash MX25L3233F %s\r\n", gvars.mx25l_ok ? "OK" : "FAIL");

	// USB mass storage: start (D+ pull up on) after the flash is ready
	HAL_PCD_Start(&hpcd_USB_OTG_FS);

	// Init
	if (BH1750_init(&hi2c1) == HAL_OK)
		tty_printf("BH1750 ok\r\n");
	else
		tty_printf("Error BH1750\r\n");

	*ram_key = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
    /* USER CODE END WHILE */

  MX_TouchGFX_Process();
    /* USER CODE BEGIN 3 */

  	    wdt_clr();

		ux_device_stack_tasks_run();	// USBX standalone

		// VSync for TouchGFX: difference with the last one, so no VSync is lost or doubled
		if ((HAL_GetTick() - vsync_tick) >= VSYNC_PERIOD_MS)
		{
			vsync_tick += VSYNC_PERIOD_MS;
			touchgfxSignalVSync();
		}

		handle_can();

		// read serial port and parse data
		process_ld2420_data();

		shell_process();

		// TFT backlight
		process_dimmer();


		// every hour a line in tft_<year>.csv
		process_logging();
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_CRSInitTypeDef RCC_CRSInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.LSIDiv = RCC_LSI_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 20;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 1;
  RCC_OscInitStruct.PLL.PLLR = 1;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the SYSCFG APB clock
  */
  __HAL_RCC_CRS_CLK_ENABLE();

  /** Configures CRS
  */
  RCC_CRSInitStruct.Prescaler = RCC_CRS_SYNC_DIV1;
  RCC_CRSInitStruct.Source = RCC_CRS_SYNC_SOURCE_USB;
  RCC_CRSInitStruct.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
  RCC_CRSInitStruct.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000,1000);
  RCC_CRSInitStruct.ErrorLimitValue = 34;
  RCC_CRSInitStruct.HSI48CalibrationValue = 63;

  HAL_RCCEx_CRSConfig(&RCC_CRSInitStruct);
}

/* USER CODE BEGIN 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  file: The file name as string.
 * @param  line: The line in file as a number.
 * @retval None
 */
void _Error_Handler(char *file, int line)
{
	tty_printf("FATAL error File [%s] line %d\r\n", file, line);
	while (1)
	{
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
