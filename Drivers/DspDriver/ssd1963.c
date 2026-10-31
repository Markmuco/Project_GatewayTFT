/*
 * ssd1963.c
 *
 *  Created on: Oct 13, 2023
 *      Author: Mark
 *      Original: SSD1963_i8080.c
 */
#include "main.h"
#include "ssd1963.h"
#include "uart_sci.h"
#include "fmc.h"

// 1 = color test after the init: blue (slow bus), red (fast bus), then black
#define SSD1963_COLOR_TEST	(0)

// Funmction prototypes
static void SSD1963_Send_Cmd(uint16_t cmd);
static void SSD1963_Send_Data(uint16_t data);
static uint16_t SSD1963_Reed_Data();
static uint16_t H24_RGB565(uint8_t reverse, uint32_t color24);




/*
 *
 */
static uint16_t H24_RGB565(uint8_t reverse, uint32_t color24)
{
	uint8_t b = (color24 >> 16) & 0xFF;
	uint8_t g = (color24 >> 8) & 0xFF;
	uint8_t r = color24 & 0xFF;


	if (reverse)
		return ((b / 8) << 11) | ((g / 4) << 5) | (r / 8);
	else
		return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}

/*
 *
 */
void LCD_Window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	SSD1963_Send_Cmd(SSD1963_CMD_SET_COLUMN_ADDRESS);
	SSD1963_Send_Data(y1 >> 8);
	SSD1963_Send_Data(y1 & 0x00FF);
	SSD1963_Send_Data(y2 >> 8);
	SSD1963_Send_Data(y2 & 0x00FF);
	SSD1963_Send_Cmd(SSD1963_CMD_SET_PAGE_ADDRESS);
	SSD1963_Send_Data(x1 >> 8);
	SSD1963_Send_Data(x1 & 0x00FF);
	SSD1963_Send_Data(x2 >> 8);
	SSD1963_Send_Data(x2 & 0x00FF);
	SSD1963_Send_Cmd(SSD1963_CMD_WRITE_MEMORY_START);
}

void LCD_Rect_Fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color24)
{
	uint32_t i = 0;
	LCD_Window(y, x, y + h - 1, x + w - 1);
	for (i = 0; i < w * h; i++)
		SSD1963_Send_Data(H24_RGB565(1, color24));
}

/**
 * @brief Set the multiplier and devider for block of PLL.
 *
 * @param multiplier Default after reset value of multiplier = 0b00101101
 * @param devider Default after reset value of devider = 00011
 * @param effectuate Effectuate multiplier and devider value.
 *                  Default after reset value of effectuate = 0
 *                  0 Ignore the multiplier (N) and divider (N) values
 *                  1 Effectuate the multiplier and divider value
 */
void SSD1963_set_PLL_multiplier(uint8_t multiplier, uint8_t devider, uint8_t effectuate)
{
	SSD1963_Send_Cmd(SSD1963_CMD_SET_PLL_MN);
	SSD1963_Send_Data(multiplier);
	SSD1963_Send_Data(devider);
	SSD1963_Send_Data(effectuate);
}

/**
 * @brief Start the PLL. Before the start, the system was operated with the crystal oscillator or clock input.
 *      After PLL enabled for 100us, can start to lock PLL
 * @param PLL_lock
 *                  0 Use reference clock as system clock
 *                  1 Use PLL output as system clock
 * @param PLL_enable
 *                  0 Disable PLL
 *                  1 Enable PLL
 */
void SSD1963_set_PLL(bool PLL_lock, bool PLL_enable)
{
	SSD1963_Send_Cmd(SSD1963_CMD_SET_PLL);
	SSD1963_Send_Data((uint8_t) ((PLL_lock << 1) | PLL_enable));
}

/**
 * @brief Set the LSHIFT (pixel clock) frequency.
 *      Configure the pixel clock to PLL freq x ((LCDC_FPR + 1) / 2^20)
 *      LCDC_FPR = (pixel clock * 2^20 / PLL freq) - 1
 * @param pixel_freq clock value in MHz
 */
void SSD1963_set_pixel_freq(uint8_t pixel_freq)
{
	/*
	 uint64_t LCDC_FPR = ((pixel_freq * 1000000) * 0x100000 / ((10 * 1000000 * (SSD1963_PLL_MULTIPLIER + 1))/(SSD1963_PLL_DEVIDER + 1))) - 1;
	 SSD1963_Send_Cmd(SSD1963_CMD_SET_LSHIFT_FREQ);
	 SSD1963_Send_Data(LCDC_FPR >> 16);
	 SSD1963_Send_Data(LCDC_FPR >> 8);
	 SSD1963_Send_Data(LCDC_FPR);
	 */
	SSD1963_Send_Cmd(SSD1963_CMD_SET_LSHIFT_FREQ);
	SSD1963_Send_Data(0x07);
	SSD1963_Send_Data(0xFF);
	SSD1963_Send_Data(0xFF);
}

/**
 * @brief TE signal is sent from the SSD1963 to the host processor at the start of VFP.
 *
 * @param  Tearing effect line mode. Default after reset value 0.
 *      0 The tearing effect output line consists of V-blanking information only.
 *      1 The tearing effect output line consists of both V-blanking and H-blanking information by
 *          set_tear_scanline (0x44).
 */
void SSD1963_Tearing(bool tearing_mode)
{
	SSD1963_Send_Cmd(SSD1963_CMD_SET_TEAR_ON);
	if (tearing_mode)
	{
		SSD1963_Send_Data(0x01);
	}
	else
	{
		SSD1963_Send_Data(0x00);
	}
}

/**
 * @brief Backlight control. Set the PWM configuration.
 *      PWMF[7:0] : Set the PWM frequency in system clock. Default after reset value 0.
 *      PWM[7:0] : Set the PWM duty cycle. Default after reset value 0.
 *      NOTE: If DBC enable (0xD0] A0 = 1, these parameter will be ignored
 *      NOTE: PWM always 0 if PWM[7:0] = 00h
 *      C[3] : PWM configuration. Default after reset value 0.
 *          0 PWM controlled by host
 *          1 PWM controlled by DBC
 *      C[0] : PWM enable. Default after reset value 0.
 *          0 PWM disable
 *          1 PWM enable
 *      D[7:0] : DBC manual brightness. Default after reset value 0.
 *          00 Dimmest
 *          FF brightest
 *      E[7:0] : DBC minimum brightness. Default after reset value 0.
 *          00 Dimmest
 *          FF brightest
 *      F[3:0] : Brightness prescaler. Default after reset value 0. This parameter is valid when
 *          Transition Effect enable (0xD0) A5 = 1
 * @param level 0 to 100%
 */
void SSD1963_backlight(uint8_t level)
{
	if (level > 100)
		level = 100;
	level = level * 255 / 100;
	SSD1963_Send_Cmd(SSD1963_CMD_SET_PWM_CONF);
	SSD1963_Send_Data(0x0006);			// PWM frequency
	SSD1963_Send_Data(level);			// PWM duty cycle
	SSD1963_Send_Data(0x0001);			// C[3] = 0: PWM controlled by host, C[0] = 1: PWM enabled
	SSD1963_Send_Data(0x00f0);			// DBC manual brightness (not used, DBC off)
	SSD1963_Send_Data(0x0000);			// DBC minimum brightness
	SSD1963_Send_Data(0x0000);			// brightness prescaler
}

/**
 * @brief Panel on / off. Off: the panel is not driven (no static image on the liquid crystal),
 *        the frame memory is kept and can still be written, on shows the actual contents.
 * @param on 1 = on, 0 = off
 */
void SSD1963_display(uint8_t on)
{
	SSD1963_Send_Cmd(on ? SSD1963_CMD_SET_DISPLAY_ON : SSD1963_CMD_SET_DISPLAY_OFF);
}

/**
 * @brief Initialization of SSD1963 over intel 8080 parallel interface
 *
 */
void SSD1963_Init(void)
{
	volatile uint16_t id[6] =
	{ 0 };
	FMC_NORSRAM_TimingTypeDef slow =
	{ 0 };

	// Until the PLL is locked the SSD1963 runs on the 10 MHz crystal: slow bus.
	// Save the (fast) CubeMX timing, restore it at the end of the init.
	uint32_t btr_fast = FMC_Bank1_R->BTCR[1];

	slow.AddressSetupTime = 15;
	slow.AddressHoldTime = 15;
	slow.DataSetupTime = 80;		// 160 MHz: ~600 ns per access
	slow.DataHoldTime = 0;
	slow.BusTurnAroundDuration = 5;
	slow.CLKDivision = 16;
	slow.DataLatency = 17;
	slow.AccessMode = FMC_ACCESS_MODE_A;
	FMC_NORSRAM_Timing_Init(hsram1.Instance, &slow, FMC_NORSRAM_BANK1);

	// Hardware reset (D_RST, PB10)
#ifdef LCD_RESET_Pin
	HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);
	HAL_Delay(10);
	HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET);
	HAL_Delay(20);
#endif

	SSD1963_Send_Cmd(SSD1963_CMD_SOFT_RESET);

	HAL_Delay(50);

	// Device Description Block: 01 57 61 01 FF, read as data (RS = A18 high)
	SSD1963_Send_Cmd(SSD1963_CMD_READ_DDB);

	for (uint8_t z = 0; z < 5; z++)
	{
		id[z] = LCD_DataRead() & 0xFF;
	}

	if (id[0] == SSD1963_SUPPLIER_ID_H && id[1] == SSD1963_SUPPLIER_ID_L && id[2] == SSD1963_PRODUCT_ID)
		tty_printf("Dsp ok\r\n");
	else
		tty_printf(" ERR dsp %02X %02X %02X %02X %02X\r\n", id[0], id[1], id[2], id[3], id[4]);

	SSD1963_set_PLL_multiplier(SSD1963_PLL_MULTIPLIER, SSD1963_PLL_DEVIDER, SSD1963_PLL_EFFECTUATE);

	SSD1963_set_PLL(false, true);

	HAL_Delay(100);

	SSD1963_set_PLL(true, true);

	HAL_Delay(50);

	SSD1963_Send_Cmd(SSD1963_CMD_SOFT_RESET);

	HAL_Delay(50);

	SSD1963_set_pixel_freq(SSD1963_PIXEL_FREQ);

	SSD1963_Send_Cmd(SSD1963_CMD_SET_LCD_MODE);
	SSD1963_Send_Data(0x0000); // 16/18Bit mode (0x0020);
	SSD1963_Send_Data(0x0000);
	SSD1963_Send_Data(mHIGH((LCD_WIDTH-1)));
	SSD1963_Send_Data(mLOW((LCD_WIDTH-1)));
	SSD1963_Send_Data(mHIGH((LCD_HEIGHT-1)));
	SSD1963_Send_Data(mLOW((LCD_HEIGHT-1)));
	SSD1963_Send_Data(0x0000);

	SSD1963_Send_Cmd(SSD1963_CMD_SET_HORI_PERIOD);
	SSD1963_Send_Data(0x0004);
	SSD1963_Send_Data(0x001f);
	SSD1963_Send_Data(0x0000);
	SSD1963_Send_Data(0x00d2);
	SSD1963_Send_Data(0x0000);
	SSD1963_Send_Data(0x0000);
	SSD1963_Send_Data(0x0000);
	SSD1963_Send_Data(0x0000);

	SSD1963_Send_Cmd(SSD1963_CMD_SET_VERT_PERIOD);
	SSD1963_Send_Data(0x0002);
	SSD1963_Send_Data(0x000c);
	SSD1963_Send_Data(0x0000);
	SSD1963_Send_Data(0x0022);
	SSD1963_Send_Data(0x0000);
	SSD1963_Send_Data(0x0000);

	SSD1963_Tearing(SSD1963_TEARMODE_VBLANKING);

	SSD1963_Send_Cmd(SSD1963_CMD_SET_GPIO_CONF);
	SSD1963_Send_Data(0x000f);
	SSD1963_Send_Data(0x0001);

	SSD1963_Send_Cmd(SSD1963_CMD_SET_GPIO_VALUE);
	SSD1963_Send_Data(0x0001);

	SSD1963_Send_Cmd(SSD1963_CMD_SET_ADDRESS_MODE);
	SSD1963_Send_Data(0x0000); // BGR mode

	SSD1963_Send_Cmd(SSD1963_CMD_RESERVED3);
	SSD1963_Send_Data(0x0050);

	SSD1963_Send_Cmd(SSD1963_CMD_SET_PIXEL_DATA_INTERFACE);
	SSD1963_Send_Data(SSD1963_PDI_16BIT565);

	SSD1963_Send_Cmd(SSD1963_CMD_SET_POST_PROC);
	SSD1963_Send_Data(0x0040);
	SSD1963_Send_Data(0x0080);
	SSD1963_Send_Data(0x0040);
	SSD1963_Send_Data(0x0001);

	HAL_Delay(10);

	SSD1963_Send_Cmd(SSD1963_CMD_SET_DISPLAY_ON);

	SSD1963_backlight(20);

	// DBC (dynamic backlight control) off, otherwise it overrules SSD1963_backlight()
	SSD1963_Send_Cmd(SSD1963_CMD_SET_DBC_CONF);
	SSD1963_Send_Data(0x0000);

	SSD1963_Clear_Screen(BLACK);

	HAL_Delay(10);

	SSD1963_Send_Cmd(SSD1963_CMD_SET_DISPLAY_ON);

#if SSD1963_COLOR_TEST
	// write only test, independent of reading the SSD1963
	tty_printf("Dsp test: blue = slow bus\r\n");
	SSD1963_Clear_Screen(BLUE);
	HAL_Delay(1000);
#endif

	// PLL is running: back to the fast CubeMX bus timing
	FMC_Bank1_R->BTCR[1] = btr_fast;

#if SSD1963_COLOR_TEST
	tty_printf("Dsp test: red = fast bus\r\n");
	SSD1963_Clear_Screen(RED);
	HAL_Delay(1000);
	SSD1963_Clear_Screen(BLACK);
#endif
}

void SSD1963_Set_X(uint16_t start_x, uint16_t end_x)
{
	SSD1963_Send_Cmd(SSD1963_CMD_SET_COLUMN_ADDRESS);
	SSD1963_Send_Data(start_x >> 8);
	SSD1963_Send_Data(start_x & 0x00ff);

	SSD1963_Send_Data(end_x >> 8);
	SSD1963_Send_Data(end_x & 0x00ff);
}

void SSD1963_Set_Y(uint16_t start_y, uint16_t end_y)
{
	SSD1963_Send_Cmd(SSD1963_CMD_SET_PAGE_ADDRESS);
	SSD1963_Send_Data(start_y >> 8);
	SSD1963_Send_Data(start_y & 0x00ff);

	SSD1963_Send_Data(end_y >> 8);
	SSD1963_Send_Data(end_y & 0x00ff);
}

//=============================================================================
// Selects screen area
//=============================================================================
void SSD1963_SetArea(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey)
{
	SSD1963_Send_Cmd(SSD1963_CMD_SET_COLUMN_ADDRESS);
	SSD1963_Send_Data((sx >> 8) & 0xFF);
	SSD1963_Send_Data((sx >> 0) & 0xFF);
	SSD1963_Send_Data((ex >> 8) & 0xFF);
	SSD1963_Send_Data((ex >> 0) & 0xFF);

	SSD1963_Send_Cmd(SSD1963_CMD_SET_PAGE_ADDRESS);
	SSD1963_Send_Data((sy >> 8) & 0xFF);
	SSD1963_Send_Data((sy >> 0) & 0xFF);
	SSD1963_Send_Data((ey >> 8) & 0xFF);
	SSD1963_Send_Data((ey >> 0) & 0xFF);

	SSD1963_Send_Cmd(SSD1963_CMD_WRITE_MEMORY_START);
}

void SSD1963_Set_Work_Area(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
	SSD1963_Set_X(x, x + width - 1);
	SSD1963_Set_Y(y, y + height - 1);
	SSD1963_Send_Cmd(SSD1963_CMD_WRITE_MEMORY_START);
}

/**
 * @brief Writes several data on SSD1963 frame bufer
 *
 * @param pData pointer to data of iternal framebufer
 * @param width width block of frame
 */
void SSD1963_FillLineFrameBuffer(uint16_t *pData, uint32_t width)
{
	for (uint32_t w = 0; w < width; w++)
	{
		SSD1963_Send_Data(*pData);
	}
}

/**
 * @brief Writes several data on SSD1963 frame bufer
 *
 * @param pData pointer to data of iternal framebufer
 * @param width width block of frame
 * @param height height block of frame
 */
void SSD1963_FillBlockFrameBuffer(uint16_t *pData, uint32_t width, uint32_t height)
{
	for (uint32_t h = 0; h < height; h++)
	{
		SSD1963_FillLineFrameBuffer(&pData[width * h], width);
	}
}

void SSD1963_Clear_Screen(uint16_t color)
{
	uint32_t i = 0;
	SSD1963_Set_Work_Area(0, 0, LCD_WIDTH, LCD_HEIGHT);
	for (i = 0; i < LCD_HEIGHT * LCD_WIDTH; i++)
	{
		SSD1963_Send_Data(color);
	}
}

/*
 *
 */
static void SSD1963_Send_Cmd(uint16_t cmd)
{
	LCD_CmdWrite(cmd);
}

/*
 *
 */
static void SSD1963_Send_Data(uint16_t data)
{
	LCD_DataWrite(data);
}

/*
 *
 */
static uint16_t SSD1963_Reed_Data()
{
	return LCD_DataRead();
}
