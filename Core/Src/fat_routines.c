/*
 * fat_routines.c
 *
 *  Created on: 14 Oct 2026
 *      Author: marku
 *
 * FAT file system on the flash disk (FileX, custom driver fx_stm32_custom_driver.c).
 * The same disk is the USB drive on the PC.
 */

#include <string.h>
#include <stdio.h>
#include "main.h"
#include "uart_sci.h"
#include "iwdg.h"
#include "can_functions.h"
#include "app_filex.h"
#include "flash_disk.h"
#include "fat_routines.h"

static FX_MEDIA flash_media;
static ULONG flash_media_memory[FLASH_DISK_BLOCK_SIZE / sizeof(ULONG)];	// one sector cache
static CHAR dir_name[FX_MAX_LONG_NAME_LEN];
static FX_FILE flash_file;

/*
 * Number with thousand separators: 1234567 -> "1.234.567"
 */
static char *dir_number(char *buf, ULONG64 value)
{
  char tmp[24];
  int len = 0;
  char *d = buf;

  // no %llu in newlib nano printf
  do
  {
    tmp[len++] = '0' + (char) (value % 10);
    value /= 10;
  } while (value != 0);
  for (int i = 0; i < len / 2; i++)
  {
    char c = tmp[i];
    tmp[i] = tmp[len - 1 - i];
    tmp[len - 1 - i] = c;
  }

  for (int i = 0; i < len; i++)
  {
    if (i > 0 && (len - i) % 3 == 0)
      *d++ = '.';
    *d++ = tmp[i];
  }
  *d = 0;
  return buf;
}

/*
 * DOS style directory listing of the flash disk (FileX, same disk as the USB drive),
 * with the full (long) file name at the end of the line.
 * The media is opened and closed for every listing, so changes made by the PC over USB are seen.
 *
 *  Volume in drive A is HOMETFT
 *  Directory of A:\
 *
 * 14-10-2026  21:30    <DIR>          LOGS
 * 14-10-2026  21:30             1.234 CONFIG.TXT
 * 14-10-2026  21:31               512 Long file name.txt
 *         2 file(s)          1.746 bytes
 *         1 dir(s)       4.124.672 bytes free
 */
void print_dir(char *path)
{
  CHAR volume[12];
  char num[24];
  UINT attributes, year, month, day, hour, minute, second;
  ULONG size;
  ULONG64 total = 0, free_bytes = 0;
  UINT files = 0, dirs = 0;
  UINT status;

  if (!flash_disk_ready())
  {
    tty_printf("Flash disk not ready\r\n");
    return;
  }

  status = fx_media_open(&flash_media, "FLASH", fx_stm32_custom_driver, FX_NULL, flash_media_memory, sizeof(flash_media_memory));
  if (status != FX_SUCCESS)
  {
    tty_printf("Not ready reading drive A (error 0x%02X, not formatted?)\r\n", status);
    return;
  }

  if (fx_media_volume_get(&flash_media, volume, FX_DIRECTORY_SECTOR) == FX_SUCCESS && volume[0] != 0)
    tty_printf("\r\n Volume in drive A is %s\r\n", volume);
  else
    tty_printf("\r\n Volume in drive A has no label\r\n");

  if (path != NULL && path[0] != 0 && strcmp(path, "\\") != 0 && strcmp(path, "/") != 0)
  {
    if (fx_directory_default_set(&flash_media, path) != FX_SUCCESS)
    {
      tty_printf("Path not found\r\n");
      fx_media_close(&flash_media);
      return;
    }
    tty_printf(" Directory of A:\\%s\r\n\r\n", path[0] == '\\' || path[0] == '/' ? path + 1 : path);
  }
  else
    tty_printf(" Directory of A:\\\r\n\r\n");

  status = fx_directory_first_full_entry_find(&flash_media, dir_name, &attributes, &size,
                                              &year, &month, &day, &hour, &minute, &second);
  while (status == FX_SUCCESS)
  {
    if ((attributes & FX_VOLUME) == 0)
    {
      tty_printf("%02u-%02u-%04u  %02u:%02u", day, month, year, hour, minute);
      if (attributes & FX_DIRECTORY)
      {
        tty_printf("    <DIR>          ");
        dirs++;
      }
      else
      {
        tty_printf(" %17s ", dir_number(num, size));
        total += size;
        files++;
      }
      tty_printf("%s\r\n", dir_name);
    }

    status = fx_directory_next_full_entry_find(&flash_media, dir_name, &attributes, &size,
                                               &year, &month, &day, &hour, &minute, &second);
  }

  fx_media_extended_space_available(&flash_media, &free_bytes);

  tty_printf("%9u file(s) %14s bytes\r\n", files, dir_number(num, total));
  tty_printf("%9u dir(s)  %14s bytes free\r\n", dirs, dir_number(num, free_bytes));

  fx_media_close(&flash_media);
}

/*
 * Open the flash disk, false (with a message) when not possible.
 * Every function opens and closes the media, so changes made by the PC over USB are seen.
 */
static bool media_open(void)
{
  UINT status;

  if (!flash_disk_ready())
  {
    tty_printf("Flash disk not ready\r\n");
    return false;
  }

  status = fx_media_open(&flash_media, "FLASH", fx_stm32_custom_driver, FX_NULL, flash_media_memory, sizeof(flash_media_memory));
  if (status != FX_SUCCESS)
  {
    tty_printf("Not ready reading drive A (error 0x%02X, not formatted?)\r\n", status);
    return false;
  }
  return true;
}

/*
 * Text to the tty for long output (type), waits when the transmit buffer is full.
 * While waiting the CAN bus is handled: over a CAN shell the output needs it to be sent,
 * and the received messages (gateway data, shell input) are not lost.
 */
static void tty_write_wait(const char *buf, uint32_t len)
{
  uint32_t start = HAL_GetTick();

  if (is_can_shell())
  {
    // 8 bytes per CAN frame, wait until all frames of this part fit in the TX queue
    uint16_t frames = (uint16_t) ((len + 7) / 8);

    while (can_tx_free() < frames)
    {
      wdt_clr();
      handle_can();
      if ((HAL_GetTick() - start) > 1000)
        return;  // CAN does not send (no other node?), skip
    }
    tty_putsn((char *) buf, (uint16_t) len);
    return;
  }

  for (uint32_t i = 0; i < len; i++)
  {
    while (!tty_putc(buf[i]))
    {
      wdt_clr();
      handle_can();
      if ((HAL_GetTick() - start) > 1000)
        return;  // tty does not send, skip
    }
    start = HAL_GetTick();
  }
}

/*
 * true when the file exists
 */
bool file_exists(char *filename)
{
  bool found;

  if (!media_open())
    return false;

  found = (fx_file_open(&flash_media, &flash_file, filename, FX_OPEN_FOR_READ) == FX_SUCCESS);
  if (found)
    fx_file_close(&flash_file);

  fx_media_close(&flash_media);
  return found;
}

/*
 * Add the text at the end of the file, the file is made when not there.
 * The date and time of the file come from fx_system_date_set() / fx_system_time_set().
 */
bool file_add(char *filename, char *txt)
{
  UINT status;

  if (!media_open())
    return false;

  status = fx_file_create(&flash_media, filename);
  if (status != FX_SUCCESS && status != FX_ALREADY_CREATED)
  {
    tty_printf("Create %s error 0x%02X\r\n", filename, status);
    fx_media_close(&flash_media);
    return false;
  }

  status = fx_file_open(&flash_media, &flash_file, filename, FX_OPEN_FOR_WRITE);
  if (status == FX_SUCCESS)
  {
    status = fx_file_relative_seek(&flash_file, 0, FX_SEEK_END);
    if (status == FX_SUCCESS)
      status = fx_file_write(&flash_file, txt, strlen(txt));
    fx_file_close(&flash_file);
  }
  if (status != FX_SUCCESS)
    tty_printf("Write %s error 0x%02X\r\n", filename, status);

  fx_media_close(&flash_media);   // flushes the FAT and directory
  flash_disk_changed();           // PC must read the disk again
  return status == FX_SUCCESS;
}

/*
 * First line of the file without the line end, false when the file is not there or empty
 */
bool file_first_line(char *filename, char *line, uint16_t size)
{
  ULONG actual = 0;
  bool ok = false;

  if (size == 0 || !media_open())
    return false;

  if (fx_file_open(&flash_media, &flash_file, filename, FX_OPEN_FOR_READ) == FX_SUCCESS)
  {
    if (fx_file_read(&flash_file, line, size - 1, &actual) == FX_SUCCESS && actual > 0)
    {
      line[actual] = 0;
      line[strcspn(line, "\r\n")] = 0;
      ok = true;
    }
    fx_file_close(&flash_file);
  }

  fx_media_close(&flash_media);
  return ok;
}

/*
 * Show the contents of a (text) file, like DOS type
 */
void type_file(char *filename)
{
  char buf[64];
  ULONG actual;
  UINT status;

  if (filename == NULL || filename[0] == 0)
  {
    tty_printf("Required parameter missing\r\n");
    return;
  }

  if (!media_open())
    return;

  status = fx_file_open(&flash_media, &flash_file, filename, FX_OPEN_FOR_READ);
  if (status != FX_SUCCESS)
  {
    tty_printf("File not found - %s\r\n", filename);
    fx_media_close(&flash_media);
    return;
  }

  while (fx_file_read(&flash_file, buf, sizeof(buf), &actual) == FX_SUCCESS && actual > 0)
    tty_write_wait(buf, actual);
  tty_printf("\r\n");

  fx_file_close(&flash_file);
  fx_media_close(&flash_media);
}

/*
 * Delete a file, like DOS del
 */
void remove_file(char *filename)
{
  UINT status;

  if (filename == NULL || filename[0] == 0)
  {
    tty_printf("Required parameter missing\r\n");
    return;
  }

  if (!media_open())
    return;

  status = fx_file_delete(&flash_media, filename);
  if (status == FX_SUCCESS)
    tty_printf("Deleted %s\r\n", filename);
  else if (status == FX_NOT_FOUND)
    tty_printf("Could Not Find %s\r\n", filename);
  else
    tty_printf("Delete %s error 0x%02X\r\n", filename, status);

  fx_media_close(&flash_media);
  if (status == FX_SUCCESS)
    flash_disk_changed();
}
