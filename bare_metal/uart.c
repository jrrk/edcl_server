// See LICENSE for license details.

#include <stdio.h>
#include <sys/ioctl.h>
#include "uart.h"
#include "minion_lib.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>

int fd, first = 1;

static struct termios oldt;

void normal(void)
{
  tcgetattr(fd, &oldt);
  oldt.c_lflag |= (ICANON|ECHO);
  oldt.c_iflag |= (ICRNL);
  tcsetattr(fd, 0, &oldt);
  close(fd);
  first = 1;
}

void cbreak(void)
{
  int rslt;
  first = 0;
  fd = open("/dev/tty", O_RDWR);
  rslt = tcgetattr(fd, &oldt);
  if (rslt) perror("tcgetattr");
  oldt.c_lflag &= ~ (ICANON|ECHO);
  oldt.c_iflag &= ~ (ICRNL);
  rslt = tcsetattr(fd, 0, &oldt);
  if (rslt) perror("tcsetattr");
  atexit(normal);
}

void uart_sendchar(char ch)
{
  if (first) cbreak();
  write(fd, &ch, 1);
}

int uart_getchar(void)
{
  int ch, cnt;
  if (first) cbreak();
  cnt = read(fd, &ch, 1);
  return (cnt < 0) ? 4 : ch == '\r' ? '\n' : ch;
}

int uart_tstchar(void)
{
  int ch;
  int rslt, cnt = 0;
  if (first) cbreak();
  rslt = ioctl(fd, FIONREAD, &cnt);
  if ((rslt < 0) | !cnt) return 0;
  return uart_getchar();
}

volatile uint32_t *uart_base_ptr = (uint32_t *)(UART_BASE);

void uart_init() {

}

void minion_console_putchar(unsigned char ch)
{
  static int addr_int = 0;
  volatile uint32_t * const video_base = (volatile uint32_t*)(10<<20);
  switch (ch)
    {
    case '\b':
      if (addr_int & 127) addr_int--;
      break;
    case '\r':
      break;
    case '\n':
      while ((addr_int & 127) < 127)
	queue_write(&(video_base[addr_int++]), ' ', 1);
      ++addr_int;
      break;
    default:
      queue_write(&video_base[addr_int++], ch, 0);
      break;
    }
  addr_int &= 4095;
}

void uart_send(uint8_t data) {
  uart_sendchar(data);
  //  minion_console_putchar(data);
}

void uart_send_string(const char *str) {
  while (*str) uart_send(*str++);
}

void uart_send_buf(const char *buf, const int32_t len) {
  int32_t i;
  for (i=0; i<len; i++) uart_send(buf[i]);
}

// IRQ triggered read
uint8_t uart_read_irq() {
  return uart_getchar();
}

uint8_t uart_recv() {
  int ch = uart_tstchar();
  // wait until RBR has data
  while(!ch)
    {
      uint32_t key;
      volatile uint32_t * const keyb_base = (volatile uint32_t*)(9<<20);
      key = queue_read(keyb_base);
      if ((1<<28) & ~key) /* FIFO not empty */
	{
	  int ch;
	  queue_write(keyb_base+1, 0, 0);
	  ch = (queue_read(keyb_base+1) >> 8) & 127; /* strip off the scan code (default ascii code is UK) */
	}
      else
	ch = uart_tstchar();  
      usleep(10000);
    }
  if (ch == '\r') ch = '\n'; /* translate CR to LF, because nobody else will */
  return ch;
}

// enable uart read IRQ
void uart_enable_read_irq() {
  *(uart_base_ptr + UART_IER) = 0x0001u;
}

// disable uart read IRQ
void uart_disable_read_irq() {
  *(uart_base_ptr + UART_IER) = 0x0000u;
}
