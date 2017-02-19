
#ifdef NOTDEF

static int sfd;

#define VERBOSE
int edcl_read(uint64_t addr, int bytes, uint8_t *obuf)
{
  int status;
  if (!sfd)
    sfd = client_main("localhost", "1234");
  status = client_read(addr, bytes, obuf);
#ifdef VERBOSE
  {
  char tmp[80];
  static char old[80];
  sprintf(tmp, "edcl_read(0x%.8llX, %d, 0x%.8llX);", addr, bytes, *(uint64_t *)obuf);
  if (strcmp(tmp,old))
    {
      printk("%s\n", tmp);
      strcpy(old,tmp);
    }
  }
#endif
  return status;
}

int edcl_write(uint64_t addr, int bytes, uint8_t *ibuf)
{
  if (!sfd)
    sfd = client_main("localhost", "1234");
#ifdef VERBOSE
  {
  char tmp[80];
  static char old[80];
  sprintf(tmp, "edcl_write(0x%.8llX, %d, 0x%.8llX);", addr, bytes, *(uint64_t *)ibuf);
  if (strcmp(tmp,old))
    { 
      printk("%s\n", tmp);
      strcpy(old,tmp);
    }
  }
#endif
  return client_write(addr, bytes, ibuf);
}

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
  edcl_write(0, sizeof(struct etrans), (uint8_t*)&tmp);
  edcl_read(0, sizeof(struct etrans), (uint8_t*)&tmp2);
  edcl_write(edcl_max*sizeof(struct etrans), sizeof(struct etrans), (uint8_t*)&tmp);
}

/* shared address space pointer (appears at 0x800000 in minion address map */
volatile static struct etrans *shared_base;
volatile uint32_t * const rxfifo_base = (volatile uint32_t*)(4<<20);
volatile uint32_t * const led_base = (volatile uint32_t*)(7<<20);

int shared_read(volatile struct etrans *addr, int cnt, struct etrans *obuf)
  {
    int rslt = edcl_read((uint64_t)addr, cnt*sizeof(struct etrans), (uint8_t*)obuf);
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
    return edcl_write((uint64_t)addr, cnt*sizeof(struct etrans), (uint8_t*)ibuf);
  }

int queue_flush(void)
{
  int cnt;
  struct etrans tmp;
  tmp.val = 0xDEADBEEF;
  edcl_trans[edcl_cnt++].mode = edcl_mode_unknown;
#ifdef VERBOSE
  {
    int i;
  printk("sizeof(struct etrans) = %ld\n", sizeof(struct etrans));
  for (i = 0; i < edcl_cnt; i++)
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

void queue_write(volatile uint32_t *const sd_ptr, uint32_t val, int flush)
 {
   struct etrans tmp;
   spin_lock(&edcl_queue_lock);
#if 0
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
   spin_unlock(&edcl_queue_lock);
 }

uint32_t queue_read(volatile uint32_t * const sd_ptr)
 {
   int cnt;
   struct etrans tmp;
   spin_lock(&edcl_queue_lock);
   tmp.mode = edcl_mode_read;
   tmp.ptr = sd_ptr;
   tmp.val = 0xDEADBEEF;
   edcl_trans[edcl_cnt++] = tmp;
   cnt = queue_flush();
   shared_read(shared_base+(cnt-2), 1, &tmp);
#ifdef VERBOSE
   printk("queue_read(%p, %p, 0x%x);\n", sd_ptr, tmp.ptr, tmp.val);
#endif   
   spin_unlock(&edcl_queue_lock);
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
   edcl_read((uint64_t)(shared_base+1), cnt*sizeof(uint32_t), (uint8_t*)tmpbuf);
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
  queue_write(led_base, data, 1);
}
#endif
