//****************************************************************************//
//  MSP430 state machine
//  fsm.h
//
//  Describtion:
//    A simple state machine for the MSP430
//
//  Generated with Excel Table
//  Date:  07/06/2021        Time:  23:01:09
//
//****************************************************************************//

#ifndef FSM_H
#define FSM_H

//****************************************************************************//
// State table typedef
//****************************************************************************//
typedef struct
{
  void (*ptrFunct)(void);
  uint8_t NextState;
} FSM_STATE_TABLE;

extern uint8_t ActState;

#define NR_EVENTS 5
#define DOOROPEN 0
#define DOORCLOSE 1
#define USERPRESS 2
#define USERRELEASE 3
#define TIMER 4

#define NR_STATES 4
#define IDLE 0
#define WATCHING 1
#define ALARM 2
#define QUIET 3

//****************************************************************************//
// Function prototypes
//****************************************************************************//
// Initialize state machine
void FSM_Init(void);

// Event function "DoorOpen"
void FSM_DoorOpen(void);

// Event function "DoorClose"
void FSM_DoorClose(void);

// Event function "UserPress"
void FSM_UserPress(void);

// Event function "UserPress"
void FSM_UserRelease(void);

// Event function "Timer"
void FSM_Timer(void);

// Transition function "prepareWatching"
void FSM_prepareWatching(void);

// Transition function "prepareQuiet"
void FSM_prepareQuiet(void);

void FSM_userRelease(void);

// Transition function "prepareIdle"
void FSM_prepareIdle(void);

// Transition function "prepareAlarm"
void FSM_prepareAlarm(void);

//****************************************************************************//

#endif /* FSM_H */

//****************************************************************************//
