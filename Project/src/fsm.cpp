//****************************************************************************//
//  MSP430 state machine
//  fsm.c
//
//  Description:
//    A simple state machine for the MSP430
//    Do not change code in here!!!
//
//  Generated with Excel Table
//  Date:  07/06/2021        Time:  23:01:00
//
//****************************************************************************//
#include <cstdint>
#include <stdint.h>
#include <stdio.h>
#include "fsm.h"

//****************************************************************************//
// Global variables
//****************************************************************************//
uint8_t  ActState;

const FSM_STATE_TABLE StateTable [NR_STATES][NR_EVENTS] =
{
  FSM_prepareWatching, WATCHING, NULL, IDLE, FSM_prepareQuiet, QUIET, NULL, IDLE,
  NULL, WATCHING, FSM_prepareIdle, IDLE, FSM_prepareQuiet, QUIET, FSM_prepareAlarm, ALARM,
  NULL, ALARM, FSM_prepareIdle, IDLE, FSM_prepareQuiet, QUIET, NULL, ALARM,
  NULL, QUIET, NULL, QUIET, FSM_prepareIdle, IDLE, FSM_prepareIdle, IDLE
};


//****************************************************************************//
// Initialize state machine
//****************************************************************************//
void FSM_Init (void)
{
  ActState = IDLE;
}


//****************************************************************************//
// Event function "DoorOpen"
//****************************************************************************//
void FSM_DoorOpen (void)
{
  if (StateTable[ActState][DOOROPEN].ptrFunct != NULL)
    StateTable[ActState][DOOROPEN].ptrFunct();

  ActState = StateTable[ActState][DOOROPEN].NextState;
}


//****************************************************************************//
// Event function "DoorClose"
//****************************************************************************//
void FSM_DoorClose (void)
{
  if (StateTable[ActState][DOORCLOSE].ptrFunct != NULL)
    StateTable[ActState][DOORCLOSE].ptrFunct();

  ActState = StateTable[ActState][DOORCLOSE].NextState;
}


//****************************************************************************//
// Event function "UserOveride"
//****************************************************************************//
void FSM_UserOveride (void)
{
  if (StateTable[ActState][USEROVERIDE].ptrFunct != NULL)
    StateTable[ActState][USEROVERIDE].ptrFunct();

  ActState = StateTable[ActState][USEROVERIDE].NextState;
}


//****************************************************************************//
// Event function "Timer"
//****************************************************************************//
void FSM_Timer (void)
{
  if (StateTable[ActState][TIMER].ptrFunct != NULL)
    StateTable[ActState][TIMER].ptrFunct();

  ActState = StateTable[ActState][TIMER].NextState;
}


//****************************************************************************//
