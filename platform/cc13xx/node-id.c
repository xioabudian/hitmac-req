/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * $Id: node-id.c,v 1.2 2010/08/26 22:08:11 nifi Exp $
 */

/**
 * \file
 *         Utility to store a node id in the infomem A
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Fredrik Osterlind <fredrik@thingsquare.com>
 */

#include "node-id.h"
#include <stdio.h>
#include "flashdata.h"

/*define relative address for store information*/
#define RESTART_COUNT_ADDR SECTOR_TO_ERASE_START_ADDR
#define RESTART_COUNT_ADDR_LEN  2

#define NORMALBYTE_RFCHANNEL_ADDR (SECTOR_TO_ERASE_START_ADDR + RESTART_COUNT_ADDR_LEN)
#define NORMALBYTE_RFCHANNEL_ADDR_LEN 3

#define CMD_ADDR (NORMALBYTE_RFCHANNEL_ADDR + NORMALBYTE_RFCHANNEL_ADDR_LEN)
#define CMD_ADDR_LEN 30

/*end define relative address*/

#define DEBUG 1
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

unsigned short node_id = 0;

/*---------------------------------------------------------------------------*/
void
node_id_restore(void)
{
}
/*---------------------------------------------------------------------------*/
void
node_id_burn(unsigned short id)
{

  node_id = id;
}
/*---------------------------------------------------------------------------*/
/*
 *  for normal btye
 *
 */

void
normalbyte_rfchannel_burn(uint8_t normalbyte, uint8_t rfchannel)
{
  uint8_t rf_data[NORMALBYTE_RFCHANNEL_ADDR_LEN];
  PRINTF("write normalbyte-- %x to flash\n", normalbyte);
  PRINTF("write channel byte-- %x to flash\n", rfchannel);
  rf_data[0] = normalbyte;
  rf_data[1] = rfchannel;
  rf_data[2] = 0xff;
  write_to_flash(rf_data, NORMALBYTE_RFCHANNEL_ADDR, NORMALBYTE_RFCHANNEL_ADDR_LEN);
}
/*---------------------------------------------------------------------------*/
void
normalbyte_rfchannel_restore(void)
{
  uint8_t normalbyte_restore[2];
  read_flash(NORMALBYTE_RFCHANNEL_ADDR, normalbyte_restore, 2);
  normal_byte = normalbyte_restore[0];
  channel_byte = normalbyte_restore[1];
  printf("normalbyte restore :%x\n", normal_byte);
  printf("channel_byte restore %x\n", channel_byte);
}
/*
 *  for meter reading data command burn
 *
 */

/*---------------------------------------------------------------------------*/
void
restore_meter_cmd(void)
{
  read_flash(CMD_ADDR, cmd_read_meter, CMD_ADDR_LEN);
}
void
print_cmd_array(void)
{
  uint8_t length = cmd_read_meter[0];
  for(int i = 0; i < length; i++) {

    PRINTF("%02x,", cmd_read_meter[i + 1]);
  }
  PRINTF("\n");
}
/*---------------------------------------------------------------------------*/
int
cmd_bytes_burn()
{
  uint8_t cmd_data[CMD_ADDR_LEN] = { 0x05, 0x10, 0x5B, 0xFE, 0x59, 0x16 };
  PRINTF("write cmd_meter_data,len %d\n", CMD_ADDR_LEN);
  write_to_flash(cmd_data, CMD_ADDR, CMD_ADDR_LEN);
  return 0;
}
/*---------------------------------------------------------------------------*/
/*
 *
 *   for channel
 */
void
restart_count_byte_burn(unsigned short val)
{
  uint8_t restart_data[RESTART_COUNT_ADDR_LEN];
  restart_data[0] = val & 0xff;
  /* restart_data[1] = val >> 8; */
  restart_data[1] = 0;
  PRINTF("write restartcount byte-- %x to flash\n", val);
  write_to_flash(restart_data, RESTART_COUNT_ADDR, RESTART_COUNT_ADDR_LEN);
}
/*---------------------------------------------------------------------------*/
void
restart_count_byte_restore(void)
{

  uint8_t read_data[RESTART_COUNT_ADDR_LEN];
  read_flash(RESTART_COUNT_ADDR, read_data, RESTART_COUNT_ADDR_LEN);

  /* restart_count=read_data[1]<<8|read_data[0]; */
  restart_count = read_data[0];
  PRINTF("restart count restore %x\n", restart_count);
}
/*---------------------------------------------------------------------------*/