// See LICENSE for license details.

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#define DECLARE_CSR(x,y) enum {x=y};

#include "encoding.h"
#include "memory.h"
#include "bits.h"
#include "uart.h"

#define SYS_write 64
#define SYS_exit 93
#define SYS_stats 1234

// initialized in crt.S
int have_vec;

#define static_assert(cond) switch(0) { case 0: case !!(long)(cond): ; }

static long handle_frontend_syscall(long which, long arg0, long arg1, long arg2)
{

  // currently it must be SYS_write
  char *s = (char *)arg1;
  while(arg2--) {
    uart_send(*s++);
  }

  return 0;
}

// place-holder
#define read_csr(reg) reg
#define read_csr_safe(reg) reg
#define write_csr(reg, val)

#define NUM_COUNTERS 18
static long counters[NUM_COUNTERS];
static char* counter_names[NUM_COUNTERS];
static int handle_stats(int enable)
{
  int i = 0;
#define READ_CTR(name) do { \
    while (i >= NUM_COUNTERS) ; \
    long csr = read_csr_safe(name); \
    if (!enable) { csr -= counters[i]; counter_names[i] = #name; } \
    counters[i++] = csr; \
  } while (0)
  READ_CTR(mcycle);  READ_CTR(minstret);
  READ_CTR(0xcc0); READ_CTR(0xcc1); READ_CTR(0xcc2); READ_CTR(0xcc3);
  READ_CTR(0xcc4); READ_CTR(0xcc5); READ_CTR(0xcc6); READ_CTR(0xcc7);
  READ_CTR(0xcc8); READ_CTR(0xcc9); READ_CTR(0xcca); READ_CTR(0xccb);
  READ_CTR(0xccc); READ_CTR(0xccd); READ_CTR(0xcce); READ_CTR(0xccf);
#undef READ_CTR
  return 0;
}

void tohost_exit(long code)
{
  // halt
  if(code) {
    char str[] = "error! exit(0xFFFFFFFFFFFFFFFF)\n";
    int i;
    for (i = 0; i < 16; i++) {
      str[29-i] = (code & 0xF) + ((code & 0xF) < 10 ? '0' : 'a'-10);
      code >>= 4;
    }
    uart_send_string(str);
  }    
  uintptr_t mstatus = read_csr(mstatus);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPP, PRV_M);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPIE, 1);
  write_csr(mstatus, mstatus);
  write_csr(mepc, 0);
  _exit(0);
}

static char trap_rpt_buf [256];

long handle_trap(long cause, long epc, long regs[32])
{
  int* csr_insn;
  long sys_ret = 0;

  if (cause == CAUSE_ILLEGAL_INSTRUCTION &&
      (*(int*)epc & *csr_insn) == *csr_insn)
    ;                           /* why single this out? csrr/csrrs stats is OK */
  else if (cause != CAUSE_MACHINE_ECALL) {
    // do some report
    sprintf(trap_rpt_buf, "mcause=%0x\n", cause);
    uart_send_string(trap_rpt_buf);
    sprintf(trap_rpt_buf, "mepc=%0x\n", epc);
    uart_send_string(trap_rpt_buf);
    sprintf(trap_rpt_buf, "mbadaddr=%0x\n", read_csr_safe(mbadaddr));
    uart_send_string(trap_rpt_buf);
    sprintf(trap_rpt_buf, "einsn=%0x\n", *(int*)epc);
    uart_send_string(trap_rpt_buf);
    sprintf(trap_rpt_buf, "sp=%0x\n", regs[2]);
    uart_send_string(trap_rpt_buf);
    sprintf(trap_rpt_buf, "tp=%0x\n", regs[4]);
    uart_send_string(trap_rpt_buf);
    tohost_exit(1337);
  }
  else if (regs[17] == SYS_exit)
    tohost_exit(regs[10]);
  else if (regs[17] == SYS_stats)
    sys_ret = handle_stats(regs[10]);
  else
    sys_ret = handle_frontend_syscall(regs[17], regs[10], regs[11], regs[12]);

  regs[10] = sys_ret;
  return epc+4;
}

long syscall(long num, long arg0, long arg1, long arg2)
{
  abort();
}

void exit(int code)
{
  syscall(SYS_exit, code, 0, 0);
  while (1);
}

void setStats(int enable)
{
  syscall(SYS_stats, enable, 0, 0);
}

void printstr(const char* s)
{
  syscall(SYS_write, 1, (long)s, strlen(s));
}

void __attribute__((weak)) thread_entry(int cid, int nc)
{
  // multi-threaded programs override this function.
  // for the case of single-threaded programs, only let core 0 proceed.
  while (cid != 0);
}

int __attribute__((weak)) main(int argc, char** argv)
{
  // single-threaded programs override this function.
  printstr("Implement main(), foo!\n");
  return -1;
}

static void init_tls()
{
  // placeholder
}

void printhex(uint64_t x)
{
  char str[17];
  int i;
  for (i = 0; i < 16; i++)
  {
    str[15-i] = (x & 0xF) + ((x & 0xF) < 10 ? '0' : 'a'-10);
    x >>= 4;
  }
  str[16] = 0;

  printstr(str);
}
