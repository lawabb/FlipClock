#include <avr/pgmspace.h>
#include "ILI948x.h"
#include "Leaves.h"
#include "LargeDigits.h"
#include "SmallChars.h"
#include "Pins.h"
#include "BTN.h"
#include "RTC.h"
#include "SimpleDLS.h"
#include "Clock.h"
#include "Config.h"
#include "Cubes.h"
#include "TriangleClock.h"
#include "PongClock.h"

// FlipClock, MEW 2019
//   Inspired by rather nice large LCD.  An experiment in graphics and animation, RLE coded digits shapes, omit some lines to create look of a tipped "leaf". Heroic measures to update LCD fast (playing loose and fast with port registers)
// + CubeClock
//   The colours are cool, experiment with coloured cubes, lozenge etc shapes are pre-computed and RLE coded
//   https://www.dafont.com/kubics-rube.font
// + Triangle Clock
//   An adaptation of an eariler TIN-like face, pretty colours
// + PongClock
//   There's a bit of program memory left, squeeze in a simple Pong Clock
//
// + Hacks to try to go "dimmer" at night (see ENABLE_BLANKING)
//   
// ***** Uno is at 99% of program storage space *****
// NOTE:
// The sketch includes a simple Daylight Saving Time adjustment class which is using a table of date for New Zealand

TriangleMesh mesh;
TriangleClock triangles(mesh);

// deal with mode which changes face randomly, every n minutes, n between 5 and 15 mins 
#define MIN_RANDOM_INTERVAL_MINS (5UL)
#define MAX_RANDOM_INTERVAL_MINS (15UL)
byte modeFace;
unsigned long faceChageTimer;
unsigned long faceChageTimeout;
void displayRandomMode()
{
  ILI948x::ClearByte(0);
  SmallChars::drawStr((LCD_WIDTH - 6*SmallChars::Width)/2, LCD_HEIGHT/2 - SmallChars::HalfHeight, "RANDOM", -1, 0xFFFF, false);
  delay(1000);
}

bool displayIsBlank = false;
bool blankingChecked = false;
bool blankFirstCheck = true;
bool BlankDisplay(bool blank)
{
  if (displayIsBlank != blank)
  {
    if (blank)
    {
      ILI948x::ClearByte(0);
    }
    displayIsBlank = blank;
    return true;
  }
  return false;
}

void setup()
{
  //Serial.begin(38400);   no room!
  
  ILI948x::Init();
  delay(100); // wait for the RTC to wake up?
  rtc.Setup();
  // Use NZ daylight savings:
  SimpleDLS::SetData(SimpleDLS::m_pNewZealandData);
  triangles.Init();

  config.Load();

  modeFace = config.modeFace;
  if (modeFace == Config::eRandom)
  {
    modeFace = Config::eFlip;
    ILI948x::DisplayOn();
    displayRandomMode();
  }
  faceChageTimer = millis();
  faceChageTimeout = random(MIN_RANDOM_INTERVAL_MINS*60000UL, MAX_RANDOM_INTERVAL_MINS*60000UL);
  
  if (modeFace == Config::eFlip)
  {
    clock.DrawBackground();
    clock.DrawTime();
    clock.DrawDate();
  }
  else if (modeFace == Config::eCubes)
  {
    cubes.DrawBackground();
    cubes.DrawTime();
    cubes.DrawDate();
  }
  else if (modeFace == Config::ePong)
  {
    pong_Init();
  }
  ILI948x::DisplayOn();

#ifdef LCD_LANDSCAPE_LEFT
  btn1Set.Init(PIN_BTN_1_SET);
  btn2Adj.Init(PIN_BTN_2_ADJ);
#else
  // swap the buttons, set is upper one
  btn1Set.Init(PIN_BTN_2_ADJ);
  btn2Adj.Init(PIN_BTN_1_SET);
#endif
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop()
{
  bool reInit = false;

  if (displayIsBlank)
  {
    if (btn1Set.CheckButtonPress() || btn2Adj.CheckButtonPress())
    {
      // button press when blank unblanks.
      displayIsBlank = false;
      reInit = true;
    }
  }
  else
  {
    if (btn1Set.CheckButtonPress()) // configure
    {
      // some consessions to make all 4 faces fit
      // for example, config switches to FlipClock mode
      config.Configure();
      ILI948x::ClearWord(backgroundColour);
      reInit = true;
    }
    else if (btn2Adj.CheckButtonPress())  // changes face
    {
      config.modeFace = (config.modeFace == Config::eRandom)?Config::eFlip:config.modeFace + 1;
      config.SaveFlags();
      modeFace = config.modeFace;
      reInit = true;
    }
  }
    
  if (!displayIsBlank)
  {
    if (config.modeFace == Config::eRandom)
    {
      if (reInit)
      {
        ILI948x::ClearByte(0);
        SmallChars::drawStr((LCD_WIDTH - 6*SmallChars::Width)/2, LCD_HEIGHT/2 - SmallChars::HalfHeight, "RANDOM", -1, 0xFFFF, false);
        delay(1000);
        modeFace = random(Config::eRandom);
        reInit = true;
      }
      else if (RTC::CheckPeriod(faceChageTimer, faceChageTimeout))
      {
        faceChageTimeout = random(MIN_RANDOM_INTERVAL_MINS*60000UL, MAX_RANDOM_INTERVAL_MINS*60000UL);
        modeFace = random(Config::eRandom);
        reInit = true;
      }
    }
    
    if (modeFace == Config::eFlip)
    {
      if (reInit)
      {
        clock.DrawBackground();
        clock.DrawTime();
        clock.DrawDate();
      }
      else
      {
        clock.CheckUpdate();
      }
    }
    else if (modeFace == Config::eCubes)
    {
      if (reInit)
      {
        cubes.DrawBackground();
        cubes.DrawTime();
        cubes.DrawDate();
      }
      else
      {
        cubes.CheckUpdate();
      }
    }
    else if (modeFace == Config::eTriangles)
    {
      if (reInit)
      {
        triangles.Init();
      }
      else
      {
        triangles.Loop();  
      }
    }
    else if (modeFace == Config::ePong)
    {
      if (reInit)
      {
        pong_Init();
      }
      else 
      {
        pong_Loop();  
      }
    }
  }
    
#ifdef ENABLE_BLANKING
  if (Config::blankCheckInterval)
  {
    if ((rtc.m_Minute % Config::blankCheckInterval) == 0 || blankFirstCheck)
    {
      // only check once on the hour 
      if (!blankingChecked)
      {
        int hour = SimpleDLS::GetDisplayHour24();
        BlankDisplay(hour < Config::blankOffHour || hour >= Config::blankOnHour);
        blankingChecked = true;
        blankFirstCheck = false;
      }
    }
    else
    {
      blankingChecked = false;
    }
  }  
#endif  
}

