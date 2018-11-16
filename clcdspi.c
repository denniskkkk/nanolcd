//gcc testspi.c -o testspi 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include "lcdfont.h"
#include "lcdlib.h"

#define LDEVICE "/dev/spidev1.0"
#define uint8_t unsigned char
#define uint16_t unsigned int
#define uint32_t unsigned long

static const uint32_t mode = 2;    // MSB first
static const uint8_t bits = 8;     // 8bits
static const uint32_t speed = 8000000; // 8Mhz
#define BUFFER_MAX 3
#define DIRECTION_MAX 64
// gpio number = (PA=0 PG=6) * 32 + (0-31)
#define GPIO_RST 6  // PG11  reset
#define GPIO_CS 13 // PA13 chip select 
#define GPIO_RS 17  // PA1  register select
volatile int spih;
volatile int gpioh;

int deviceopen(void) {
	char buffer[BUFFER_MAX];
	char path[DIRECTION_MAX];
	ssize_t bytes_written;
	int ret;

	spih = open(LDEVICE, O_RDWR);
	if(spih<0) {
        fprintf (stderr, "device not found!!!\n");
        return -1;
    }

	ret = ioctl(spih, SPI_IOC_WR_MODE32, &mode);
	if (ret<0) {
        fprintf (stderr, "spi mode not set!!!\n");
        return -1;
    }
	ret = ioctl(spih, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret<0) {
        fprintf (stderr, "spi bit word not set!!!\n");
        return -1;
    }

	ret = ioctl(spih, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret<0) {
        fprintf (stderr, "spi speed not set!!!\n");
        return -1;
    }

	//   
	gpioh = open("/sys/class/gpio/export", O_WRONLY);
	if (gpioh<0) return -1;    
	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", GPIO_CS);
	write(gpioh, buffer, bytes_written);
    close(gpioh);
    
    gpioh = open("/sys/class/gpio/export", O_WRONLY);
	if (gpioh<0) return -1;
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", GPIO_RS);
	write(gpioh, buffer, bytes_written);
	close(gpioh);

	gpioh = open("/sys/class/gpio/export", O_WRONLY);
	if (gpioh<0) return -1;    
	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", GPIO_RST);
	write(gpioh, buffer, bytes_written);
    close(gpioh);    

	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", GPIO_CS);
	gpioh = open(path, O_WRONLY);
    if (gpioh<0) return -1;
	write(gpioh, "out", GPIO_CS);
	close(gpioh);
    
    snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", GPIO_RS);
	gpioh = open(path, O_WRONLY);
    if (gpioh<0) return -1;
	write(gpioh, "out", GPIO_RS);
	close(gpioh);
	//
	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", GPIO_RST);
	gpioh = open(path, O_WRONLY);
    if (gpioh<0) return -1;
	write(gpioh, "out", GPIO_RST);
	close(gpioh);    

	return 0;
}

int deviceclose(void) {
#define BUFFER_MAX 3
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;

	close(spih);

	//
	gpioh = open("/sys/class/gpio/unexport", O_WRONLY);
	if (gpioh<0) {
        fprintf (stderr, "error close\n");
        return -1;
    }

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", GPIO_RST);
	write(gpioh, buffer, bytes_written);

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", GPIO_CS);
	write(gpioh, buffer, bytes_written);
    
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", GPIO_RS);
	write(gpioh, buffer, bytes_written);

	close(gpioh);

	return 0;
}

#define VALUE_MAX 30
// position of letter PA=0, PB=1, PG=6
// pin number = 0-7 
// param pin = (position of letter in alphabet - 1) * 32 + pin number
// value true:1 false:0
int gpioWrite(int pin, int value) {
	char path[VALUE_MAX];
	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	gpioh = open(path, O_WRONLY);
	if (gpioh < 0) { 
        fprintf (stderr, "error write value\n");
        return -1;
    }
	write(gpioh, (value == 0)? "0": "1", 1);
	close(gpioh);

	return 0;
}


// set #RESET
void devicereset(void) {
	gpioWrite(GPIO_RST, 0); // pull low reset
	usleep(1000);
	gpioWrite(GPIO_RST, 1); // return 1
}

// devsel = CS pin number, data write data, len of data write.
int spibulktransfer( uint8_t *txdata, uint8_t *rxdata, int len) {
	struct spi_ioc_transfer tr = {
			.tx_buf = (unsigned long)txdata,
			.rx_buf = (unsigned long)NULL,
			.len = len,
			.delay_usecs = 0,
			.speed_hz = speed,
			.bits_per_word = bits
		};
	return ioctl(spih, SPI_IOC_MESSAGE(1), &tr);
}

int spitransfer(uint8_t txdata) {
    uint8_t tmptx[1];
    tmptx[0] = txdata;
	struct spi_ioc_transfer tr = {
			.tx_buf = (unsigned long)tmptx,
			.rx_buf = (unsigned long)NULL,
			.len = 1,
			.delay_usecs = 0,
			.speed_hz = speed,
			.bits_per_word = bits
		};
	return ioctl(spih, SPI_IOC_MESSAGE(1), &tr);
}

void LCD_WRITE_REG(unsigned int index)
{
  unsigned int value_index;
  gpioWrite (GPIO_RS, 0);  // RS
  gpioWrite (GPIO_CS, 0);  // CS
  value_index = index;
  value_index = value_index >> 8;
  spitransfer (value_index);    //00000000 000000000
  value_index = index;
  value_index &= 0x00ff;
  spitransfer (index);
  gpioWrite (GPIO_RS, 1);
  gpioWrite (GPIO_CS, 1);
}

void LCD_SEND_COMMAND(unsigned int index, unsigned int data)
{
  //select command register
  gpioWrite (GPIO_RS, 0);
  gpioWrite (GPIO_CS, 0);
  spitransfer (index >> 8);  //00000000 000000000
  spitransfer (index);
  gpioWrite (GPIO_CS, 1);
  //send data
  gpioWrite (GPIO_RS, 1);
  gpioWrite (GPIO_CS, 0);
  spitransfer(data >> 8);  //00000000 000000000
  spitransfer(data);
  gpioWrite (GPIO_CS, 1);
}

void LCD_WRITE_COMMAND(unsigned int index, unsigned int data)
{
  //select command register
  gpioWrite (GPIO_RS, 0);
  gpioWrite (GPIO_CS, 0);
  spitransfer (index >> 8);  //00000000 000000000
  spitransfer (index);
  gpioWrite (GPIO_CS, 1);
  //send data
  gpioWrite (GPIO_RS, 1);
  gpioWrite (GPIO_CS, 0);
  spitransfer (data >> 8);  //00000000 000000000
  spitransfer (data);
  gpioWrite (GPIO_CS, 1);
  //printf ("wr command %02X, %02X\n",index,data);
}

void LCD_WRITE_DATA(unsigned int data)
{
  spitransfer(data >> 8);  //00000000 000000000
  spitransfer(data);
}

void lcd_init(void)
{
  LCD_WRITE_COMMAND( 0x000, 0x0001 ); /* oschilliation start */
  usleep (1000);
  /* Power settings */
  LCD_WRITE_COMMAND( 0x100, 0x0000 ); /*power supply setup*/
  LCD_WRITE_COMMAND( 0x101, 0x0000 );
  LCD_WRITE_COMMAND( 0x102, 0x3110 );
  LCD_WRITE_COMMAND( 0x103, 0xe200 );
  LCD_WRITE_COMMAND( 0x110, 0x009d );
  LCD_WRITE_COMMAND( 0x111, 0x0022 );
  LCD_WRITE_COMMAND( 0x100, 0x0120 );
  usleep( 2 );
  LCD_WRITE_COMMAND( 0x100, 0x3120 );
  usleep( 8000 );
  /* Display control */
  LCD_WRITE_COMMAND( 0x001, 0x0100 );
  LCD_WRITE_COMMAND( 0x002, 0x0000 );
  LCD_WRITE_COMMAND( 0x003, 0x1230 );
  LCD_WRITE_COMMAND( 0x006, 0x0000 );
  LCD_WRITE_COMMAND( 0x007, 0x0101 );
  LCD_WRITE_COMMAND( 0x008, 0x0808 );
  LCD_WRITE_COMMAND( 0x009, 0x0000 );
  LCD_WRITE_COMMAND( 0x00b, 0x0000 );
  LCD_WRITE_COMMAND( 0x00c, 0x0000 );
  LCD_WRITE_COMMAND( 0x00d, 0x0018 );
  /* LTPS control settings */
  LCD_WRITE_COMMAND( 0x012, 0x0000 );
  LCD_WRITE_COMMAND( 0x013, 0x0000 );
  LCD_WRITE_COMMAND( 0x018, 0x0000 );
  LCD_WRITE_COMMAND( 0x019, 0x0000 );

  LCD_WRITE_COMMAND( 0x203, 0x0000 );
  LCD_WRITE_COMMAND( 0x204, 0x0000 );

  LCD_WRITE_COMMAND( 0x210, 0x0000 );
  LCD_WRITE_COMMAND( 0x211, 0x00ef );
  LCD_WRITE_COMMAND( 0x212, 0x0000 );
  LCD_WRITE_COMMAND( 0x213, 0x013f );
  LCD_WRITE_COMMAND( 0x214, 0x0000 );
  LCD_WRITE_COMMAND( 0x215, 0x0000 );
  LCD_WRITE_COMMAND( 0x216, 0x0000 );
  LCD_WRITE_COMMAND( 0x217, 0x0000 );

  // Gray scale settings
  LCD_WRITE_COMMAND( 0x300, 0x5343);
  LCD_WRITE_COMMAND( 0x301, 0x1021);
  LCD_WRITE_COMMAND( 0x302, 0x0003);
  LCD_WRITE_COMMAND( 0x303, 0x0011);
  LCD_WRITE_COMMAND( 0x304, 0x050a);
  LCD_WRITE_COMMAND( 0x305, 0x4342);
  LCD_WRITE_COMMAND( 0x306, 0x1100);
  LCD_WRITE_COMMAND( 0x307, 0x0003);
  LCD_WRITE_COMMAND( 0x308, 0x1201);
  LCD_WRITE_COMMAND( 0x309, 0x050a);

  /* RAM access settings */
  LCD_WRITE_COMMAND( 0x400, 0x4027 );
  LCD_WRITE_COMMAND( 0x401, 0x0000 );
  LCD_WRITE_COMMAND( 0x402, 0x0000 ); /* First screen drive position (1) */
  LCD_WRITE_COMMAND( 0x403, 0x013f ); /* First screen drive position (2) */
  LCD_WRITE_COMMAND( 0x404, 0x0000 );

  LCD_WRITE_COMMAND( 0x200, 0x0000 );
  LCD_WRITE_COMMAND( 0x201, 0x0000 );

  LCD_WRITE_COMMAND( 0x100, 0x7120 );
  LCD_WRITE_COMMAND( 0x007, 0x0103 );
  usleep(1000);
  LCD_WRITE_COMMAND( 0x007, 0x0113 );
  usleep(1000);

}

void lcd_clear_screen(unsigned int bgcolor)
{
  unsigned int i, j;
  LCD_WRITE_COMMAND(0x210, 0x00);
  LCD_WRITE_COMMAND(0x212, 0x0000);
  LCD_WRITE_COMMAND(0x211, 0xEF);
  LCD_WRITE_COMMAND(0x213, 0x013F);
  LCD_WRITE_COMMAND(0x200, 0x0000);
  LCD_WRITE_COMMAND(0x201, 0x0000);
  gpioWrite (GPIO_RS, 0);
  LCD_WRITE_REG(0x202); //RAM Write index
  gpioWrite (GPIO_CS, 0);
  uint8_t *tmpbuf = calloc (1280, sizeof(uint8_t));
  for (int i =0; i < 640; i++) {
      *(tmpbuf + i * 2) = bgcolor >> 8;
      *(tmpbuf + i *2 + 1) =  bgcolor;
  }
  //memset (tmpbuf, 0x00, 320 * 2);
  
  for (j =0 ;j < 120; j++) {
  spibulktransfer (tmpbuf, NULL, 1280); 
  }
  free (tmpbuf);
  /*
  for (i = 0; i < 320; i++)
  {
    for (j = 0; j < 240; j++)
    {
      LCD_WRITE_DATA( color_background );
    }
  }
  */
  gpioWrite (GPIO_RS, 0);
  gpioWrite (GPIO_CS, 1);
}

void lcd_set_cursor(unsigned char x, unsigned int y)
{
  if ( (x > 320) || (y > 240) )
  {
    return;
  }
  LCD_WRITE_COMMAND( 0x200, x );
  LCD_WRITE_COMMAND( 0x201, y );
}

void lcd_display_char(  unsigned char ch_asc,
                        unsigned int color_front,
                        unsigned int color_background,
                        unsigned char postion_x,
                        unsigned char postion_y)
{
  unsigned char i, j, b;
  const unsigned char *p = 0;
  LCD_WRITE_COMMAND(0x210, postion_x * 8); //x start point
  LCD_WRITE_COMMAND(0x212, postion_y * 16); //y start point
  LCD_WRITE_COMMAND(0x211, postion_x * 8 + 7); //x end point
  LCD_WRITE_COMMAND(0x213, postion_y * 16 + 15); //y end point
  LCD_WRITE_COMMAND(0x200, postion_x * 8);
  LCD_WRITE_COMMAND(0x201, postion_y * 16);
  gpioWrite (GPIO_RS, 0);
  LCD_WRITE_REG(0x202);
  gpioWrite (GPIO_CS, 0);
  p = ascii;
  p += ch_asc * 16;
  for (j = 0; j < 16; j++)
  {
    b = *(p + j);
    for (i = 0; i < 8; i++)
    {
      if ( b & 0x80 )
      {
        LCD_WRITE_DATA(color_front);
      }
      else
      {
        LCD_WRITE_DATA(color_background);
      }
      b = b << 1;
    }
  }
  gpioWrite (GPIO_CS, 1);
}

void lcd_display_string(unsigned char *str, unsigned int color_front,
                        unsigned int color_background, unsigned char x, unsigned char y )
{
  while (*str)
  {
    lcd_display_char( *str, color_front, color_background, x, y);
    if (++x >= 30)
    {
      x = 0;
      if (++y >= 20)
      {
        y = 0;
      }
    }
    str ++;
  }
}

void dumphex (uint8_t *data, int len) {
  int c, q = 0, row =0;
  for (c =0; c < len ; c++) {
       printf ("%02X:", data[c]);   
       q++;
       if(q % 16 == 0) {
           printf ("\n");
           q = 0;
           
           row ++;
       }
  }
}
 
// test colours
const unsigned int color[] =
{
  // 0 ~ 262143, 0x00000 ~ 0x3FFFF
  0xf800, //red
  0x07e0, //green
  0x001f, //blue
  0xffe0, //yellow
  0x0000, //black
  0xffff, //white
  0x07ff, //light blue
  0xf81f  //purple
};
 
// test text
unsigned char txt[80] = "A quick brown fox ABCDEFG12345";
unsigned char txt1[80] = "******************************";
unsigned char txt2[80] = "!@#$%^&*()ABCDEFGHIJKLMNOP<>_+";

void testtext () {
  int c;
  for (c = 0; c < 18; c++) {
    lcd_display_string(txt, WHITE, color [c / 4 ], 0, c );
  }
  lcd_display_string(txt1, WHITE, BLUE, 0, 18 );
  lcd_display_string(txt2, WHITE, VIOLET, 0, 19 );
}
 
//ls /sys/class/gpio
// spi buf size max 4096
#define BUFSIZE 4096
int main () {
uint8_t txbuf[BUFSIZE];
int err = 0;
int c, l;
    for (c =0; c < BUFSIZE; c++) {
      txbuf [c] = 0x00;   
    }
    // init spi
    if ( deviceopen() < 0) {
        fprintf (stderr , "error setup io\n");
        return -1;
    }
    // reset
    devicereset();
    lcd_init();
    lcd_clear_screen(BLUE);
    for (c =0; c < 1000000; c++ ) {
    //devicereset();
    lcd_clear_screen(BLUE);
    //lcd_set_cursor (0,0);
    testtext();
    //lcd_clear_screen(BLUE);
    //sleep (3);
    }
    //

    //spibulktransfer ( txbuf, NULL, BUFSIZE);

    //deviceclose();
    return 0;
}
