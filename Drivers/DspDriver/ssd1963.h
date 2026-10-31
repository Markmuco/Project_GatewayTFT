/*
 * ssd1963.h
 *
 *  Created on: Oct 13, 2023
 *      Author: Mark
 */
#ifndef SRC_DSPDRIVER_SSD1963_H_
#define SRC_DSPDRIVER_SSD1963_H_

#define LCD_BASE0        		((uint32_t)0x60000000)
#define LCD_BASE1        		((uint32_t)0x60080000)

#define LCD_BL_ON				HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_RESET)
#define LCD_BL_OFF				HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET)

#define LCD_CmdWrite(command)	*(volatile uint16_t *) (LCD_BASE0) = (command)
#define LCD_DataWrite(data)		*(volatile uint16_t *) (LCD_BASE1) = (data)
#define	LCD_StatusRead()		*(volatile uint16_t *) (LCD_BASE0) //if use read  Mcu interface DB0~DB15 needs increase pull high
#define	LCD_DataRead()			*(volatile uint16_t *) (LCD_BASE1) //if use read  Mcu interface DB0~DB15 needs increase pull high

#define mHIGH(x) 			(x >> 8)
#define mLOW(x) 			(x & 0xFF)

#define LCD_MODULE_XTAL                             10
#define LCD_WIDTH                                   800
#define LCD_HEIGHT                                  480
#define LSHIFT                                      0
#define MIRROR_V                                    0
#define MIRROR_H                                    0

#define	RED                                         0XF800
#define	GREEN                                       0X07E0
#define	BLUE                                        0X001F
#define BLACK                                       0X0000
#define WHITE                                       0XFFFF

#define SSD1963_PIXEL_FREQ                          16

#define SSD1963_TEARMODE_VBLANKING                  0
#define SSD1963_TEARMODE_VHBLANKING                 1

/**
 * @brief Device Description Block (DDB)
 * SSL[15:8] : Supplier ID of Solomon Systech Limited high byte, always 01h (POR = 00000001)
 * SSL[7:0] : Supplier ID of Solomon Systech Limited low byte, always 57h (POR = 01010111)
 * PROD[7:0] : Product ID, always 61h (POR = 01100001)
 * REV[2:0] : Revision code, always 01h (POR = 001)
 * Exit code, always FFh (POR = 11111111)
 */
#define SSD1963_SUPPLIER_ID_H                       0x01
#define SSD1963_SUPPLIER_ID_L                       0x57
#define SSD1963_PRODUCT_ID                          0x61
#define SSD1963_REVISION_CODE                       0xFF

/**
 * @brief Set the MN of PLL.
 * VCO = Reference input clock x (Multiplier + 1)
 * PLL frequency = VCO / (Divider + 1)
 */
#define SSD1963_PLL_MULTIPLIER                      0x23
#define SSD1963_PLL_DEVIDER                         0x02
#define SSD1963_PLL_EFFECTUATE                      0x54

#define SSD1963_CMD_NOP                             0x00
#define SSD1963_CMD_SOFT_RESET                      0x01
#define SSD1963_CMD_GET_POWER_MODE                  0x0A
#define SSD1963_CMD_GET_ADDRESS_MODE                0x0B
#define SSD1963_CMD_RESERVED1                       0x0C
#define SSD1963_CMD_GET_DISPLAY_MODE                0x0D
#define SSD1963_CMD_GET_TEAR_EFFECT_STATUS          0x0E
#define SSD1963_CMD_RESERVED2                       0x0F
#define SSD1963_CMD_ENTER_SLEEP_MODE                0x10
#define SSD1963_CMD_EXIT_SLEEP_MODE                 0x11
#define SSD1963_CMD_ENTER_PARTIAL_MODE              0x12
#define SSD1963_CMD_ENTER_NORMAL_MODE               0x13
#define SSD1963_CMD_EXIT_INVERT_MODE                0x20
#define SSD1963_CMD_ENTER_INVERT_MODE               0x21
#define SSD1963_CMD_SET_GAMMA_CURVE                 0x26
#define SSD1963_CMD_SET_DISPLAY_OFF                 0x28
#define SSD1963_CMD_SET_DISPLAY_ON                  0x29
#define SSD1963_CMD_SET_COLUMN_ADDRESS              0x2A
#define SSD1963_CMD_SET_PAGE_ADDRESS                0x2B
#define SSD1963_CMD_WRITE_MEMORY_START              0x2C
#define SSD1963_CMD_READ_MEMORY_START               0x2E
#define SSD1963_CMD_SET_PARTIAL_AREA                0x30
#define SSD1963_CMD_SET_SCROLL_AREA                 0x33
#define SSD1963_CMD_SET_TEAR_OFF                    0x34
#define SSD1963_CMD_SET_TEAR_ON                     0x35
#define SSD1963_CMD_SET_ADDRESS_MODE                0x36
#define SSD1963_CMD_SET_SCROLL_START                0x37
#define SSD1963_CMD_EXIT_IDLE_MODE                  0x38
#define SSD1963_CMD_ENTER_IDLE_MODE                 0x39
#define SSD1963_CMD_RESERVED3                       0x3A
#define SSD1963_CMD_WRITE_MEMORY_CONTINUE           0x3C
#define SSD1963_CMD_READ_MEMORY_CONTINUE            0x3E
#define SSD1963_CMD_SET_TEAR_SCANLINE               0x44
#define SSD1963_CMD_GET_SCANLINE                    0x45
#define SSD1963_CMD_READ_DDB                        0xA1
#define SSD1963_CMD_RESERVED4                       0xA8
#define SSD1963_CMD_SET_LCD_MODE                    0xB0
#define SSD1963_CMD_GET_LCD_MODE                    0xB1
#define SSD1963_CMD_SET_HORI_PERIOD                 0xB4
#define SSD1963_CMD_GET_HORI_PERIOD                 0xB5
#define SSD1963_CMD_SET_VERT_PERIOD                 0xB6
#define SSD1963_CMD_GET_VERT_PERIOD                 0xB7
#define SSD1963_CMD_SET_GPIO_CONF                   0xB8
#define SSD1963_CMD_GET_GPIO_CONF                   0xB9
#define SSD1963_CMD_SET_GPIO_VALUE                  0xBA
#define SSD1963_CMD_GET_GPIO_STATUS                 0xBB
#define SSD1963_CMD_SET_POST_PROC                   0xBC
#define SSD1963_CMD_GET_POST_PROC                   0xBD
#define SSD1963_CMD_SET_PWM_CONF                    0xBE
#define SSD1963_CMD_GET_PWM_CONF                    0xBF
#define SSD1963_CMD_SET_LCD_GEN0                    0xC0
#define SSD1963_CMD_GET_LCD_GEN0                    0xC1
#define SSD1963_CMD_SET_LCD_GEN1                    0xC2
#define SSD1963_CMD_GET_LCD_GEN1                    0xC3
#define SSD1963_CMD_SET_LCD_GEN2                    0xC4
#define SSD1963_CMD_GET_LCD_GEN2                    0xC5
#define SSD1963_CMD_SET_LCD_GEN3                    0xC6
#define SSD1963_CMD_GET_LCD_GEN3                    0xC7
#define SSD1963_CMD_SET_GPIO0_ROP                   0xC8
#define SSD1963_CMD_GET_GPIO0_ROP                   0xC9
#define SSD1963_CMD_SET_GPIO1_ROP                   0xCA
#define SSD1963_CMD_GET_GPIO1_ROP                   0xCB
#define SSD1963_CMD_SET_GPIO2_ROP                   0xCC
#define SSD1963_CMD_GET_GPIO2_ROP                   0xCD
#define SSD1963_CMD_SET_GPIO3_ROP                   0xCE
#define SSD1963_CMD_GET_GPIO3_ROP                   0xCF
#define SSD1963_CMD_SET_DBC_CONF                    0xD0
#define SSD1963_CMD_GET_DBC_CONF                    0xD1
#define SSD1963_CMD_SET_DBC_TH                      0xD4
#define SSD1963_CMD_GET_DBC_TH                      0xD5
#define SSD1963_CMD_SET_PLL                         0xE0
#define SSD1963_CMD_SET_PLL_MN                      0xE2
#define SSD1963_CMD_GET_PLL_MN                      0xE3
#define SSD1963_CMD_GET_PLL_STATUS                  0xE4
#define SSD1963_CMD_SET_DEEP_SLEEP                  0xE5
#define SSD1963_CMD_SET_LSHIFT_FREQ                 0xE6
#define SSD1963_CMD_GET_LSHIFT_FREQ                 0xE7
#define SSD1963_CMD_RESERVED5                       0xE8
#define SSD1963_CMD_RESERVED6                       0xE9
#define SSD1963_CMD_SET_PIXEL_DATA_INTERFACE        0xF0
#define SSD1963_CMD_GET_PIXEL_DATA_INTERFACE        0xF1
#define SSD1963_CMD_RESERVED7                       0xFF

#define SSD1963_PDI_8BIT			0
#define SSD1963_PDI_12BIT			1
#define SSD1963_PDI_16BIT			2
#define SSD1963_PDI_16BIT565		3
#define SSD1963_PDI_18BIT			4
#define SSD1963_PDI_24BIT			5
#define SSD1963_PDI_9BIT			6

/*
 * Available functions
 */
#ifdef __cplusplus
extern "C" {
#endif

void SSD1963_Init(void);
void SSD1963_Set_Work_Area(uint16_t x, uint16_t y, uint16_t length, uint16_t width);
void SSD1963_Clear_Screen(uint16_t color);
void SSD1963_backlight(uint8_t level);
void SSD1963_display(uint8_t on);		// panel on / off, the frame memory is kept

// not used for TouchGFX
void SSD1963_FillBlockFrameBuffer(uint16_t* pData, uint32_t width, uint32_t height);
void SSD1963_FillLineFrameBuffer(uint16_t* pData, uint32_t width);

#ifdef __cplusplus
}
#endif



#endif /* SRC_DSPDRIVER_SSD1963_H_ */
