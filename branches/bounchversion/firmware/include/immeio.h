

void eraserow(u8 row);
void erasescreen();
void drawstr(u8 row, u8 col, char *str);
void drawint(u8 row, u8 col, u16 val);
void drawhex(u8 row, u8 col, u16 val);
void poll_keyboard();
void usb_up(void);
// Set a clock rate of approx. 2.5 Mbps for 26 MHz Xtal clock
#define SPI_BAUD_M  170
#define SPI_BAUD_E  16
