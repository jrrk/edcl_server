
typedef struct {
  uint8_t cmd;
  uint64_t addr;
  int bytes;
  uint8_t iobuf[8192];
} rw_struct_t;

#ifdef __cplusplus
extern "C" {
#endif
  int edcl_main(void);
  void edcl_close(void);
  int edcl_read(uint64_t addr, int bytes, uint8_t *obuf);
  int edcl_write(uint64_t addr, int bytes, uint8_t *ibuf);
#ifdef LOGEDCL
  int log_edcl_read(uint64_t addr, int bytes, uint8_t *obuf);
  int log_edcl_write(uint64_t addr, int bytes, uint8_t *ibuf);
  void log_edcl(const char *fmt, ...);
#endif
  void edcl_bootstrap(int entry);
  void write_led(uint32_t data);
  int queue_block_read1(uint32_t tmp[], int max);
  void queue_read_array(volatile uint32_t * const sd_ptr, uint32_t cnt, uint32_t iobuf[]);
  uint32_t queue_read(volatile uint32_t * const sd_ptr);
  void queue_write(volatile uint32_t *const sd_ptr, uint32_t val, int flush);
int edcl_main(void);
void edcl_close(void);
int edcl_read(uint64_t addr, int bytes, uint8_t *obuf);
int edcl_write(uint64_t addr, int bytes, uint8_t *ibuf);
int log_edcl_read(uint64_t addr, int bytes, uint8_t *obuf);
int log_edcl_write(uint64_t addr, int bytes, uint8_t *ibuf);
void log_edcl(const char *fmt, ...);
void edcl_bootstrap(int entry);
void write_led(uint32_t data);
int queue_block_read1(uint32_t tmp[], int max);
void queue_read_array(volatile uint32_t * const sd_ptr, uint32_t cnt, uint32_t iobuf[]);
uint32_t queue_read(volatile uint32_t * const sd_ptr);
void queue_write(volatile uint32_t *const sd_ptr, uint32_t val, int flush);
int client_write(uint64_t addr, int bytes, const uint8_t *ibuf);
int client_read(uint64_t addr, int bytes, uint8_t *obuf);
int client_main(char *host, char *port);
void client_send(char *buf, int len);
  int queue_flush(void);
  int queue_flush_cond(void);
  int simple_write(volatile uint32_t *const addr, int data);
  int simple_read(volatile uint32_t *const addr);

extern volatile uint32_t * const sd_base;
extern volatile uint32_t * const led_base;

#ifdef __cplusplus
};
#endif
