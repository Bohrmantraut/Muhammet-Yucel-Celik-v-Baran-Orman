/*
 * retarget.c
 *
 *  Created on: Jan 2, 2026
 *      Author: muham
 */

#ifndef SRC_RETARGET_C_
#define SRC_RETARGET_C_



#endif /* SRC_RETARGET_C_ */


#include "main.h"
#include <errno.h>
#include <sys/unistd.h>   // STDOUT_FILENO, STDERR_FILENO

extern UART_HandleTypeDef huart2;

int _write(int file, char *ptr, int len)
{
  if (file == STDOUT_FILENO || file == STDERR_FILENO)
  {
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
  }
  errno = EBADF;
  return -1;
}

int _read(int file, char *ptr, int len)
{
  (void)file; (void)ptr; (void)len;
  errno = EBADF;
  return -1;
}

int _close(int file)
{
  (void)file;
  errno = EBADF;
  return -1;
}

int _lseek(int file, int ptr, int dir)
{
  (void)file; (void)ptr; (void)dir;
  errno = EBADF;
  return -1;
}

