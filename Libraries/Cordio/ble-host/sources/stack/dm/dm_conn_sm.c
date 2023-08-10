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

#include <stdio.h>
#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "dm_api.h"
#include "dm_main.h"
#include "dm_conn.h"

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

/*! State machine action set array */
dmConnAct_t *dmConnActSet[DM_CONN_NUM_ACT_SETS];

char *GetDmEvtStr(uint8_t evt)
{
  switch(evt) {
    case 20: return "CCC_STATE";
    case 21: return "DB_HASH_CALC_CMPL";    // ATTS_DB_HASH_CALC_CMPL_IND
    case 22: return "MTU_UPDATE";
    case 28: return "CONN_CMPL"; // DM_CONN_MSG_HCI_LE_CONN_CMPL
    case 29: return "HCI_DISCONNECT_CMPL";    // 
    case 30: return "CONN_UPDATE_CMPL";  // DM_CONN_MSG_HCI_LE_CONN_UPDATE_CMPL
    case 32: return "RESET_CMPL";           // DM_RESET_CMPL_IND
    case 33: return "ADV_START";            // DM_ADV_START_IND
    case 39: return "CONN_OPEN";
    case 40: return "CONN_CLOSE";           // DM_CONN_CLOSE_IND
    case 41: return "CONN_UPDATE";
    case 42: return "PAIR_CMPL";            // DM_SEC_PAIR_CMPL_IND
    case 44: return "SEC_ENCRYPT";
    case 46: return "AUTH_REQ";
    case 47: return "SEC_KEY";              // DM_SEC_KEY_IND
    case 48: return "LTK_REQ";
    case 49: return "SEC_PAIR";
    case 58: return "ADD_DEV_TO_RES_LIST";  // DM_PRIV_ADD_DEV_TO_RES_LIST_IND
    case 63: return "ADDR_RES_ENABLE";      // DM_PRIV_SET_ADDR_RES_ENABLE_IND
    case 65: return "DATA_LEN_CHANGE";
    case 70: return "PHY_UPDATE";
    case 87: return "FEAT";   // DM_REMOTE_FEATURES_IND
    case 113: return "UPDATE_SLAVE";  // DM_CONN_MSG_API_UPDATE_SLAVE
    case 153: return "TRIM_TIMER";          // TRIM_TIMER_EVT
    case 154: return "CUST_SPEC_TMR";       // CUST_SPEC_TMR_EVT
    case 155: return "CGM_MEAS_TMR";        // CGM_MEAS_TMR_EVT
    default: return " ";
  }
}


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
  uint8_t           old_st = pCcb->state;

  DM_TRACE_INFO3("dmConnSmExecute evt=%d %s st=%d", pMsg->hdr.event, GetDmEvtStr(pMsg->hdr.event), old_st);  // search "DM conn event handler messages" and "DM_CONN_SM_"

  /* get the event */
  event = DM_MSG_MASK(pMsg->hdr.event);

  /* get action */
  action = dmConnStateTbl[old_st][event][DM_CONN_ACTION];

  /* set next state */
  pCcb->state = dmConnStateTbl[pCcb->state][event][DM_CONN_NEXT_STATE];

  /* look up action set */
  actSet = dmConnActSet[DM_CONN_ACT_SET_ID(action)];

  /* if action set present */
  if (actSet != NULL)
  {
    /* execute action function in action set */
    (*actSet[DM_CONN_ACT_ID(action)])(pCcb, pMsg);
  }
  else
  {
     /* no action */
     dmConnSmActNone(pCcb, pMsg);
  }
}
