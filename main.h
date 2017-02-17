
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
  int log_edcl_read(uint64_t addr, int bytes, uint8_t *obuf);
  int log_edcl_write(uint64_t addr, int bytes, uint8_t *ibuf);
  void log_edcl(const char *fmt, ...);
  void edcl_bootstrap(int entry);
  void write_led(uint32_t data);
  int queue_block_read1(uint32_t tmp[], int max);
  void queue_read_array(volatile uint32_t * const sd_ptr, uint32_t cnt, uint32_t iobuf[]);
  uint32_t queue_read(volatile uint32_t * const sd_ptr);
  void queue_write(volatile uint32_t *const sd_ptr, uint32_t val, int flush);
  
  
#ifdef __cplusplus
};
#endif
