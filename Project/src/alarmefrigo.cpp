#include "main.h"

#include "alarmefrigo.h"


#define CHECK_MSEC 5 // Read hardware every 5 msec

typedef unsigned char uint8_t;
#include "fsm.h"

#include "debouncer.h"
#include "blinker.h"
#include "timer.h"
#include "fsm.h"
#include "tictoc.h"

unsigned long elapsed_time(bool reset = true);
class Debouncer;



Debouncer magnetSwitch(DOOR_GPIO_Port, DOOR_Pin);
Debouncer pushButton(CTRL_GPIO_Port, CTRL_Pin);

Blinker redLedBlink(LED_GPIO_Port, LED_Pin, 500 / CHECK_MSEC, 500 / CHECK_MSEC);
Blinker buzzerBlink(BUZZER_GPIO_Port, BUZZER_Pin, 500 / CHECK_MSEC, 500 / CHECK_MSEC);
Timer eventTimer(1000/CHECK_MSEC);
Timer shutdownTimer(1000/CHECK_MSEC);

TicToc ticToc;


void alarmefrigo_setup()
{

  // Green light to show that we are working.
  ticToc.tic();
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

//  digitalWrite(GREEN_LED, HIGH);

  // Wakeup if magnet switch close or button pressed
  //pinMode(magnetSwitchPin, INPUT_PULLUP);
  //attachInterrupt(magnetSwitchPin, buttonISR, CHANGE);
  //attachInterrupt(PUSH2, buttonISR, CHANGE);

  FSM_Init();

  ticToc.wait(5000);

  // We can now turn off the green light.
//  digitalWrite(GREEN_LED, LOW);
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

}

void alarmefrigo_loop()
{
  ticToc.tic();

  redLedBlink.update();
  buzzerBlink.update();


  bool magnetSwitch_state_change = magnetSwitch.updateState();
  bool pushButton_state_change = pushButton.updateState();
  bool timeElapsed = eventTimer.update();

  if (magnetSwitch_state_change || pushButton_state_change)
    shutdownTimer.start(1);

  if (!magnetSwitch.state)
    FSM_DoorOpen();
  else
    FSM_DoorClose();

  if (pushButton.state)
    FSM_UserOveride();

  if (timeElapsed)
    FSM_Timer();

  // Sleep
  ticToc.wait(CHECK_MSEC);

  //if (shutdownTimer.update() && ActState==IDLE)
  //  suspend();
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
  redLedBlink.start(500 / CHECK_MSEC, 500 / CHECK_MSEC);
  buzzerBlink.stop();
//  greenLedBlink.stop();
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
  redLedBlink.start(100 / CHECK_MSEC, 500 / CHECK_MSEC);
  buzzerBlink.stop();
//  greenLedBlink.start();
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
    buzzerBlink.stop();

    // TODO:
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
  buzzerBlink.start();
//  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}
