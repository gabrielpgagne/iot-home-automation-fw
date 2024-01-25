#define CHECK_MSEC 5 // Read hardware every 5 msec 
#define PRESS_MSEC 50 // Stable time before registering pressed 
#define RELEASE_MSEC 50 // Stable time before registering released

typedef unsigned char uint8_t;
#include "fsm.h"

#include "debouncer.h"
#include "blinker.h"
#include "timer.h"
#include "fsm.h"
#include "tictoc.h"

unsigned long elapsed_time(bool reset = true);
class Debouncer;

const uint8_t myLED = RED_LED;
const uint8_t myButton = PUSH2;

// Between VCC and pin, NO.
const uint8_t magnetSwitchPin = P1_4;


Debouncer magnetSwitch(magnetSwitchPin);
// S2 connect pin to ground, it is inverted.
Debouncer pushButton(PUSH2, true);
Blinker redLedBlink(RED_LED, 500 / CHECK_MSEC, 500 / CHECK_MSEC);
Blinker greenLedBlink(GREEN_LED, 500 / CHECK_MSEC, 500 / CHECK_MSEC);
Timer eventTimer(1000/CHECK_MSEC);
Timer shutdownTimer(1000/CHECK_MSEC);

TicToc ticToc;

void buttonISR()
{
  wakeup();
  shutdownTimer.start(1);
}


void setup() {
  pinMode(myLED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(PUSH2, INPUT_PULLUP);

  // Green light to show that we are working.
  ticToc.tic();
  digitalWrite(GREEN_LED, HIGH);
  
  // Wakeup if magnet switch close or button pressed
  pinMode(magnetSwitchPin, INPUT_PULLUP);
  attachInterrupt(magnetSwitchPin, buttonISR, CHANGE);
  attachInterrupt(PUSH2, buttonISR, CHANGE);

  FSM_Init();
  
  ticToc.wait(500);
  // We can now turn off the green light.
  digitalWrite(GREEN_LED, LOW);
}

void loop()
{
  ticToc.tic();

  redLedBlink.update();
  greenLedBlink.update();

  bool magnetSwitch_state_change = magnetSwitch.updateState();
  bool pushButton_state_change = pushButton.updateState();
  bool timeElapsed = eventTimer.update();

  if (magnetSwitch_state_change || pushButton_state_change)
    shutdownTimer.start(1);
      
  if (magnetSwitch.state)
    FSM_DoorOpen();
  else
    FSM_DoorClose();

  // S2 is to ground.
  if (!pushButton.state)
    FSM_UserOveride();

  if (timeElapsed)
    FSM_Timer();
    
  // Sleep
  ticToc.wait(CHECK_MSEC);

  if (shutdownTimer.update() && ActState==IDLE)
    suspend();
}

//****************************************************************************//
// Transition function "prepareWatching"
//****************************************************************************//
void FSM_prepareWatching (void)
{
  // Set timer to 30 sec
  // blink rled
  // stop buzzer
  // gled off
  eventTimer.start(10);
  redLedBlink.start();
  greenLedBlink.stop();
}


//****************************************************************************//
// Transition function "prepareQuiet"
//****************************************************************************//
void FSM_prepareQuiet (void)
{
  // stop buzzer
  // set timer to 30 mn
  // blink green led
  // rled off

  eventTimer.start(30 * 60);
  redLedBlink.stop();
  greenLedBlink.start();
}


//****************************************************************************//
// Transition function "prepareIdle"
//****************************************************************************//
void FSM_prepareIdle (void)
{
  // stop buzzer
  // stop timer
  // leds off
  // go to sleep

    eventTimer.stop();
    redLedBlink.stop();
    greenLedBlink.stop();

//    suspend();
}


//****************************************************************************//
// Transition function "prepareAlarm"
//****************************************************************************//
void FSM_prepareAlarm (void)
{
  // stop timer
  // start buzzer
  // gled off
  // rled on

  eventTimer.stop();
  redLedBlink.stop();
  digitalWrite(RED_LED, HIGH);
  greenLedBlink.stop();
}
