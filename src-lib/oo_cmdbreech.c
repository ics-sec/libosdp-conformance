/*
  oo_cmdbreech - breech-loading command processor

  (C)Copyright 2017-2018 Smithee Solutions LLC
  (C)Copyright 2015-2017 Smithee,Spelvin,Agnew & Plinge, Inc.

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


#include <string.h>


#include <jansson.h>


#include <osdp-tls.h>
#include <open-osdp.h>
#include <osdp_conformance.h>


extern OSDP_OUT_CMD
  current_output_command [];
extern OSDP_PARAMETERS
  p_card;


int
  read_command
    (OSDP_CONTEXT
      *ctx,
    OSDP_COMMAND
      *cmd)

{ /* read_command */

  FILE *cmdf;
  char current_command [1024];
  char current_options [1024];
//  char field [1024];
  int i;
  char json_string [16384];
  OSDP_MFG_ARGS *mfg_args;
  json_t *parameter;
  json_t *root;
  int set_led_temp;
  json_error_t status_json;
  int status;
  int status_io;
  char *test_command;
  char this_command [1024];
  json_t *value;
  char vstr [1024];


  status = ST_CMD_PATH;
  cmdf = fopen (ctx->command_path, "r");
  if (cmdf != NULL)
  {
    status = ST_OK;
    memset (json_string, 0, sizeof (json_string));
    status_io = fread (json_string,
      sizeof (json_string [0]), sizeof (json_string), cmdf);
    if (status_io >= sizeof (json_string))
      status = ST_CMD_OVERFLOW;
    if (status_io <= 0)
      status = ST_CMD_UNDERFLOW;
  };
  if (status EQUALS ST_OK)
  {
    root = json_loads (json_string, 0, &status_json);
    if (!root)
    {
      fprintf (stderr, "JSON parser failed.  String was ->\n%s<-\n",
        json_string);
      status = ST_CMD_ERROR;
    };
  };
  if (status EQUALS ST_OK)
  {
    value = json_object_get (root, "command");
    strcpy (current_command, json_string_value (value));
    if (!json_is_string (value))
      status = ST_CMD_INVALID;
    if (ctx->verbosity > 3)
      fprintf (stderr, "command was %s\n", current_command);
  };

  // command bio_read

  if (status EQUALS ST_OK)
  {
    if (0 EQUALS strcmp (current_command, "bio_read"))
    {
      cmd->command = OSDP_CMD_NOOP; // nothing other than what's here so no-op

      status = send_bio_read_template (ctx);
    };
  };

  // command busy

  if (status EQUALS ST_OK)
  {
    if (0 EQUALS strcmp (current_command, "busy"))
    {
      cmd->command = OSDP_CMDB_BUSY;
    };
  };

  // command buzz

  if (status EQUALS ST_OK)
  {
    if (0 EQUALS strcmp (current_command, "buzz"))
    {
      cmd->command = OSDP_CMDB_BUZZ;

      // set default on, off, repeat timers

      cmd->details [0] = 15;
      cmd->details [1] = 15;
      cmd->details [2] = 3;

      // also use off_time if it's present

      parameter = json_object_get (root, "off_time");
      if (json_is_string (parameter))
      {
        strcpy (vstr, json_string_value (parameter));
        sscanf (vstr, "%d", &i);
        cmd->details [1] = i;
      };

      // also use on_time if it's present

      parameter = json_object_get (root, "on_time");
      if (json_is_string (parameter))
      {
        strcpy (vstr, json_string_value (parameter));
        sscanf (vstr, "%d", &i);
        cmd->details [0] = i;
      };

      // also use repeat if it's present

      parameter = json_object_get (root, "repeat");
      if (json_is_string (parameter))
      {
        strcpy (vstr, json_string_value (parameter));
        sscanf (vstr, "%d", &i);
        cmd->details [2] = i;
      };
    };
  }; 

  // command capabilities

  if (status EQUALS ST_OK)
  {
    if (0 EQUALS strcmp (current_command, "capabilities"))
    {
      cmd->command = OSDP_CMDB_CAPAS;
    };
  }; 

  // COMSET.  takes two option arguments, "new_address" and "new_speed".
  // default for new_address is 0x00, default for new_speed is 9600

  if (status EQUALS ST_OK)
  {

    if (0 EQUALS strcmp (current_command, "comset"))
    {
      cmd->command = OSDP_CMDB_COMSET;

      value = json_object_get (root, "new_address");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        cmd->details [0] = i;
      };
      value = json_object_get (root, "new_speed");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        *(int *) &(cmd->details [4]) = i; // by convention bytes 4,5,6,7 are the speed.
      };
      if (ctx->verbosity > 2)
        fprintf (stderr, "Command COMSET Address %d Speed %d\n",
          (int) (cmd->details [0]),
          *(int *) &(cmd->details [4]));
    };
  }; 

  // command conform_2_2_1

  if (status EQUALS ST_OK) {
    if (0 EQUALS strcmp (current_command, "conform_2_2_1")) {
      cmd->command = OSDP_CMDB_CONFORM_2_2_1; }; };

  // command conform_2_2_2

  if (status EQUALS ST_OK) {
    if (0 EQUALS strcmp (current_command, "conform_2_2_2")) {
      cmd->command = OSDP_CMDB_CONFORM_2_2_2; }; };

  // command conform_2_2_3

  if (status EQUALS ST_OK) {
    if (0 EQUALS strcmp (current_command, "conform_2_2_3")) {
      cmd->command = OSDP_CMDB_CONFORM_2_2_3; }; };

  // command conform_2_2_4

  if (status EQUALS ST_OK) {
    if (0 EQUALS strcmp (current_command, "conform_2_2_4")) {
      cmd->command = OSDP_CMDB_CONFORM_2_2_4; }; };

  // command conform_2_6_1

  if (status EQUALS ST_OK)
  {
    if (0 EQUALS strcmp (current_command, "conform_2_6_1"))
    {
      cmd->command = OSDP_CMDB_CONFORM_2_6_1;
      strcpy (ctx->text,
" ***OSDP CONFORMANCE TEST*** 45678901234567890123456789012345678901234567890123456789012345678901234567890");
    };
  };

  // command conform_2_11_3 - send an ID on the all-stations PD address

  if (status EQUALS ST_OK)
  {
    if (0 EQUALS strcmp (current_command, "conform_2_11_3"))
    {
      cmd->command = OSDP_CMDB_CONFORM_2_11_3;
    };
  };


  // command conform_3_14_2 - corrupted COMSET

  if (status EQUALS ST_OK)
  {
    if (0 EQUALS strcmp (current_command, "conform_3_14_2"))
    {
      cmd->command = OSDP_CMD_NOOP; // nothing other than what's here so no-op

      status = send_comset (ctx, p_card.addr, 0, "999999");
    };
  };

  // 2-14-3: rogue secure poll

  if (status EQUALS ST_OK) {
    if (0 EQUALS strcmp (current_command, "conform_2_14_3")) {
      cmd->command = OSDP_CMDB_CONFORM_2_14_3; }; };

  // command conform_3_20_1 - MFG

  if (status EQUALS ST_OK) {
    if (0 EQUALS strcmp (current_command, "conform_3_20_1")) {
      cmd->command = OSDP_CMDB_CONFORM_3_20_1; }; };

  // command induce-NAK

  if (status EQUALS ST_OK) {
    if (0 EQUALS strcmp (current_command, "induce-NAK")) {
      cmd->command = OSDP_CMDB_INDUCE_NAK; }; };

  // command MFG.  Arguments are OUI, command-id, command-specific-data.
  // c-s-d is is 2-hexit bytes, length inferred.

  if (status EQUALS ST_OK) {
    if (0 EQUALS strcmp(current_command, "mfg")) {
      cmd->command = OSDP_CMDB_MFG; 
      mfg_args = (OSDP_MFG_ARGS *)(cmd->details);
      memset(mfg_args, 0, sizeof (*mfg_args));
      parameter = json_object_get(root, "command-id");
      if (json_is_string(parameter))
      {
        int i;
        sscanf(json_string_value(parameter), "%d", &i);
        mfg_args->command_ID = i;
      };
      parameter = json_object_get(root, "command-specific-data");
      if (json_is_string(parameter))
      {
        strcpy(mfg_args->c_s_d, json_string_value(parameter));
      };
      parameter = json_object_get(root, "oui");
      if (json_is_string(parameter))
      {
        strcpy(mfg_args->oui, json_string_value(parameter));
      };
      parameter = json_object_get(root, "payload");
      if (json_is_string (parameter))
      {
        strcpy((char *)cmd->details, json_string_value (parameter));
      };
    };
  };

  // command text

  if (status EQUALS ST_OK)
  {
    if (0 EQUALS strcmp (current_command, "text"))
    {
      char
        field [1024];
      json_t
        *value;
      strcpy (field, "message");
      value = json_object_get (root, field);
      if (json_is_string (value))
      {
        strcpy (ctx->text, json_string_value (value));
        cmd->command = OSDP_CMDB_TEXT;
      };
    };
  };

  // command transfer

  if (status EQUALS ST_OK) {
    if (0 EQUALS strcmp (current_command, "transfer")) {
      cmd->command = OSDP_CMDB_TRANSFER; }; };

  // if there's a "file" argument use that

  parameter = json_object_get (root, "file");
  if (json_is_string (parameter))
  {
    strcpy ((char *)cmd->details, json_string_value (parameter));
  };

  if (status EQUALS ST_OK)
  {
    strcpy (this_command, json_string_value (value));
    test_command = "dump_status";
    if (0 EQUALS strncmp (this_command, test_command, strlen (test_command)))
    {
      cmd->command = OSDP_CMDB_DUMP_STATUS;
      if (ctx->verbosity > 3)
        fprintf (stderr, "command was %s\n",
          this_command);
    };
  }; 
  if (status EQUALS ST_OK)
  {
    strcpy (this_command, json_string_value (value));
    test_command = "identify";
    if (0 EQUALS strncmp (this_command, test_command, strlen (test_command)))
    {
      cmd->command = OSDP_CMDB_IDENT;
      if (ctx->verbosity > 3)
        fprintf (stderr, "command was %s\n",
          this_command);
    };
  }; 

  // initiate secure channel

  if (status EQUALS ST_OK)
  {
    strcpy (this_command, json_string_value (value));
    test_command = "initiate-secure-channel";
    if (0 EQUALS strncmp (this_command, test_command, strlen (test_command)))
    {
      cmd->command = OSDP_CMDB_INIT_SECURE;
      if (ctx->verbosity > 3)
        fprintf (stderr, "command was %s\n",
          this_command);
    };
  }; 

  // request input status

  if (status EQUALS ST_OK)
  {
    strcpy (this_command, json_string_value (value));
    test_command = "input_status";
    if (0 EQUALS strncmp (this_command, test_command, strlen (test_command)))
    {
      cmd->command = OSDP_CMDB_ISTAT;
      if (ctx->verbosity > 3)
        fprintf (stderr, "command was %s\n",
          this_command);
    };
  }; 

  // command "keypad" - send keyboard input

  if (status EQUALS ST_OK)
  {
    if (0 EQUALS strcmp (current_command, "keypad"))
    {
      cmd->command = OSDP_CMDB_KEYPAD;

      value = json_object_get (root, "digits");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        if (strlen (vstr) > 9)
        {
          fprintf (stderr, "Too many digits in keypad input, truncating to first 9\n");
          vstr [9] = 0;
        };
        memcpy (cmd->details, vstr, 9);
      };
    };
  };

  // command "local_status" - request local status

  if (status EQUALS ST_OK)
  {
    strcpy (this_command, json_string_value (value));
    test_command = "local_status";
    if (0 EQUALS strncmp (this_command, test_command, strlen (test_command)))
    {
      cmd->command = OSDP_CMDB_LSTAT;
      if (ctx->verbosity > 3)
        fprintf (stderr, "command was %s\n",
          this_command);
    };
  }; 

  // command "led"

  if (status EQUALS ST_OK)
  {
    if (0 EQUALS strcmp (current_command, "led"))
    {
      OSDP_RDR_LED_CTL *led_ctl;

      cmd->command = OSDP_CMDB_LED;
      /*
        "details" is an OSDP_RDR_LED_CTL structure.
        set up details with the default values (and then tune that if there
        are parameters with the command.)
      */
      led_ctl = (OSDP_RDR_LED_CTL *)(cmd->details);
      set_led_temp = 0;
      led_ctl->led           = 0;
      led_ctl->perm_control  = OSDP_LED_SET;
      led_ctl->perm_off_time = 0;
      led_ctl->perm_off_color = OSDP_LEDCOLOR_BLACK;
      led_ctl->perm_on_color  = OSDP_LEDCOLOR_GREEN;
      led_ctl->perm_on_time   = 30;
      led_ctl->reader         = 0;
      led_ctl->temp_control   = OSDP_LED_TEMP_NOP;
      led_ctl->temp_off       = 3;
      led_ctl->temp_off_color = OSDP_LEDCOLOR_GREEN;
      led_ctl->temp_on        = 3;
      led_ctl->temp_on_color  = OSDP_LEDCOLOR_RED;
      led_ctl->temp_timer_lsb = 30;
      led_ctl->temp_timer_msb = 0;

      if (ctx->verbosity > 3)
        fprintf (stderr, "command was %s\n",
          this_command);

      value = json_object_get (root, "led_number");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        led_ctl->led = i;
      };
      value = json_object_get (root, "perm_control");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        led_ctl->perm_control = i;
      };
      value = json_object_get (root, "perm-off-time");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        led_ctl->perm_off_time = i;
      };
      value = json_object_get (root, "perm_off_color");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        led_ctl->perm_off_color = i;
      };
      value = json_object_get (root, "perm-on-time");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        led_ctl->perm_on_time = i;
      };
      value = json_object_get (root, "perm_on_color");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        led_ctl->perm_on_color = i;
      };
      value = json_object_get (root, "temp_off_color");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        led_ctl->temp_off_color = i;
        if (i > 0)
          set_led_temp = 1;
      };
      value = json_object_get (root, "temp_off");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        led_ctl->temp_off = i;
        if (i > 0)
          set_led_temp = 1;
      };
      value = json_object_get (root, "temp_on");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        led_ctl->temp_on = i;
        if (i > 0)
          set_led_temp = 1;
      };
      value = json_object_get (root, "temp_on_color");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        led_ctl->temp_on_color = i;
        if (i > 0)
          set_led_temp = 1;
      };
      value = json_object_get (root, "temp_timer");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        led_ctl->temp_timer_lsb = i & 0xff;
        led_ctl->temp_timer_msb = i >> 8;
        if (i > 0)
          set_led_temp = 1;
      };

      // lastly look for "temp-control".  If it's set it overrides the
      // implicit temp-control from other values being set.

      value = json_object_get (root, "temp_control");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        set_led_temp = 0; // NOT the implied control
        led_ctl->temp_control = i;
      };
      if (set_led_temp)
      {
        led_ctl->temp_control = OSDP_LED_TEMP_SET;
      };
    };
  }; 
  if (status EQUALS ST_OK)
  {
    if (0 EQUALS strcmp (current_command, "operator_confirm"))
    {
      cmd->command = OSDP_CMD_NOOP; // nothing other than what's here so no-op
      value = json_object_get (root, "test");
      if (json_is_string (value))
      {
        strcpy (current_options, json_string_value (value));
        status = osdp_conform_confirm (current_options);
      };
    };
  };

  // output (digital bits out)

  if (status EQUALS ST_OK)
  {
    int
      i;
    char
      vstr [1024];

    test_command = "output";
    if (0 EQUALS strcmp (current_command, test_command))
    {
      cmd->command = OSDP_CMDB_OUT;
      if (ctx->verbosity > 3)
        fprintf (stderr, "command was %s\n",
          this_command);

      // default values in case some are missing

      current_output_command [0].output_number = 0;
      current_output_command [0].control_code = 2; // permanent on immediate
      current_output_command [0].timer = 0; // forever

      // the output command takes arguments: output_number, control_code

      value = json_object_get (root, "output-number");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        current_output_command [0].output_number = i;
      };
      value = json_object_get (root, "control-code");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        current_output_command [0].control_code = i;
      };
      value = json_object_get (root, "timer");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        current_output_command [0].timer = i;
      };
    };
  }; 

  // request output status

  if (status EQUALS ST_OK)
  {
    if (0 EQUALS strcmp (current_command, "output_status"))
    {
      cmd->command = OSDP_CMDB_OSTAT;
      if (ctx->verbosity > 3)
        fprintf (stderr, "command was %s\n",
          this_command);
    };
  }; 

  if (status EQUALS ST_OK)
  {
    value = json_object_get (root, "command");

    strcpy (this_command, json_string_value (value));
    test_command = "present_card";
    if (0 EQUALS strncmp (this_command, test_command, strlen (test_command)))
    {
      cmd->command = OSDP_CMDB_PRESENT_CARD;
      if (ctx->verbosity > 3)
        fprintf (stderr, "command was %s\n",
          this_command);
    };
  }; 

  // request (attached) reader status

  if (status EQUALS ST_OK)
  {
    strcpy (this_command, json_string_value (value));
    test_command = "reader_status";
    if (0 EQUALS strncmp (this_command, test_command, strlen (test_command)))
    {
      cmd->command = OSDP_CMDB_RSTAT;
      if (ctx->verbosity > 3)
        fprintf (stderr, "command was %s\n",
          this_command);
    };
  }; 

  if (status EQUALS ST_OK)
  {
    strcpy (this_command, json_string_value (value));
    test_command = "reset_power";
    if (0 EQUALS strncmp (this_command, test_command, strlen (test_command)))
    {
      cmd->command = OSDP_CMDB_RESET_POWER;
      if (ctx->verbosity > 3)
        fprintf (stderr, "command was %s\n",
          this_command);
    };
  }; 
  if (status EQUALS ST_OK)
  {
    strcpy (this_command, json_string_value (value));
    test_command = "send_poll";
    if (0 EQUALS strncmp (this_command, test_command, strlen (test_command)))
    {
      cmd->command = OSDP_CMDB_SEND_POLL;
      if (ctx->verbosity > 3)
        fprintf (stderr, "command was %s\n",
          this_command);
    };
  }; 
  if (status EQUALS ST_OK)
  {
    strcpy (this_command, json_string_value (value));
    test_command = "tamper";
    if (0 EQUALS strncmp (this_command, test_command, strlen (test_command)))
    {
      cmd->command = OSDP_CMDB_TAMPER;
      if (ctx->verbosity > 3)
        fprintf (stderr, "command was %s\n",
          this_command);
    };
  }; 


  // command verbosity
  // arg level - range 0-9

  if (status EQUALS ST_OK)
  {
    if (0 EQUALS strcmp (current_command, "verbosity"))
    {
      int
        i;
      char
        vstr [1024];

      cmd->command = OSDP_CMD_NOOP; // nothing other than what's here so no-op
      if (ctx->verbosity > 3)
        fprintf (stderr, "command was %s\n",
          this_command);

      value = json_object_get (root, "level");
      if (json_is_string (value))
      {
        strcpy (vstr, json_string_value (value));
        sscanf (vstr, "%d", &i);
        ctx->verbosity = i;
      };
    };
  }; 

  if (cmdf != NULL)
    fclose (cmdf);
  if (status != ST_OK)
    fprintf(stderr, "Status %d at read_command.\n", status);
  return (status);

} /* read_command */

