#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <libusb-1.0/libusb.h>
#include <rtl-sdr.h>
#include <libhackrf/hackrf.h>


#if 1
#include <stdio.h>
#define PRINTF(__s, ...) fprintf(stderr, __s, ##__VA_ARGS__)
#define TRACE() PRINTF("[t] %s,%u\n", __FILE__, __LINE__)
#define ERROR() PRINTF("[e] %s,%u\n", __FILE__, __LINE__)
#else
#define TRACE()
#define ERROR()
#define PRINTF(...)
#endif


typedef struct rtlsdr_dev
{
  hackrf_device* devh;
  uint32_t freq;
  uint32_t sampl_rate;
  int gain;
  volatile int async_status;
} rtlsdr_dev_t;


uint32_t rtlsdr_get_device_count(void)
{
  ssize_t i;
  libusb_context *ctx;
  libusb_device **list;
  uint32_t device_count = 0;
  struct libusb_device_descriptor dd;
  ssize_t cnt;

  libusb_init(&ctx);

  cnt = libusb_get_device_list(ctx, &list);
  for (i = 0; i < cnt; i++)
  {
    libusb_get_device_descriptor(list[i], &dd);

    if (dd.idVendor != 0x1d50) continue ;
    if (dd.idProduct != 0x6089) continue ;

    device_count++;
  }

  libusb_free_device_list(list, 1);
  libusb_exit(ctx);

  return device_count;
}

const char* rtlsdr_get_device_name
(
 uint32_t index
)
{
  return "hackrf one";
}

int rtlsdr_get_device_usb_strings
(
 uint32_t index,
 char *manufact,
 char *product,
 char *serial
)
{
  if (manufact != NULL) strcpy(manufact, "great scott gadgets");
  if (product != NULL) strcpy(product, "hackrf one");
  if (serial != NULL) strcpy(serial, "none");
  return 0;
}

int rtlsdr_get_index_by_serial
(
 const char *serial
)
{
  int i, cnt, r;
  char str[256];

  if (!serial) return -1;

  cnt = rtlsdr_get_device_count();
  if (!cnt) return -2;

  for (i = 0; i < cnt; i++)
  {
    r = rtlsdr_get_device_usb_strings(i, NULL, NULL, str);
    if (!r && !strcmp(serial, str)) return i;
  }

  return -3;
}

int rtlsdr_open
(
 rtlsdr_dev_t **devp,
 uint32_t index
)
{
  rtlsdr_dev_t* dev;
  int err = 0;

  if (hackrf_init())
  {
    err = -1;
    goto on_error_0;
  }

  dev = malloc(sizeof(rtlsdr_dev_t));
  if (dev == NULL)
  {
    err = -ENOMEM;
    goto on_error_1;
  }

  if (hackrf_open(&dev->devh))
  {
    err = -1;
    goto on_error_2;
  }

  if (hackrf_set_lna_gain(dev->devh, 32))
  {
    err = -1;
    goto on_error_3;
  }
  dev->gain = 32;

  dev->async_status = 0;

  *devp = dev;
  return 0;

 on_error_3:
  hackrf_close(dev->devh);
 on_error_2:
  free(dev);
 on_error_1:
  hackrf_exit();
 on_error_0:
  *devp = NULL;
  return err;
}

int rtlsdr_close
(
 rtlsdr_dev_t *dev
)
{
  hackrf_close(dev->devh);
  hackrf_exit();
  free(dev);
  return 0;
}

int rtlsdr_set_xtal_freq
(
 rtlsdr_dev_t *dev,
 uint32_t rtl_freq,
 uint32_t tuner_freq
)
{
  if (rtl_freq != tuner_freq)
  {
    ERROR();
    return -1;
  }

  if (hackrf_set_freq(dev->devh, (double)rtl_freq))
  {
    ERROR();
    return -1;
  }

  dev->freq = rtl_freq;

  return 0;
}

int rtlsdr_get_xtal_freq
(
 rtlsdr_dev_t *dev,
 uint32_t *rtl_freq,
 uint32_t *tuner_freq
)
{
  *rtl_freq = dev->freq;
  *tuner_freq = dev->freq;
  return 0;
}

int rtlsdr_get_usb_strings
(
 rtlsdr_dev_t *dev,
 char *manufact,
 char *product,
 char *serial
)
{
  return rtlsdr_get_device_usb_strings
    (0, manufact, product, serial);
}

int rtlsdr_write_eeprom
(
 rtlsdr_dev_t *dev,
 uint8_t *data,
 uint8_t offset,
 uint16_t len
)
{
  ERROR();
  return -1;
}

int rtlsdr_read_eeprom
(
 rtlsdr_dev_t *dev,
 uint8_t *data,
 uint8_t offset,
 uint16_t len
)
{
  ERROR();
  return -1;
}

int rtlsdr_set_center_freq
(
 rtlsdr_dev_t *dev,
 uint32_t freq
)
{
  return rtlsdr_set_xtal_freq(dev, freq, freq);
}

uint32_t rtlsdr_get_center_freq
(
 rtlsdr_dev_t *dev
)
{
  uint32_t f;
  if (rtlsdr_get_xtal_freq(dev, &f, &f)) return -1;
  return f;
}

int rtlsdr_set_freq_correction
(
 rtlsdr_dev_t *dev,
 int ppm
)
{
  return 0;
}

int rtlsdr_get_freq_correction
(
 rtlsdr_dev_t *dev
)
{
  return 0;
}

enum rtlsdr_tuner rtlsdr_get_tuner_type
(
 rtlsdr_dev_t *dev
)
{
  return RTLSDR_TUNER_UNKNOWN;
}

int rtlsdr_get_tuner_gains
(
 rtlsdr_dev_t *dev,
 int *gainsp
)
{
  static const int gains[] = { 0, 8, 16, 24, 32, 40 };
  memcpy(gainsp, gains, sizeof(gains));
  return sizeof(gains) / sizeof(gains[0]);
}

int rtlsdr_set_tuner_gain
(
 rtlsdr_dev_t *dev,
 int gain
)
{
  uint32_t lna_gain;

  lna_gain = (uint32_t)gain;
  if (lna_gain % 8) lna_gain = (lna_gain + 8) & ~(8 - 1);
  if (lna_gain > 40) lna_gain = 40;

  return hackrf_set_lna_gain(dev->devh, lna_gain);
}

int rtlsdr_get_tuner_gain
(
 rtlsdr_dev_t *dev
)
{
  return dev->gain;
}

int rtlsdr_set_tuner_if_gain
(
 rtlsdr_dev_t *dev,
 int stage,
 int gain
)
{
  return rtlsdr_set_tuner_gain(dev, gain);
}

int rtlsdr_set_tuner_gain_mode
(
 rtlsdr_dev_t *dev,
 int manual
)
{
  return 0;
}

int rtlsdr_set_sample_rate
(
 rtlsdr_dev_t *dev,
 uint32_t rate
)
{
  if (hackrf_set_sample_rate(dev->devh, (double)rate))
  {
    ERROR();
    return -1;
  }

  dev->sampl_rate = rate;

  return 0;
}

uint32_t rtlsdr_get_sample_rate
(
 rtlsdr_dev_t *dev
)
{
  return dev->sampl_rate;
}

int rtlsdr_set_testmode
(
 rtlsdr_dev_t *dev,
 int on
)
{
  return 0;
}

int rtlsdr_set_agc_mode
(
 rtlsdr_dev_t *dev,
 int on
)
{
  return 0;
}

int rtlsdr_set_direct_sampling
(
 rtlsdr_dev_t *dev,
 int on
)
{
  if (on == 0) return -1;
  return 0;
}

int rtlsdr_get_direct_sampling
(
 rtlsdr_dev_t *dev
)
{
  return 1;
}

int rtlsdr_set_offset_tuning
(
 rtlsdr_dev_t *dev,
 int on
)
{
  return 0;
}

int rtlsdr_get_offset_tuning
(
 rtlsdr_dev_t *dev
)
{
  return 0;
}

int rtlsdr_reset_buffer
(
 rtlsdr_dev_t *dev
)
{
  return 0;
}

struct hackrf_rx_ctx
{
  rtlsdr_read_async_cb_t cb;
  void* ctx;
  void* buf;
  int len;
  volatile int count;
};

static int hackrf_rx_sync_cb(hackrf_transfer* trans)
{
  struct hackrf_rx_ctx* const rx_ctx = trans->rx_ctx;
  int len;
  if (rx_ctx->count) return 0;
  len = trans->valid_length;
  if (len > rx_ctx->len) len = rx_ctx->len;
  memcpy(rx_ctx->buf, trans->buffer, len);
  rx_ctx->len = len;
  rx_ctx->count = 1;
  return 0;
}

int rtlsdr_read_sync
(
 rtlsdr_dev_t *dev,
 void *buf,
 int len,
 int *n_read
)
{
  struct hackrf_rx_ctx rx_ctx;

  rx_ctx.buf = buf;
  rx_ctx.len = len;
  rx_ctx.count = 0;

  hackrf_start_rx(dev->devh, hackrf_rx_sync_cb, &rx_ctx);
  while (rx_ctx.count == 0) usleep(10);
  /* hackrf_stop_rx(dev->devh); */
  *n_read = rx_ctx.len;

  return 0;
}

int rtlsdr_wait_async
(
 rtlsdr_dev_t *dev,
 rtlsdr_read_async_cb_t cb,
 void *ctx
)
{
  return rtlsdr_read_async(dev, cb, ctx, 0, 0);
}

static int hackrf_rx_async_cb(hackrf_transfer* trans)
{
  struct hackrf_rx_ctx* const rx_ctx = trans->rx_ctx;

  rx_ctx->cb((unsigned char*)trans->buffer, (uint32_t)trans->valid_length, rx_ctx->ctx);
  return 0;
}

int rtlsdr_read_async
(
 rtlsdr_dev_t *dev,
 rtlsdr_read_async_cb_t cb,
 void *ctx,
 uint32_t buf_num,
 uint32_t buf_len
)
{
  struct hackrf_rx_ctx rx_ctx;
  int r;

  dev->async_status = 0;

  rx_ctx.cb = cb;
  rx_ctx.ctx = ctx;
  r = hackrf_start_rx(dev->devh, hackrf_rx_async_cb, (void*)&rx_ctx);
  if (r) return -1;

  while (dev->async_status == 0) usleep(1000000);

  hackrf_stop_rx(dev->devh);

  return 0;
}

int rtlsdr_cancel_async
(
 rtlsdr_dev_t *dev
)
{
  dev->async_status = 1;
  return 0;
}
