#ifndef DEV_MAP_HEADER
#define DEV_MAP_HEADER
#define DEV_MAP__io_ext_bram__BASE 0x40000000
#define DEV_MAP__io_ext_bram__MASK 0x1ffff
#define DEV_MAP__mem__BASE 0x80000000
#define DEV_MAP__mem__MASK 0x7ffffff
#define DEV_MAP__io_ext_flash__BASE 0x41000000
#define DEV_MAP__io_ext_flash__MASK 0xffffff
#define DEV_MAP__io_int_prci0__BASE 0x3000
#define DEV_MAP__io_int_prci0__MASK 0xfff
#define DEV_MAP__io_int_rtc__BASE 0x2000
#define DEV_MAP__io_int_rtc__MASK 0xfff
#define DEV_MAP__io_ext_uart__BASE 0x42000000
#define DEV_MAP__io_ext_uart__MASK 0x1fff
#define DEV_MAP__io_ext_spi__BASE 0x42002000
#define DEV_MAP__io_ext_spi__MASK 0x1fff
#define DEV_MAP__io_int_bootrom__BASE 0x0
#define DEV_MAP__io_int_bootrom__MASK 0x1fff
#endif // DEV_MAP_HEADER
