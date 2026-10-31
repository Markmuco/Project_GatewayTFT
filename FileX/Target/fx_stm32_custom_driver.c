/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/

/* Private includes ----------------------------------------------------------*/
#include "fx_stm32_custom_driver.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* USER CODE BEGIN USER_CODE_SECTION_0 */
/*
 * FileX driver for the MX25L3233F, 512 byte sectors via flash_disk.c,
 * the same disk as the USB mass storage (ux_device_msc.c).
 * A partition table (MBR) written by Windows is supported: FileX adds
 * fx_media_hidden_sectors (partition start) to every logical sector.
 */
#include "flash_disk.h"

UINT _fx_partition_offset_calculate(void *partition_sector, UINT partition, ULONG *partition_start, ULONG *partition_size);

static UINT fx_flash_read(ULONG sector, UCHAR *buffer, ULONG sectors)
{
  return flash_disk_read(sector, buffer, sectors) ? FX_SUCCESS : FX_IO_ERROR;
}

static UINT fx_flash_write(ULONG sector, UCHAR *buffer, ULONG sectors)
{
  return flash_disk_write(sector, buffer, sectors) ? FX_SUCCESS : FX_IO_ERROR;
}

/* USER CODE END USER_CODE_SECTION_0 */

VOID  fx_stm32_custom_driver(FX_MEDIA *media_ptr)
{
  /* USER CODE BEGIN USER_CODE_SECTION_1 */

  /* USER CODE END USER_CODE_SECTION_1 */

  switch (media_ptr->fx_media_driver_request)
  {
    case FX_DRIVER_INIT:
    {

     /* USER CODE BEGIN DRIVER_INIT */

     /* USER CODE END DRIVER_INIT */

      media_ptr->fx_media_driver_status = FX_SUCCESS;

     /* USER CODE BEGIN POST_DRIVER_INIT */
      media_ptr->fx_media_driver_status = flash_disk_ready() ? FX_SUCCESS : FX_IO_ERROR;

     /* USER CODE END POST_DRIVER_INIT */
      break;
    }

    case FX_DRIVER_UNINIT:
    {
     /* USER CODE BEGIN DRIVER_UNINIT */

     /* USER CODE END DRIVER_UNINIT */

      media_ptr->fx_media_driver_status = FX_SUCCESS;

    /* USER CODE BEGIN POST_DRIVER_UNINIT */

     /* USER CODE END POST_DRIVER_UNINIT */
      break;
    }

    case FX_DRIVER_BOOT_READ:
    {
    /* USER CODE BEGIN DRIVER_BOOT_READ */

     /* USER CODE END DRIVER_BOOT_READ */

      media_ptr->fx_media_driver_status = FX_SUCCESS;

    /* USER CODE BEGIN POST_DRIVER_BOOT_READ */
      {
        ULONG partition_start = 0;
        ULONG partition_size = 0;
        UINT status = fx_flash_read(0, media_ptr->fx_media_driver_buffer, 1);

        // sector 0 is a MBR (partition table)? then read the boot sector of the first partition
        if (status == FX_SUCCESS &&
            _fx_partition_offset_calculate(media_ptr->fx_media_driver_buffer, 0, &partition_start, &partition_size) == FX_SUCCESS &&
            partition_start != 0)
        {
          status = fx_flash_read(partition_start, media_ptr->fx_media_driver_buffer, 1);
        }
        media_ptr->fx_media_driver_status = status;
      }

     /* USER CODE END POST_DRIVER_BOOT_READ */
      break;
    }

    case FX_DRIVER_READ:
    {
    /* USER CODE BEGIN DRIVER_READ */

     /* USER CODE END DRIVER_READ */

      media_ptr->fx_media_driver_status = FX_SUCCESS;

    /* USER CODE BEGIN POST_DRIVER_READ */
      media_ptr->fx_media_driver_status = fx_flash_read(
          (ULONG) (media_ptr->fx_media_driver_logical_sector + media_ptr->fx_media_hidden_sectors),
          media_ptr->fx_media_driver_buffer, media_ptr->fx_media_driver_sectors);

     /* USER CODE END POST_DRIVER_READ */
      break;
    }

    case FX_DRIVER_BOOT_WRITE:
    {
    /* USER CODE BEGIN DRIVER_BOOT_WRITE */

     /* USER CODE END DRIVER_BOOT_WRITE */

      media_ptr->fx_media_driver_status = FX_SUCCESS;

    /* USER CODE BEGIN POST_DRIVER_BOOT_WRITE */
      media_ptr->fx_media_driver_status = fx_flash_write((ULONG) media_ptr->fx_media_hidden_sectors,
          media_ptr->fx_media_driver_buffer, 1);

     /* USER CODE END POST_DRIVER_BOOT_WRITE */
      break;
    }

    case FX_DRIVER_WRITE:
    {

    /* USER CODE BEGIN DRIVER_WRITE */

     /* USER CODE END DRIVER_WRITE */

      media_ptr->fx_media_driver_status = FX_SUCCESS;

     /* USER CODE BEGIN POST_DRIVER_WRITE */
      media_ptr->fx_media_driver_status = fx_flash_write(
          (ULONG) (media_ptr->fx_media_driver_logical_sector + media_ptr->fx_media_hidden_sectors),
          media_ptr->fx_media_driver_buffer, media_ptr->fx_media_driver_sectors);

     /* USER CODE END POST_DRIVER_WRITE */
      break;
    }

    case FX_DRIVER_FLUSH:
    {
    /* USER CODE BEGIN DRIVER_FLUSH */

     /* USER CODE END DRIVER_FLUSH */

      media_ptr->fx_media_driver_status = FX_SUCCESS;

    /* USER CODE BEGIN POST_DRIVER_FLUSH */
      // write through, nothing cached in the driver

     /* USER CODE END POST_DRIVER_FLUSH */
      break;
    }

    case FX_DRIVER_ABORT:
    {

    /* USER CODE BEGIN DRIVER_ABORT */

     /* USER CODE END DRIVER_ABORT */

      media_ptr->fx_media_driver_status = FX_SUCCESS;

    /* USER CODE BEGIN POST_DRIVER_ABORT */

     /* USER CODE END POST_DRIVER_ABORT */
      break;
    }

    default:
    {
        media_ptr->fx_media_driver_status = FX_IO_ERROR;
        break;
    }
  }
}

/* USER CODE BEGIN USER_CODE_SECTION_2 */

/* USER CODE END USER_CODE_SECTION_2 */

