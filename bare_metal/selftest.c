// SD test program

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "elf.h"
#include "diskio.h"
#include "ff.h"
#include "uart.h"
#include "memory.h"
#include "encoding.h"
#include "bits.h"
#include "minion_lib.h"
#include "sdhci-minion-hash-md5.h"

int strcmp (const char *s1, const char *s2)
 {
  /* No checks for NULL */
  char *s1p = (char *)s1;
  char *s2p = (char *)s2;

  while (*s2p != '\0')
    {
      if (*s1p != *s2p)
        break;

      ++s1p;
      ++s2p;
    }
  return (*s1p - *s2p);
}

/* Read a text file and display it */

static int mounted = 0;
FATFS MyFatFs;   /* Work area (file system object) for logical drive */

void board_mmc_power_init(void);
void minion_dispatch(const char *ucmd);

int mount(int now)
{
  if (!mounted)
    {
      // Register work area to the default drive
      if(f_mount(&MyFatFs, "", now)) {
	printf("Fail to mount SD driver!\n");
	return 1;
      }
    }
  mounted = 1;
  return 0;
}

int open(FIL *filp, const char *path)
{
  /* Open a text file */
  FRESULT fr = f_open(filp, path, FA_READ);
  if (fr) {
    printf("failed to open %s!\n", path);
  } else {
    printf("%s opened\n", path);
    fr = FR_OK;
  }
  return (int)fr;
}

int opendir(DIR *dirp, const char *path)
{
  /* Open a directory */
  FRESULT fr = f_opendir(dirp, path);
  if (fr) {
    printf("failed to open %s!\n", path);
  } else {
    printf("%s opened\n", path);
    fr = FR_OK;
  }
  return (int)fr;
}

void displaydir(DIR *dirp)
{
  uint32_t br;            /* Read count */
  uint8_t buffer[64];     /* File copy buffer */
  /* Read all lines and display it */
  uint32_t idx;
  FILINFO fno;
  FRESULT fr;
  do {
    idx = dirp->index;
    memset(&fno, 0, sizeof(FILINFO));
    fr = f_readdir(dirp, &fno);  /* Read a dir item */
    printf("Number %d Name %s\n", dirp->index, fno.fname);
    }
  while ((dirp->index != idx) && (fr==FR_OK));
}

int closedir(DIR *filp)
{
  /* Close the dir */
  if(f_closedir(filp)) {
    printf("fail to close dir");
    return 1;
  }
  return 0;
}

void display(FIL *filp)
{
  uint32_t br;            /* Read count */
  uint8_t buffer[64];     /* File copy buffer */
  /* Read all lines and display it */
  uint32_t fsize = 0;
  for (;;) {
    FRESULT fr = f_read(filp, buffer, sizeof(buffer)-1, &br);  /* Read a chunk of source file */
    if (fr || br == 0) break; /* error or eof */
    buffer[br] = 0;
    printf("%s", buffer);
    fsize += br;
  }
  printf("file size %d\n", fsize);
}

int close(FIL *filp)
{
  /* Close the file */
  if(f_close(filp)) {
    printf("fail to close file!");
    return 1;
  }
  return 0;
}

int unmount(void)
{
  if (mounted)
    {
      if(f_mount(NULL, "", 1)) {         /* unmount it */
	printf("fail to umount disk!");
	return 1;
      }
    }
  mounted = 0;
}

int sdcard_test(const char *nam)
{  
  uint32_t i;
  FIL fil;                /* File object */
  FRESULT fr;             /* FatFs return code */
  
  /* Register work area to the default drive */

  if (!mounted)
    {
      if (mount(1)) return 1;
    }
  
  if (open(&fil, nam)) return 1;

  display(&fil);

  if (close(&fil)) return 1;

  printf("%s closed.\n", nam);

  return 0;
}

int show_dir(const char *nam)
{  
  DIR fil;                /* Dir object */
  
  /* Register work area to the default drive */

  if (!mounted)
    {
      if (mount(1)) return 1;
    }
  
  if (opendir(&fil, nam)) return 1;

  displaydir(&fil);

  if (closedir(&fil)) return 1;

  printf("%s closed.\n", nam);

  return 0;
}

// A dram test program

unsigned long long lfsr64(unsigned long long d) {
  // x^64 + x^63 + x^61 + x^60 + 1
  unsigned long long bit = 
    (d >> (64-64)) ^
    (d >> (64-63)) ^
    (d >> (64-61)) ^
    (d >> (64-60)) ^
    1;
  return (d >> 1) | (bit << 63);
}

// max size of file image is 16M
#define MAX_FILE_SIZE 0x1000000

// size of DDR RAM (128M for NEXYS4-DDR) 
#define DDR_SIZE 0x8000000

// 4K size read burst
#define SD_READ_SIZE 4096

static uint8_t *boot_file_buf;
static uint8_t *memory_base = (uint8_t *)((uint64_t *)(DEV_MAP__mem__BASE));
static char kernel[32];

int prepare (const char *cmdline)
{
  FIL fil;                // File object
  FRESULT fr;             // FatFs return code
  uint8_t buffer[64];     /* checksum buffer */
  uint32_t br = 0;                  // Read count

  printf("lowRISC boot program\n=====================================\n");
  if (cmdline[1])
    strcpy(kernel, cmdline+1);
  else strcpy(kernel, "boot.bin");

  memset(buffer, 0, sizeof(buffer));

  if (!mounted)
    {
      if (mount(1)) return 1;
    }
  
  /* Open a checksum file */
  if (!open(&fil, "boot.md5"))
    {
      int i;
      fr = f_read(&fil, buffer, sizeof(buffer)-1, &br);  /* Read a chunk of source file */
      for (i = 0; i < br; i++)
	{
	  if (buffer[i] == ' ')
	    buffer[i] = 0;
	}

      printf("Expected md5sum (from boot.md5) = %s\n", buffer);
      
      // Close the file
      if(f_close(&fil)) {
	printf("fail to close md5sum file!");
	return 1;
      }
    }

  // Open a file
  printf("Load %s into memory\n", kernel);
  fr = f_open(&fil, kernel, FA_READ);
  if (fr) {
    printf("Failed to open %s\n", kernel);
    return (int)fr;
  }

  // Read file into memory
  boot_file_buf = (uint8_t *)sbrk(MAX_FILE_SIZE);
  uint8_t *buf = boot_file_buf;
  uint32_t fsize = 0;           // file size count
  md5_ctx_t context;
  char *hash_value;
  do {
    fr = f_read(&fil, boot_file_buf+fsize, SD_READ_SIZE, &br);  // Read a chunk of source file
    if (!fr)
      {
	uart_send("|/-\\"[(fsize/SD_READ_SIZE)&3]);
	uart_send('\b');
	fsize += br;
      }
  } while(!(fr || (br == 0)));

  // Close the file
  if(f_close(&fil)) {
    printf("fail to close %s!\n", kernel);
    return 1;
  }

  printf("Load %d chunks bytes to memory address %p from %s of %d bytes.\n", fsize, boot_file_buf, kernel, fsize);

  md5_begin(&context);
  md5_hash(&context, boot_file_buf, fsize);
  md5_end(&context);
  hash_value = (char *)hash_bin_to_hex(&context);

  if (strcmp(hash_value, (char *)buffer))
    printf("Expected sum %s, actual %s\n", buffer, hash_value);
  else
    printf("MD5 checksum %s OK\n", hash_value);
  
  return fsize;
}

static char linbuf[80];

void mygets(char *cmd)
{
  int ch;
  char *chp = cmd;
  do
    {
      ch = uart_recv();
      uart_send(ch);
      if (chp < linbuf+sizeof(linbuf))
	*chp++ = ch;
      else
	abort();
    }
  while (ch != '\n');
  *--chp = 0;
}

void old_init2(size_t addr, size_t addr2)
{
  int i, rca, busy, timeout = 0;
  size_t data, sdcmd, arg, setting;
  //  board_mmc_power_init();
  sd_cmd_start(0);
  sd_reset(0,1,1,1);
  sd_blkcnt(1);
  sd_blksize(0x200);
  get_card_status(0);
  if (card_status[12])
    {
    myputs("card slot is empty\n");
    return;
    }
  sd_transaction_v(0,0x00000000,0x0);
  sd_transaction_v(8,0x000001AA,0x1);
  do {
    sd_transaction_v(55,0x00000000,0x1);
    busy = sd_transaction_v(41,0x40300000,0x1);
  } while ((0x80000000U & ~busy) && (timeout++ < 100));
  sd_transaction_v(2,0x00000000,0x3);
  rca = sd_transaction_v(3,0x00000000,0x1);
  myputs("rca=");
  myputhex(rca, 8);
  myputchar('\n');
  sd_transaction_v(9,rca,0x3);
  sd_transaction_v(13,rca,0x1);
  sd_transaction_v(7,rca,0x1);
  sd_transaction_v(55,rca,0x1);
  sd_transaction_v(51,0x00000000,0x1);
  sd_transaction_v(55,rca,0x1);
  sd_transaction_v(13,0x00000000,0x1);
  sd_transaction_v(16,0x00000200,0x1);
  for (i = addr; i <= addr2; i++)
    {
      sd_transaction_v(17,i,0x15);
    }
#if 0
  sd_transaction_v(18,0x00000040,0x1);
  sd_transaction_v(12,0x00000000,0x1);
#endif
}

const char *scan(const char *start, size_t *data, int base)
{
  *data = 0;
  while (*start)
    {
      if (*start >= '0' && *start <= '9') *data = *data * base + *start++ - '0';
      else if (*start >= 'A' && *start <= 'F') *data = *data * base + *start++ - 'A' + 10;
      else if (*start >= 'a' && *start <= 'f') *data = *data * base + *start++ - 'a' + 10;
      else if (*start == ' ') ++start;
      else return start+1;
    }
  return start;
}

size_t mystrtol(const char *nptr, char **endptr, int base)
{
  size_t data;
  const char *last = scan(nptr, &data, base);
  if (endptr) *endptr = (char *)last;
  return data;
}

void show_sector(u8 *buf)
{
  int i;
  for (i = 0; i < 512; i++)
    {
      if ((i & 31) == 0)
	{
	  myputchar('\r');
	  myputchar('\n');
	  myputhex(i, 4);
	  myputchar(':');
	  myputchar(' ');
	}
      myputhex(buf[i], 2);
      myputchar(' ');
    }
  myputchar('\r');
  myputchar('\n');
}

void minion_dispatch(const char *ucmd)
{
  int i, rca, busy;
  size_t addr, addr2, data, sdcmd, arg, setting;
  const char *nxt;
  switch(*ucmd)
      {
      case 4:
	break;
      case 'c':
	card_response();
	break;
      case 'C':
	nxt = scan(ucmd+1, &data, 16);
	myputchar('C');
	myputchar(' ');
	myputhex(data, 8);
	myputchar('\n');
	sd_clk_div(data);
	break;
      case 'd':
	printf("DEV_MAP__io_ext_bram__BASE = %x\n", DEV_MAP__io_ext_bram__BASE);
	printf("DEV_MAP__io_ext_bram__MASK = %x\n", DEV_MAP__io_ext_bram__MASK);
	printf("DEV_MAP__mem__BASE = %x\n", DEV_MAP__mem__BASE);
	printf("DEV_MAP__mem__MASK = %x\n", DEV_MAP__mem__MASK);
	printf("DEV_MAP__io_ext_flash__BASE = %x\n", DEV_MAP__io_ext_flash__BASE);
	printf("DEV_MAP__io_ext_flash__MASK = %x\n", DEV_MAP__io_ext_flash__MASK);
	printf("DEV_MAP__io_int_prci0__BASE = %x\n", DEV_MAP__io_int_prci0__BASE);
	printf("DEV_MAP__io_int_prci0__MASK = %x\n", DEV_MAP__io_int_prci0__MASK);
	printf("DEV_MAP__io_int_rtc__BASE = %x\n", DEV_MAP__io_int_rtc__BASE);
	printf("DEV_MAP__io_int_rtc__MASK = %x\n", DEV_MAP__io_int_rtc__MASK);
	printf("DEV_MAP__io_ext_uart__BASE = %x\n", DEV_MAP__io_ext_uart__BASE);
	printf("DEV_MAP__io_ext_uart__MASK = %x\n", DEV_MAP__io_ext_uart__MASK);
	printf("DEV_MAP__io_ext_spi__BASE = %x\n", DEV_MAP__io_ext_spi__BASE);
	printf("DEV_MAP__io_ext_spi__MASK = %x\n", DEV_MAP__io_ext_spi__MASK);
	printf("DEV_MAP__io_int_bootrom__BASE = %x\n", DEV_MAP__io_int_bootrom__BASE);
	printf("DEV_MAP__io_int_bootrom__MASK = %x\n", DEV_MAP__io_int_bootrom__MASK);
	break;
      case 'D':
	show_dir(ucmd+1);
	break;
      case 'f':
	sdcard_test(ucmd+1);
	break;
      case 'h':
	nxt = scan(ucmd+1, &addr, 16);
	nxt = scan(nxt, &addr2, 16);
	do
	  {
	    u8 *buf = (u8*)minion_iobuf;
	    if (!addr2)
	      {
		myputchar('h');
		myputchar(' ');
		myputhex(addr, 8);
		myputchar(':');
		show_sector(buf);
	      }
	    else
	      {
		hash_buf(buf, 512);
	      }
	    ++addr;
	  }
	while ((addr <= addr2) && addr2);
	break;
      case 'i':
	nxt = scan(ucmd+1, &addr, 16);
	nxt = scan(nxt, &addr2, 16);
	if (!addr2) addr2 = addr;
	old_init2(addr, addr2);
	break;
      case 'I':
	nxt = scan(ucmd+1, &addr, 16);
	nxt = scan(nxt, &addr2, 16);
	if (!addr2) addr2 = addr;
	init_sd();
	for (i = addr; i <= addr2; i++) sd_read_sector1(i);
	mount(0);
	break;
      case 'l':
	nxt = scan(ucmd+1, &data, 16);
	myputchar('l');
	myputchar(' ');
	myputhex(data, 2);
	myputchar('\n');
	write_led(data);
      case 'm':
	if (mounted)
	  myputs("already mounted\n");
	else
	  mount(1);
	break;
      case 'P':
	addr = prepare(ucmd+1);
	break;
      case 'r':
	nxt = scan(ucmd+1, &addr, 16);
	myputchar('r');
	myputchar(' ');
	myputhex(addr, 8);
	myputchar(',');
	data = queue_read((unsigned *)addr);
	myputhex(data, 2);
	myputchar('\n');
	break;
      case 'R':
	nxt = scan(ucmd+1, &addr, 16);
	addr &= ~3;
	nxt = scan(nxt, &addr2, 16);
	while (addr <= addr2)
	  {
	    myputchar('R');
	    myputchar(' ');
	    myputhex(addr, 8);
	    myputchar(':');
	    data = queue_read((unsigned *)addr);
	    myputhex(data, 8);
	    myputchar('\n');
	    addr += 4;
	  }
	break;
      case 's':
	nxt = scan(ucmd+1, &sdcmd, 16);
	myputchar('s');
	myputchar(' ');
	myputhex(sdcmd,2);
	nxt = scan(nxt, &arg, 16);
	myputchar(',');
	myputhex(arg, 8);
	nxt = scan(nxt, &setting, 16);
	myputchar(',');
	myputhex(setting, 1);
        myputchar('\n');
	sd_transaction_v(sdcmd, arg, setting);
	myputchar('\n');
	break;
      case 't':
	nxt = scan(ucmd+1, &data, 16);
	myputchar('t');
	myputchar(' ');
	myputhex(data, 8);
	myputchar('\n');
	sd_timeout(data);
	break;
      case 'u':
	unmount();
	break;
      case 'W':
	nxt = scan(ucmd+1, &addr, 16);
	myputchar('W');
	myputchar(' ');
	myputhex(addr, 8);
	nxt = scan(nxt, &data, 16);
	myputchar(',');
	myputhex(data, 8);
	myputchar('\n');
	queue_write((unsigned *)addr, data, 0);
	break;
      case 'q':
	break;
      default: printf("%c: unknown command\n", *ucmd);
      }
}

int main (void)
{
  uart_init();
  board_mmc_power_init();  
  do {
    uart_send_string("selftest> ");
    mygets(linbuf);
    minion_dispatch(linbuf);
  } while (*linbuf != 'q');
  return 0;
}

