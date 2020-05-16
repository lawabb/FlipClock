#pragma once

// landscape right (USB bottom right) unless one of these is defined
//#define LCD_PORTRAIT      // Portrait, USB connector at bottom
//#define LCD_PORTRAIT_TOP    // Portrait, USB connector at top
//#define LCD_LANDSCAPE_LEFT  // Landscape, USB connector at top left

#ifdef LCD_PORTRAIT 
#define LCD_WIDTH  320
#define LCD_HEIGHT 480
#else               
#define LCD_WIDTH  480
#define LCD_HEIGHT 320
#endif

#define LCD_RD_BIT   B00000001  // A0
#define LCD_WR_BIT   B00000010
#define LCD_RS_BIT   B00000100
#define LCD_CS_BIT   B00001000
#define LCD_RST_BIT  B00010000  // A4

#define LCD_RD_PIN   A0
#define LCD_WR_PIN   A1     
#define LCD_RS_PIN   A2        
#define LCD_CS_PIN   A3       
#define LCD_RST_PIN  A4
// BUTTON            A5

// SD card not used
#define SD_SCK_PIN   13 // LED_BUILTIN
#define SD_DO_PIN    12 // SCL
#define SD_DI_PIN    11 // SDA
#define SD_SS_PIN    10 // BUTTON

#define LCD_OR_PORTC B00100000 // Keep A5 INPUT_PULLUP
#define LCD_OR_PORTB B00011100 // Keep 10 INPUT_PULLUP and 11, 12, (SDA & SCL) HIGH

// Blasts into TX, RX on PORTD and Pins 10, 11, 12, 13 on PORTB, pins on PORTC, EXCEPT what is set high by LCD_OR_PORTB & LCD_OR_PORTC
#define CMD(_cmd)   { PORTD =  (_cmd); PORTB = LCD_OR_PORTB |  (_cmd); PORTC = LCD_OR_PORTC | LCD_RST_BIT | LCD_RD_BIT;              PINC = LCD_WR_BIT; }
#define DATA(_data) { PORTD = (_data); PORTB = LCD_OR_PORTB | (_data); PORTC = LCD_OR_PORTC | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT; PINC = LCD_WR_BIT; }


class ILI948x
{
  public:
    static void Init();
    static void DisplayOn();
    static void Cmd(byte cmd);
    static void DataByte(byte data);
    static void DataWord(word data);
    static void ColourWord(word colour, unsigned long count);
    static void ColourByte(byte colour, unsigned long count);
    static unsigned long Window(word x,word y,word w,word h);
    static void ClearByte(byte colour);
    static void ClearWord(word colour);

    static void SetScrollLeft(bool left); // portrait direction
    
    static byte m_MADCTL0x36;
private:
    static const byte initialisation[];
};

