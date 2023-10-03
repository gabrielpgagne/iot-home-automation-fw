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


Debouncer doorSwitch(DOOR_GPIO_Port, DOOR_Pin);
Debouncer pushButton(CTRL_GPIO_Port, CTRL_Pin);

Blinker redLedBlink(LED_GPIO_Port, LED_Pin, 500 / CHECK_MSEC, 500 / CHECK_MSEC);
Blinker buzzerBlink(BUZZER_GPIO_Port, BUZZER_Pin, 500 / CHECK_MSEC, 500 / CHECK_MSEC);
Timer eventTimer(1);
Timer shutdownTimer(1);

TicToc ticToc;


void alarmefrigo_setup()
{
  // Green light to show that we are working.
  ticToc.tic();
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);


  FSM_Init();

  ticToc.wait(5000);

  // We can now turn off the green light.
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  shutdownTimer.start(1000/CHECK_MSEC);
}

void alarmefrigo_loop()
{
  ticToc.tic();

  redLedBlink.update();
  buzzerBlink.update();


  int doorswitch_state = doorSwitch.updateState();
  int pushButton_state = pushButton.updateState();
  bool timeElapsed = eventTimer.update();

  // Something will happen
  bool buttons_active = doorswitch_state==-1 || pushButton_state==-1;

  if (doorswitch_state == 1)
    FSM_DoorOpen();
  else if (doorswitch_state == 0)
    FSM_DoorClose();

  if (pushButton_state == 1)
    FSM_UserOveride();

  if (timeElapsed)
    FSM_Timer();


  if (shutdownTimer.update() && ActState==IDLE && !buttons_active)
  {
	  // Got to deep sleep until something happens.
	  suspend();
	  shutdownTimer.start(1000/CHECK_MSEC);
	  doorSwitch.resetCount();
	  pushButton.resetCount();
  }
  else
  {
	  // Sleep for the next CHECK_MSEC
	  ticToc.wait(CHECK_MSEC);
  }
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
  eventTimer.start(10 * 1000 / CHECK_MSEC);
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

  eventTimer.start(30 * 60 * 1000 / CHECK_MSEC);
  redLedBlink.start(50 / CHECK_MSEC, 1000 / CHECK_MSEC);
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

    shutdownTimer.start(1000/CHECK_MSEC);
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

	if (ActState!=DOOROPEN)
	{
		eventTimer.stop();
		redLedBlink.stop();
		if (false)
			buzzerBlink.start();
		else
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	}
}
