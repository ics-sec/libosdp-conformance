/*
  oo_util3 -more (3)  open osdp utility routines

  (C)Copyright 2017-2020 Smithee Solutions LLC
  (C)Copyright 2014-2017 Smithee Spelvin Agnew & Plinge, Inc.

  Support provided by the Security Industry Association
  http://www.securityindustry.org

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
    http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/


#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>


#include <osdp-tls.h>
#include <open-osdp.h>
#include <osdp_conformance.h>
#include <iec-xwrite.h>


extern OSDP_CONTEXT context;
extern OSDP_INTEROP_ASSESSMENT osdp_conformance;
extern OSDP_PARAMETERS p_card;
unsigned int web_color_lookup [16] = {
    0x000000, 0xFF0000, 0x00FF00, 0x008080,
    0x444444, 0x550101, 0x660101, 0x770101,
    0x0000FF, 0x010101, 0x010101, 0x010101,
    0x010101, 0x010101, 0x010101, 0x010101,
  };
time_t previous_time;
char tlogmsg [1024];
char tlogmsg2 [3*1024];
int mfg_rep_sequence;


int
  osdp_build_message
    (unsigned char
        *buf,
    int
      *updated_length,
    unsigned char
      command,
    int
      dest_addr,
    int
      sequence,
    int
      data_length,
    unsigned char
      *data,
    int
      secure)

{ /* osdp_build_mesage */

  int
    check_size;
  unsigned char
    *cmd_ptr;
  int
    new_length;
  unsigned char
    *next_data;
  OSDP_HDR
    *p;
  int
    status;
  int
    whole_msg_lth;


  status = ST_OK;
  if (m_check EQUALS OSDP_CHECKSUM)
    check_size = 1;
  else
    check_size = 2;
  new_length = *updated_length;

  p = (OSDP_HDR *)buf;
  p->som = C_SOM;
  new_length ++;

  // addr
  p->addr = dest_addr;
  // if we're the PD set the high order bit
  if (context.role EQUALS OSDP_ROLE_PD)
    p->addr = p->addr | 0x80;

  new_length ++;

  // length

  /*
    length goes in before CRC calc.
    length is 5 (fields to CTRL) + [if no sec] 1 for CMND + data
  */
  whole_msg_lth = 5;
  if (secure != 0)
status = -2;
  else
    whole_msg_lth = whole_msg_lth + 1; //CMND
  whole_msg_lth = whole_msg_lth + data_length;
  whole_msg_lth = whole_msg_lth + check_size; // including CRC

  p->len_lsb = 0x00ff & whole_msg_lth;
  new_length ++;
  p->len_msb = (0xff00 & whole_msg_lth) >> 8;
  new_length ++;

  // control
  p->ctrl = 0;
  p->ctrl = p->ctrl | (0x3 & sequence);

  // set CRC depending on current value of global parameter
  if (m_check EQUALS OSDP_CRC)
    p->ctrl = p->ctrl | 0x04;

  new_length ++;

  // secure is bit 3 (mask 0x08)
  if (secure)
  {
    p->ctrl = p->ctrl | 0x08;
    cmd_ptr = buf + 8; // STUB pretend sec blk is 3 bytes len len 1 payload
  }
  else
  {
    cmd_ptr = buf + 5; // skip security stuff
  };
  // hard-coded off for now
  if (secure != 0)
    status = -1;
  
  *cmd_ptr = command;
  new_length++;
  next_data = 1+cmd_ptr;

  if (data_length > 0)
  {
    int i;
    unsigned char *sptr;
    sptr = cmd_ptr + 1;
    for (i=0; i<data_length; i++)
    {
      *(sptr+i) = *(i+data);
      new_length ++;
      next_data ++; // where crc goes (after data)
    };
    if (context.verbosity > 9)
      fprintf (stderr, "data_length %d new_length now %d next_data now %lx\n",
        data_length, new_length, (unsigned long)next_data);
  };

  // crc
  if (m_check EQUALS OSDP_CRC)
{
  unsigned short int parsed_crc;
  unsigned char *crc_check;
  crc_check = next_data;
  parsed_crc = fCrcBlk (buf, new_length);

  // low order byte first
  *(crc_check+1) = (0xff00 & parsed_crc) >> 8;
  *(crc_check) = (0x00ff & parsed_crc);
  new_length ++;
  new_length ++;
}
  else
  {
    unsigned char
      cksum;
    unsigned char *
      pchecksum;

    pchecksum = next_data;
    cksum = checksum (buf, new_length);
    *pchecksum = cksum;
    new_length ++;
  };

  if (context.verbosity > 9)
  {
    fprintf (stderr, "build: sequence %d. Lth %d\n", sequence, new_length);
  }
  
  *updated_length = new_length;
  return (status);

} /* osdp_build_message */


int
  osdp_check_command_reply
    (int role,
    int command,
    OSDP_MSG *m,
    char *tlogmsg2)

{ /* osdp_check_command_reply */

  OSDP_CONTEXT *ctx;
  int status;


  ctx = &context;
  status = ST_OK;
  if ((role EQUALS OSDP_ROLE_PD) || (role EQUALS OSDP_ROLE_MONITOR))
  {
    switch (command)
    {
    default:
      if ((role != OSDP_ROLE_MONITOR) && (ctx->verbosity > 8))
        fprintf (ctx->log,
"top of osdp_check_command_reply: possible unknown command 0x%x\n",
          command);
      break;

#define OSDP_CHECK_CMDREP(osdp_tag,conformance_test, data_offset) \
      status = ST_OSDP_CMDREP_FOUND; \
      m->data_payload = m->cmd_payload + data_offset; \
      if (ctx->verbosity > 2) \
        strcpy (tlogmsg2, osdp_command_reply_to_string(command, ctx->role)); /* osdp_tag */ \
      osdp_conformance.conformance_test.test_status = OCONFORM_EXERCISED; \
      if (osdp_conformance.conforming_messages < PARAM_MMT) { \
if (ctx->verbosity>3) fprintf(stderr, "cm was %d, incrementing\n", osdp_conformance.conforming_messages); \
        osdp_conformance.conforming_messages ++;};

    case OSDP_ACURXSIZE:
      OSDP_CHECK_CMDREP ("osdp_ACURXSIZE", cmd_max_rec, 1);
      break;

    case OSDP_BUZ:
#ifndef NOSQUISH

      OSDP_CHECK_CMDREP ("osdp_BUZZ", cmd_buz, 1);
#endif
#ifdef NOSQUISH
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_BUZZ");

      osdp_test_set_status(OOC_SYMBOL_cmd_buz, OCONFORM_EXERCISED);

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
#endif
      break;

    case OSDP_CAP:
      OSDP_CHECK_CMDREP ("osdp_CAP", cmd_pdcap, 1);
      break;

    case OSDP_CHLNG:
      OSDP_CHECK_CMDREP ("osdp_zCHLNG", cmd_chlng, 1);
      break;

    case OSDP_COMSET:
      OSDP_CHECK_CMDREP ("osdp_COMSET", cmd_comset, 1);
      break;

    case OSDP_FILETRANSFER:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_FILETRANSFER");

      osdp_conformance.cmd_filetransfer.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_ID:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_ID");

      osdp_conformance.cmd_id.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_ISTAT:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_ISTAT");

      osdp_conformance.cmd_istat.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_KEEPACTIVE:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_KEEPACTIVE");
      osdp_conformance.cmd_keepactive.test_status = OCONFORM_EXERCISED;
      break;

    case OSDP_KEYSET:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      osdp_conformance.cmd_keyset.test_status = OCONFORM_EXERCISED;
      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_LED:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_LED");

//      osdp_conformance.cmd_led.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_LSTAT:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_LSTAT");

      osdp_test_set_status(OOC_SYMBOL_cmd_lstat, OCONFORM_EXERCISED);

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_MFG:
      {
        OSDP_HDR *p;

        status = ST_OSDP_CMDREP_FOUND;
        p = (OSDP_HDR *)m->ptr;
        m->data_payload = m->cmd_payload + 1;

      // if the reply bit is set it's an FTSTAT else it's an MFG

      if (0x80 & (p->addr))
      {
        if (ctx->verbosity > 2)
          strcpy (tlogmsg2, "osdp_FTSTAT");
      }
      else
      {
        if (ctx->verbosity > 2)
          strcpy (tlogmsg2, "osdp_MFG");
        osdp_conformance.cmd_mfg.test_status = OCONFORM_EXERCISED;
      };

        if (osdp_conformance.conforming_messages < PARAM_MMT)
          osdp_conformance.conforming_messages ++;
      }
      break;

    case OSDP_OSTAT:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_OSTAT");

      osdp_conformance.cmd_ostat.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_OUT:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_OUT");

      osdp_conformance.cmd_out.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_POLL:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = NULL;
      m->data_length = 0;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_POLL");

      ctx->cp_polls ++;
      // this is used for parsing so don't score conforming messages here.
      break;

    case OSDP_RSTAT:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_RSTAT");

      osdp_conformance.cmd_rstat.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_SCRYPT:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      osdp_conformance.cmd_scrypt.test_status = OCONFORM_EXERCISED;
      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_TEXT:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_TEXT");

      osdp_conformance.cmd_text.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;
    };
  };

  // if we're the CP look at the message as if it's incoming.
  // if we're in monitor mode look at it (if it's not been already seen)

  if ((status != ST_OSDP_CMDREP_FOUND) && 
    ((role EQUALS OSDP_ROLE_ACU) || (role EQUALS OSDP_ROLE_MONITOR)))
  {
    switch (command)
    {
    default:
      fprintf(ctx->log, "***possible bad response 0x%02x\n", command);
      // status = ST_OSDP_BAD_COMMAND_REPLY;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "\?\?\?");

      // if we don't recognize the command/reply code it fails 2-15-1
      osdp_conformance.CMND_REPLY.test_status = OCONFORM_FAIL;
      SET_FAIL (ctx, "2-15-1");
      break;

    case OSDP_ACK:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = NULL;
      m->data_length = 0;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_ACK");
      ctx->pd_acks ++;

      osdp_test_set_status(OOC_SYMBOL_cmd_poll, OCONFORM_EXERCISED);
      osdp_test_set_status(OOC_SYMBOL_rep_ack, OCONFORM_EXERCISED);

      // if we just got an ack for an OSTAT mark that too.
      if (ctx->last_command_sent EQUALS OSDP_OSTAT)
        osdp_conformance.cmd_ostat_ack.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_BUSY:
      OSDP_CHECK_CMDREP ("osdp_BUSY", resp_busy, 1);
      break;

    case OSDP_CCRYPT:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      strcpy (tlogmsg2, "osdp_CCRYPT");
      break;

    case OSDP_COM:
      OSDP_CHECK_CMDREP ("osdp_COM", cmd_comset, 1);
      break;

    case OSDP_FTSTAT:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      strcpy (tlogmsg2, "osdp_FTSTAT");
      break;

    case OSDP_KEYPAD:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_KEYPAD");

      osdp_conformance.resp_keypad.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_ISTATR:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_ISTATR");

      osdp_conformance.cmd_istat.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_LSTATR:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_LSTATR");

      osdp_test_set_status(OOC_SYMBOL_cmd_lstat, OCONFORM_EXERCISED);

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_MFG:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_MFG");

      osdp_conformance.cmd_mfg.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_MFGREP:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_MFGREP");

      osdp_conformance.resp_mfg.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_NAK:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_NAK");

      osdp_test_set_status(OOC_SYMBOL_rep_nak, OCONFORM_EXERCISED);

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_OSTATR:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_OSTATR");

      osdp_conformance.cmd_ostat.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_PDCAP:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_PDCAP");

//      osdp_conformance.rep_device_capas.test_status = OCONFORM_EXERCISED;
      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_PDID:
      OSDP_CHECK_CMDREP ("osdp_PDID", rep_device_ident, 1);
      break;

    case OSDP_RAW:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_RAW");

      osdp_conformance.rep_raw.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_RMAC_I:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      strcpy (tlogmsg2, "osdp_RMAC_I");
      break;

    case OSDP_RSTATR:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_RSTATR");

      osdp_conformance.cmd_rstat.test_status = OCONFORM_EXERCISED;

      if (osdp_conformance.conforming_messages < PARAM_MMT)
        osdp_conformance.conforming_messages ++;
      break;

    case OSDP_XRD:
      status = ST_OSDP_CMDREP_FOUND;
      m->data_payload = m->cmd_payload + 1;
      if (ctx->verbosity > 2)
        strcpy (tlogmsg2, "osdp_XRD");
      break;
    };
  };
  if (ctx->verbosity > 8)
  {
    if ((status != ST_OSDP_CMDREP_FOUND) && (status != ST_OK))
      fprintf (stderr,
        "bottom of osdp_check_command_reply: bad status %d\n", status);
  };
  return (status);

} /* osdp_check_command_reply */


/*
  monitor_osdp_message - output the message to the log for tracing
*/
int
  monitor_osdp_message
    (OSDP_CONTEXT *context,
    OSDP_MSG *msg)

{ /* monitor_osdp_message */

  int status;
  char tlogmsg [1024];


  status = ST_OK;
  memset(tlogmsg, 0, sizeof(tlogmsg));

  if (msg->direction EQUALS 0) // direction 0 is TO the PD
  {
    switch (msg->msg_cmd)
    {
    case OSDP_ACURXSIZE:
      status = oosdp_make_message (OOSDP_MSG_ACURXSIZE, tlogmsg, msg);
      if (status == ST_OK)
        status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
      break;
    case OSDP_CHLNG:
      status = oosdp_make_message (OOSDP_MSG_CHLNG, tlogmsg, msg);
      if (status == ST_OK)
        status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
      break;
    case OSDP_COMSET:
      status = oosdp_make_message (OOSDP_MSG_COMSET, tlogmsg, msg);
      if (status == ST_OK)
        status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
      break;
    case OSDP_KEEPACTIVE:
      status = oosdp_make_message (OOSDP_MSG_KEEPACTIVE, tlogmsg, msg);
      if (status == ST_OK)
        status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
      break;
    case OSDP_KEYSET:
      status = oosdp_make_message (OOSDP_MSG_KEYSET, tlogmsg, msg);
      if (status == ST_OK)
        status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
      break;
    case OSDP_MFG:
      status = oosdp_make_message (OOSDP_MSG_MFG, tlogmsg, msg);
      if (status == ST_OK)
        status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
      break;
    case OSDP_OUT:
      status = oosdp_make_message(OOSDP_MSG_OUT, tlogmsg, msg);
      if (status == ST_OK)
        status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
      break;
    case OSDP_SCRYPT:
      status = oosdp_make_message(OOSDP_MSG_SCRYPT, tlogmsg, msg);
      if (status == ST_OK)
        status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
      break;
    case OSDP_XWR:
      status = oosdp_make_message(OOSDP_MSG_XWRITE, tlogmsg, msg);
      if (status == ST_OK)
        status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
      break;
    };
  };
  if (msg->direction != 0) // direction non-0 (128) is FROM the PD
  {
    switch (msg->msg_cmd)
    {
    case OSDP_CCRYPT:
      status = oosdp_make_message (OOSDP_MSG_CCRYPT, tlogmsg, msg);
      if (status == ST_OK)
        status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
      break;
    case OSDP_COM:
      status = oosdp_make_message (OOSDP_MSG_COM, tlogmsg, msg);
      if (status == ST_OK)
        status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
      break;
    case OSDP_ISTATR:
      status = oosdp_make_message (OOSDP_MSG_ISTATR, tlogmsg, msg);
      if (status == ST_OK)
        status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
      break;
    case OSDP_XRD:
      status = oosdp_make_message (OOSDP_MSG_XREAD, tlogmsg, msg);
      if (status == ST_OK)
        status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
      break;
    };
  };
  switch (msg->msg_cmd)
  {
  case OSDP_BUZ:
    status = oosdp_make_message (OOSDP_MSG_BUZ, tlogmsg, msg);
    if (status == ST_OK)
      status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
    break;
  case OSDP_FILETRANSFER:
    if (context->verbosity > 3)
    {
      status = oosdp_make_message (OOSDP_MSG_FILETRANSFER, tlogmsg, msg);
      if (status == ST_OK)
        status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
    };
    break;

  case OSDP_FTSTAT:
    if (context->verbosity > 3)
    {
      status = oosdp_make_message (OOSDP_MSG_FTSTAT, tlogmsg, msg);
      if (status == ST_OK)
        status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
    };
    break;

  case OSDP_KEYPAD:
    status = oosdp_make_message (OOSDP_MSG_KEYPAD, tlogmsg, msg);
    if (status == ST_OK)
      status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
    break;

  case OSDP_LED:
    status = oosdp_make_message (OOSDP_MSG_LED, tlogmsg, msg);
    if (status == ST_OK)
      status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
    break;

  case OSDP_LSTATR:
    status = oosdp_make_message (OOSDP_MSG_LSTATR, tlogmsg, msg);
    if (status == ST_OK)
      status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
    break;

  case OSDP_MFGREP:
    status = oosdp_make_message (OOSDP_MSG_MFGREP, tlogmsg, msg);
    if (status == ST_OK)
      status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
    break;

  case OSDP_NAK:
    context->sent_naks ++; // nobody updated it in monitor mode
    status = oosdp_make_message (OOSDP_MSG_NAK, tlogmsg, msg);
    if (status == ST_OK)
      status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
    break;

  case OSDP_OSTATR:
    status = oosdp_make_message (OOSDP_MSG_OUT_STATUS, tlogmsg, msg);
    if (status EQUALS ST_OK)
      status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
    break;

  case OSDP_PDCAP:
    status = oosdp_make_message (OOSDP_MSG_PD_CAPAS, tlogmsg, msg);
    if (status == ST_OK)
      status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
    break;

  case OSDP_PDID:
    status = oosdp_make_message (OOSDP_MSG_PD_IDENT, tlogmsg, msg);
    if (status == ST_OK)
      status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
    break;

  case OSDP_RAW:
    status = oosdp_make_message(OOSDP_MSG_RAW, tlogmsg, msg);
    if (status == ST_OK)
      status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
    break;

  case OSDP_RMAC_I:
    status = oosdp_make_message(OOSDP_MSG_RMAC_I, tlogmsg, msg);
    if (status == ST_OK)
      status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
    break;

  case OSDP_TEXT:
    status = oosdp_make_message(OOSDP_MSG_TEXT, tlogmsg, msg);
    if (status == ST_OK)
      status = oosdp_log (context, OSDP_LOG_NOTIMESTAMP, 1, tlogmsg);
    break;
  };
  return (status);

} /* monitor_osdp_message */

