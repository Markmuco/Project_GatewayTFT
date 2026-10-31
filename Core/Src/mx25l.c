/*
 * mx25l.c
 *
 * Driver for the Macronix MX25L3233F on OCTOSPI1, single line SPI, 24 bit address.
 */

#include "main.h"
#include "octospi.h"
#include "mx25l.h"

#define CMD_WREN		0x06
#define CMD_RDSR		0x05
#define CMD_READ_FAST	0x0B	// 8 dummy cycles
#define CMD_PP			0x02
#define CMD_SE			0x20	// 4 kB sector erase
#define CMD_RDID		0x9F
#define CMD_RSTEN		0x66
#define CMD_RST			0x99

#define SR_WIP			0x01
#define SR_WEL			0x02

#define TIMEOUT_MS		100U
#define TIMEOUT_PP_MS	50U
#define TIMEOUT_SE_MS	500U

static void cmd_setup(OSPI_RegularCmdTypeDef *c, uint8_t instr)
{
	c->OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
	c->FlashId = HAL_OSPI_FLASH_ID_1;
	c->Instruction = instr;
	c->InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
	c->InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
	c->InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	c->Address = 0;
	c->AddressMode = HAL_OSPI_ADDRESS_NONE;
	c->AddressSize = HAL_OSPI_ADDRESS_24_BITS;
	c->AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
	c->AlternateBytes = 0;
	c->AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	c->AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
	c->AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
	c->DataMode = HAL_OSPI_DATA_NONE;
	c->NbData = 0;
	c->DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
	c->DummyCycles = 0;
	c->DQSMode = HAL_OSPI_DQS_DISABLE;
	c->SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;
}

static bool cmd_simple(uint8_t instr)
{
	OSPI_RegularCmdTypeDef c;

	cmd_setup(&c, instr);
	return HAL_OSPI_Command(&hospi1, &c, TIMEOUT_MS) == HAL_OK;
}

static bool read_status(uint8_t *sr)
{
	OSPI_RegularCmdTypeDef c;

	cmd_setup(&c, CMD_RDSR);
	c.DataMode = HAL_OSPI_DATA_1_LINE;
	c.NbData = 1;

	if (HAL_OSPI_Command(&hospi1, &c, TIMEOUT_MS) != HAL_OK)
		return false;
	return HAL_OSPI_Receive(&hospi1, sr, TIMEOUT_MS) == HAL_OK;
}

static bool wait_ready(uint32_t timeout_ms)
{
	uint32_t start = HAL_GetTick();
	uint8_t sr;

	do
	{
		if (!read_status(&sr))
			return false;
		if ((sr & SR_WIP) == 0)
			return true;
	} while ((HAL_GetTick() - start) <= timeout_ms);

	return false;
}

static bool write_enable(void)
{
	uint8_t sr;

	if (!cmd_simple(CMD_WREN))
		return false;
	return read_status(&sr) && (sr & SR_WEL);
}

/*
 * Reset the chip and check the JEDEC id (C2 20 16), call after MX_OCTOSPI1_Init()
 */
bool mx25l_init(void)
{
	OSPI_RegularCmdTypeDef c;
	uint8_t id[3];

	if (!cmd_simple(CMD_RSTEN) || !cmd_simple(CMD_RST))
		return false;
	HAL_Delay(1);		// tRST

	cmd_setup(&c, CMD_RDID);
	c.DataMode = HAL_OSPI_DATA_1_LINE;
	c.NbData = sizeof(id);

	if (HAL_OSPI_Command(&hospi1, &c, TIMEOUT_MS) != HAL_OK)
		return false;
	if (HAL_OSPI_Receive(&hospi1, id, TIMEOUT_MS) != HAL_OK)
		return false;

	return id[0] == 0xC2 && id[1] == 0x20 && id[2] == 0x16;
}

bool mx25l_read(uint32_t addr, uint8_t *data, uint32_t len)
{
	OSPI_RegularCmdTypeDef c;

	if (len == 0)
		return true;
	if (addr >= MX25L_SIZE || len > MX25L_SIZE - addr)
		return false;

	cmd_setup(&c, CMD_READ_FAST);
	c.Address = addr;
	c.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
	c.DummyCycles = 8;
	c.DataMode = HAL_OSPI_DATA_1_LINE;
	c.NbData = len;

	if (HAL_OSPI_Command(&hospi1, &c, TIMEOUT_MS) != HAL_OK)
		return false;
	return HAL_OSPI_Receive(&hospi1, data, 1000) == HAL_OK;
}

static bool page_program(uint32_t addr, const uint8_t *data, uint32_t len)
{
	OSPI_RegularCmdTypeDef c;

	if (!write_enable())
		return false;

	cmd_setup(&c, CMD_PP);
	c.Address = addr;
	c.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
	c.DataMode = HAL_OSPI_DATA_1_LINE;
	c.NbData = len;

	if (HAL_OSPI_Command(&hospi1, &c, TIMEOUT_MS) != HAL_OK)
		return false;
	if (HAL_OSPI_Transmit(&hospi1, (uint8_t *) data, TIMEOUT_MS) != HAL_OK)
		return false;

	return wait_ready(TIMEOUT_PP_MS);
}

/*
 * Program data, the target area must be erased. Splits on page boundaries.
 */
bool mx25l_write(uint32_t addr, const uint8_t *data, uint32_t len)
{
	if (addr >= MX25L_SIZE || len > MX25L_SIZE - addr)
		return false;

	while (len > 0)
	{
		uint32_t chunk = MX25L_PAGE_SIZE - (addr % MX25L_PAGE_SIZE);

		if (chunk > len)
			chunk = len;
		if (!page_program(addr, data, chunk))
			return false;

		addr += chunk;
		data += chunk;
		len -= chunk;
	}
	return true;
}

bool mx25l_erase_sector(uint32_t addr)
{
	OSPI_RegularCmdTypeDef c;

	if (addr >= MX25L_SIZE)
		return false;
	if (!write_enable())
		return false;

	cmd_setup(&c, CMD_SE);
	c.Address = addr & ~(MX25L_SECTOR_SIZE - 1);
	c.AddressMode = HAL_OSPI_ADDRESS_1_LINE;

	if (HAL_OSPI_Command(&hospi1, &c, TIMEOUT_MS) != HAL_OK)
		return false;

	return wait_ready(TIMEOUT_SE_MS);
}
