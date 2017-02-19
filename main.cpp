/**
 * @file
 * @copyright  Copyright 2016 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      Test application to verify UDP/EDCL transport library.
 */

//#include "api_core.h"
//#include "iservice.h"
#include "udp.h"
//#include "ithread.h"
//#include "ielfloader.h"
#include "main.h"
#include "edcl.h"
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <assert.h>

static char tmp[256], old[256];

int _edcl_read(uint64_t addr, int bytes, uint8_t *obuf)
{
  int status;
  status = EdclService_read(addr,bytes,obuf);
#ifdef VERBOSE
  sprintf(tmp, "edcl_read(0x%.8lX, %d, 0x%.8lX);", addr, bytes, *(uint64_t *)obuf);
  if (strcmp(tmp,old))
    {
      puts(tmp);
      strcpy(old,tmp);
    }
#endif
  return status;
}

int _edcl_write(uint64_t addr, int bytes, uint8_t *ibuf)
{
#ifdef VERBOSE
  sprintf(tmp, "edcl_write(0x%.8lX, %d, 0x%.8lX);", addr, bytes, *(uint64_t *)ibuf);
  if (strcmp(tmp,old))
    {
      puts(tmp);
      strcpy(old,tmp);
    }
#endif
  return EdclService_write(addr,bytes,ibuf);
}

#ifdef LOGEDCL
static int logf;

void edcl_close(void)
{
  if (logf) close(logf);
}

void log_edcl(const char *fmt, ...)
       {
           int n;
           int size = 100;     /* Guess we need no more than 100 bytes */
           char *p, *np;
           va_list ap;

           if ((p = (char *)malloc(size)) == NULL)
               return;

           while (1) {

               /* Try to print in the allocated space */

               va_start(ap, fmt);
               n = vsnprintf(p, size, fmt, ap);
               va_end(ap);

               /* Check error code */

               if (n < 0)
		 return;

               /* If that worked, return the string */

               if (n < size)
		 {
		   int actual;
		   if (!logf)
		     logf = open("/var/tmp/edcl.log", O_CREAT|O_TRUNC|O_WRONLY, 0700);
		   actual = write(logf, p, n);
		   assert(actual==n);
		   return;
		 }

               /* Else try again with more space */

               size = n + 1;       /* Precisely what is needed */

               if ((np = (char *)realloc (p, size)) == NULL) {
                   free(p);
                   return;
               } else {
                   p = np;
               }
           }
       }

int log_edcl_read(uint64_t addr, int bytes, uint8_t *obuf)
{
  int i, rslt = _edcl_read(addr, bytes, obuf);
  log_edcl("edcl_read(%X,%X) =>", addr, bytes);
  for (i = 0; i < bytes; i++) log_edcl(" %.2X", obuf[i]);
  log_edcl("\n");
}

int log_edcl_write(uint64_t addr, int bytes, uint8_t *ibuf)
{
  int i;
  log_edcl("edcl_write(%X,%X) =>", addr, bytes);
  for (i = 0; i < bytes; i++) log_edcl(" %.2X", ibuf[i]);
  log_edcl("\n");
  return _edcl_write(addr, bytes, ibuf);
}
#else
#define log_edcl_read _edcl_read
#define log_edcl_write _edcl_write
#endif

enum edcl_mode {
  edcl_mode_unknown,
  edcl_mode_read,
  edcl_mode_write,
  edcl_mode_block_read,
  edcl_mode_bootstrap,
  edcl_max=256};

#pragma pack(4)

static struct etrans {
  enum edcl_mode mode;
  volatile uint32_t *ptr;
  uint32_t val;
} edcl_trans[edcl_max+1];

#pragma pack()

static int edcl_cnt;

void edcl_bootstrap(int entry)
{
  struct etrans tmp, tmp2;
  tmp.val = 0xDEADBEEF;
  tmp.mode = edcl_mode_bootstrap;
  tmp.ptr = (uint32_t *)(size_t)entry;
  log_edcl_write(0, sizeof(struct etrans), (uint8_t*)&tmp);
  log_edcl_read(0, sizeof(struct etrans), (uint8_t*)&tmp2);
  log_edcl_write(edcl_max*sizeof(struct etrans), sizeof(struct etrans), (uint8_t*)&tmp);
}

/* shared address space pointer (appears at 0x800000 in minion address map */
volatile static struct etrans *shared_base;
volatile uint32_t * const rxfifo_base = (volatile uint32_t*)(4<<20);

int shared_read(volatile struct etrans *addr, int cnt, struct etrans *obuf)
  {
    int rslt = log_edcl_read((uint64_t)addr, cnt*sizeof(struct etrans), (uint8_t*)obuf);
#ifdef EDCL_VERBOSE4
    int i;
    for (i = 0; i < cnt; i++)
      {
	printk("shared_read(%d, %p) => %p,%x;\n", i, addr+i, obuf[i].ptr, obuf[i].val);
      }
#endif
    return rslt;
  }

int shared_write(volatile struct etrans *addr, int cnt, struct etrans *ibuf)
  {
#ifdef EDCL_VERBOSE4
    int i;
    for (i = 0; i < cnt; i++)
      {
	{
	  int j;
	  printk("shared_write(%d, %p, 0x%x, 0x%p);\n", i, addr+i, cnt, ibuf);
	  for (j = 0; j < sizeof(struct etrans); j++)
	    printk("%x ", ((volatile uint8_t *)(&addr[i]))[j]);
	  printk("\n");
	}
      }
#endif	
    return log_edcl_write((uint64_t)addr, cnt*sizeof(struct etrans), (uint8_t*)ibuf);
  }

int queue_flush(void)
{
  int cnt;
  struct etrans tmp;
  tmp.val = 0xDEADBEEF;
  edcl_trans[edcl_cnt++].mode = edcl_mode_unknown;
#ifdef VERBOSE
  printk("sizeof(struct etrans) = %d\n", sizeof(struct etrans));
  for (int i = 0; i < edcl_cnt; i++)
    {
      switch(edcl_trans[i].mode)
	{
	case edcl_mode_write:
	  printk("queue_mode_write(%p, 0x%x);\n", edcl_trans[i].ptr, edcl_trans[i].val);
	  break;
	case edcl_mode_read:
	  printk("queue_mode_read(%p, 0x%x);\n", edcl_trans[i].ptr, edcl_trans[i].val);
	  break;
	case edcl_mode_unknown:
	  if (i == edcl_cnt-1)
	    {
	    printk("queue_end();\n");
	    break;
	    }
	default:
	  printk("queue_mode %d\n", edcl_trans[i].mode);
	  break;
	}
    }
#endif
  shared_write(shared_base, edcl_cnt, edcl_trans);
  shared_write(shared_base+edcl_max, 1, &tmp);
  do {
#ifdef VERBOSE
    int i = 10000000;
    int tot = 0;
    while (i--) tot += i;
    printk("waiting for minion %x\n", tot);
#endif
    shared_read(shared_base, 1, &tmp);
  } while (tmp.ptr);
  tmp.val = 0;
  shared_write(shared_base+edcl_max, 1, &tmp);
  cnt = edcl_cnt;
  edcl_cnt = 1;
  edcl_trans[0].mode = edcl_mode_read;
  edcl_trans[0].ptr = (volatile uint32_t*)(8<<20);
  return cnt;
}

int queue_flush_cond(void)
{
  if (edcl_cnt != 1) queue_flush();
}

void queue_write(volatile uint32_t *const sd_ptr, uint32_t val, int flush)
 {
   struct etrans tmp;
#if 1
   flush = 1;
#endif   
   tmp.mode = edcl_mode_write;
   tmp.ptr = sd_ptr;
   tmp.val = val;
   edcl_trans[edcl_cnt++] = tmp;
   if (flush || (edcl_cnt==edcl_max-1))
     {
       queue_flush();
     }
#ifdef VERBOSE  
   printk("queue_write(%p, 0x%x);\n", tmp.ptr, tmp.val);
#endif
 }

uint32_t queue_read(volatile uint32_t * const sd_ptr)
 {
   int cnt;
   struct etrans tmp;
   tmp.mode = edcl_mode_read;
   tmp.ptr = sd_ptr;
   tmp.val = 0xDEADBEEF;
   edcl_trans[edcl_cnt++] = tmp;
   cnt = queue_flush();
   shared_read(shared_base+(cnt-2), 1, &tmp);
#ifdef VERBOSE
   printk("queue_read(%p, %p, 0x%x);\n", sd_ptr, tmp.ptr, tmp.val);
#endif   
   return tmp.val;
 }

void queue_read_array(volatile uint32_t * const sd_ptr, uint32_t cnt, uint32_t iobuf[])
 {
   int i, n, cnt2;
   struct etrans tmp;
   if (edcl_cnt+cnt >= edcl_max)
     {
     queue_flush();
     }
   for (i = 0; i < cnt; i++)
     {
       tmp.mode = edcl_mode_read;
       tmp.ptr = sd_ptr+i;
       tmp.val = 0xDEADBEEF;
       edcl_trans[edcl_cnt++] = tmp;
     }
   cnt2 = queue_flush();
   n = cnt2-1-cnt;
   shared_read(shared_base+n, cnt, edcl_trans+n);
   for (i = n; i < n+cnt; i++) iobuf[i-n] = edcl_trans[i].val;
 }

int queue_block_read1(uint32_t tmpbuf[], int max)
{
   int cnt = max;
   struct etrans tmp;
   queue_flush();
   tmp.mode = edcl_mode_block_read;
   tmp.ptr = rxfifo_base;
   tmp.val = 1;
   shared_write(shared_base, 1, &tmp);
   tmp.val = 0xDEADBEEF;
   shared_write(shared_base+edcl_max, 1, &tmp);
   do {
    shared_read(shared_base, 1, &tmp);
  } while (tmp.ptr);
#ifdef SDHCI_VERBOSE3
   printk("queue_block_read1 completed\n");
#endif
   if (cnt > tmp.mode) cnt = tmp.mode;
   log_edcl_read((uint64_t)(shared_base+1), cnt*sizeof(uint32_t), (uint8_t*)tmpbuf);
   return cnt;
}

void rx_write_fifo(uint32_t data)
{
  queue_write(rxfifo_base, data, 0);
}

uint32_t rx_read_fifo(void)
{
  return queue_read(rxfifo_base);
}

void write_led(uint32_t data)
{
  volatile uint32_t * const led_base = (volatile uint32_t*)(7<<20);
  queue_write(led_base, data, 1);
}
