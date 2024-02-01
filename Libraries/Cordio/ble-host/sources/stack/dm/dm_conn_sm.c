/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Device manager connection management state machine.
 *
 *  Copyright (c) 2009-2018 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019 Packetcraft, Inc.
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "dm_api.h"
#include "dm_main.h"
#include "dm_conn.h"
#include "sch_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Column position of next state */
#define DM_CONN_NEXT_STATE          0

/*! Column position of action */
#define DM_CONN_ACTION              1

/*! Number of columns in the state machine state tables */
#define DM_CONN_NUM_COLS            2

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! DM Conn state machine state tables */
static const uint8_t dmConnStateTbl[DM_CONN_SM_NUM_STATES][DM_CONN_NUM_MSGS][DM_CONN_NUM_COLS] =
{
  /* Idle state */
  {
  /* Event                                    Next state                   Action */
  /* API_OPEN */                             {DM_CONN_SM_ST_CONNECTING,    DM_CONN_SM_ACT_OPEN},
  /* API_CLOSE */                            {DM_CONN_SM_ST_IDLE,          DM_CONN_SM_ACT_NONE},
  /* API_ACCEPT */                           {DM_CONN_SM_ST_ACCEPTING,     DM_CONN_SM_ACT_ACCEPT},
  /* HCI_LE_CONN_CMPL_FAIL */                {DM_CONN_SM_ST_IDLE,          DM_CONN_SM_ACT_NONE},
  /* HCI_LE_CONN_CMPL */                     {DM_CONN_SM_ST_CONNECTED,     DM_CONN_SM_ACT_CONN_ACCEPTED},
  /* HCI_DISCONNECT_CMPL */                  {DM_CONN_SM_ST_IDLE,          DM_CONN_SM_ACT_NONE},
  /* HCI_LE_CONN_UPDATE_CMPL */              {DM_CONN_SM_ST_IDLE,          DM_CONN_SM_ACT_NONE},
  /* HCI_LE_CREATE_CONN_CANCEL_CMD_CMPL */   {DM_CONN_SM_ST_IDLE,          DM_CONN_SM_ACT_NONE}
  },
  /* Connecting state */
  {
  /* Event                                    Next state                   Action */
  /* API_OPEN */                             {DM_CONN_SM_ST_CONNECTING,    DM_CONN_SM_ACT_NONE},
  /* API_CLOSE */                            {DM_CONN_SM_ST_DISCONNECTING, DM_CONN_SM_ACT_CANCEL_OPEN},
  /* API_ACCEPT */                           {DM_CONN_SM_ST_CONNECTING,    DM_CONN_SM_ACT_NONE},
  /* HCI_LE_CONN_CMPL_FAIL */                {DM_CONN_SM_ST_IDLE,          DM_CONN_SM_ACT_CONN_FAILED},
  /* HCI_LE_CONN_CMPL */                     {DM_CONN_SM_ST_CONNECTED,     DM_CONN_SM_ACT_CONN_OPENED},
  /* HCI_DISCONNECT_CMPL */                  {DM_CONN_SM_ST_IDLE,          DM_CONN_SM_ACT_CONN_FAILED},
  /* HCI_LE_CONN_UPDATE_CMPL */              {DM_CONN_SM_ST_CONNECTING,    DM_CONN_SM_ACT_NONE},
  /* HCI_LE_CREATE_CONN_CANCEL_CMD_CMPL */   {DM_CONN_SM_ST_CONNECTING,    DM_CONN_SM_ACT_NONE}
  },
  /* Accepting state */
  {
  /* Event                                    Next state                   Action */
  /* API_OPEN */                             {DM_CONN_SM_ST_ACCEPTING,     DM_CONN_SM_ACT_NONE},
  /* API_CLOSE */                            {DM_CONN_SM_ST_IDLE,          DM_CONN_SM_ACT_CANCEL_ACCEPT},
  /* API_ACCEPT */                           {DM_CONN_SM_ST_ACCEPTING,     DM_CONN_SM_ACT_NONE},
  /* HCI_LE_CONN_CMPL_FAIL */                {DM_CONN_SM_ST_IDLE,          DM_CONN_SM_ACT_ACCEPT_FAILED},
  /* HCI_LE_CONN_CMPL */                     {DM_CONN_SM_ST_CONNECTED,     DM_CONN_SM_ACT_CONN_ACCEPTED},
  /* HCI_DISCONNECT_CMPL */                  {DM_CONN_SM_ST_IDLE,          DM_CONN_SM_ACT_ACCEPT_FAILED},
  /* HCI_LE_CONN_UPDATE_CMPL */              {DM_CONN_SM_ST_ACCEPTING,     DM_CONN_SM_ACT_NONE},
  /* HCI_LE_CREATE_CONN_CANCEL_CMD_CMPL */   {DM_CONN_SM_ST_ACCEPTING,     DM_CONN_SM_ACT_NONE}
  },
  /* Connected state */
  {
  /* Event                                    Next state                   Action */
  /* API_OPEN */                             {DM_CONN_SM_ST_CONNECTED,     DM_CONN_SM_ACT_NONE},
  /* API_CLOSE */                            {DM_CONN_SM_ST_DISCONNECTING, DM_CONN_SM_ACT_CLOSE},
  /* API_ACCEPT */                           {DM_CONN_SM_ST_CONNECTED,     DM_CONN_SM_ACT_NONE},
  /* HCI_LE_CONN_CMPL_FAIL */                {DM_CONN_SM_ST_CONNECTED,     DM_CONN_SM_ACT_NONE},
  /* HCI_LE_CONN_CMPL */                     {DM_CONN_SM_ST_CONNECTED,     DM_CONN_SM_ACT_NONE},
  /* HCI_DISCONNECT_CMPL */                  {DM_CONN_SM_ST_IDLE,          DM_CONN_SM_ACT_CONN_CLOSED},
  /* HCI_LE_CONN_UPDATE_CMPL */              {DM_CONN_SM_ST_CONNECTED,     DM_CONN_SM_ACT_HCI_UPDATED},
  /* HCI_LE_CREATE_CONN_CANCEL_CMD_CMPL */   {DM_CONN_SM_ST_CONNECTED,     DM_CONN_SM_ACT_NONE}
  },
  /* Disconnecting state */
  {
  /* Event                                    Next state                   Action */
  /* API_OPEN */                             {DM_CONN_SM_ST_DISCONNECTING, DM_CONN_SM_ACT_NONE},
  /* API_CLOSE */                            {DM_CONN_SM_ST_DISCONNECTING, DM_CONN_SM_ACT_NONE},
  /* API_ACCEPT */                           {DM_CONN_SM_ST_DISCONNECTING, DM_CONN_SM_ACT_NONE},
  /* HCI_LE_CONN_CMPL_FAIL */                {DM_CONN_SM_ST_IDLE,          DM_CONN_SM_ACT_CONN_CLOSED},
  /* HCI_LE_CONN_CMPL */                     {DM_CONN_SM_ST_DISCONNECTING, DM_CONN_SM_ACT_CLOSE},
  /* HCI_DISCONNECT_CMPL */                  {DM_CONN_SM_ST_IDLE,          DM_CONN_SM_ACT_CONN_CLOSED},
  /* HCI_LE_CONN_UPDATE_CMPL */              {DM_CONN_SM_ST_DISCONNECTING, DM_CONN_SM_ACT_NONE},
  /* HCI_LE_CREATE_CONN_CANCEL_CMD_CMPL */   {DM_CONN_SM_ST_DISCONNECTING, DM_CONN_SM_ACT_NONE}
  }
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/
extern uint8_t gu8Debug;
extern uint8_t gu8DbgCharBuf[DBG_CHAR_BUF_SIZE];
extern uint32_t gu32DbgCharBufNdx;

/*! State machine action set array */
dmConnAct_t *dmConnActSet[DM_CONN_NUM_ACT_SETS];

/*************************************************************************************************/
/*!
 *  \brief  Execute the DM connection state machine.
 *
 *  \param  pCcb  Connection control block.
 *  \param  pMsg  State machine message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void dmConnSmExecute(dmConnCcb_t *pCcb, dmConnMsg_t *pMsg)
{
  dmConnAct_t       *actSet;
  uint8_t           action;
  uint8_t           event;

  /* get the event */
  event = DM_MSG_MASK(pMsg->hdr.event);

  /* get action */
  action = dmConnStateTbl[pCcb->state][event][DM_CONN_ACTION];
  uint8_t nextSt = dmConnStateTbl[pCcb->state][event][DM_CONN_NEXT_STATE];
  uint8_t mainMstSlv = DM_CONN_ACT_SET_ID(action);
  uint8_t actSetNdx = DM_CONN_ACT_ID(action);

  // 24 DM_CONN_MSG_API_OPEN
  // 28 DM_CONN_MSG_HCI_LE_CONN_CMPL
  // 0  DM_CONN_SM_ST_IDLE

  // evt=24(0) st=0 act=16 nxtSt=1 mainMstSlv=1 actSetNdx=0 DM_CONN_SM_ACT_OPEN
  // 24 0 0 16 1 1 0

  // evt=28(4) st=1 act=2 nxtSt=3 mainMstSlv=0 actSetNdx=2
  // 28 4 1 2 3 0 2
  
  // evt=29 st=3 act=
  //@? WsfTrace("dmConnSmExecute evt=%d(%d) st=%d act=%d nxtSt=%d mainMstSlv=%d actSetNdx=%d", pMsg->hdr.event, event, pCcb->state, action, nextSt, mainMstSlv, actSetNdx);
  if (gu32DbgCharBufNdx + 60 < DBG_CHAR_BUF_SIZE)
  {
    gu32DbgCharBufNdx += sprintf((char *)&gu8DbgCharBuf[gu32DbgCharBufNdx], "@17 %d %d %d %d %d %d %d %d,\r\n",
      PalBbGetCurrentTime(), pMsg->hdr.event, event, pCcb->state, action, nextSt, mainMstSlv, actSetNdx);
  }
  if (pMsg->hdr.event == 28 && event == 4 && pCcb->state ==1 && action == 2 && nextSt ==3 && mainMstSlv == 0 && actSetNdx == 2)
  {
    gu8Debug = 18;
  }

  /* set next state */
  pCcb->state = nextSt;

  /* look up action set */
  actSet = dmConnActSet[mainMstSlv];

  /* if action set present */
  if (actSet != NULL)
  {
    /* execute action function in action set */
    (*actSet[actSetNdx])(pCcb, pMsg);
  }
  else
  {
     /* no action */
     dmConnSmActNone(pCcb, pMsg);
  }
}
