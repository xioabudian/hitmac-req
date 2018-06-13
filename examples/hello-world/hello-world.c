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
 */

/**
 * \file
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/netstack.h"
#include "dev/leds.h"

#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
//@test
#include "dev/serial-line.h"
#define PERIOD 1
#define SEND_INTERVAL (PERIOD*CLOCK_SECOND)
/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
char test_fun1() //no expeir
{
  // while(1){
    NETSTACK_RADIO.on();
    NETSTACK_RADIO.off();
    return 0;
  // }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Hello, world\n");
  static radio_value_t rssi=-50;
  static radio_value_t channel;
  static radio_value_t ch_num=0;
  static struct etimer periodic;
  uint8_t buf[30]={0,1,2};

  etimer_set(&periodic,SEND_INTERVAL);

  while(1){
    // test_fun1();
  	PROCESS_YIELD();
  	if(etimer_expired(&periodic)){
  		// NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI,&rssi);//get local rssi
  		// printf("RSSI %d\n",rssi);
      NETSTACK_RADIO.on();

      NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL,&channel);//get local channel
      printf("CHANNEL %d\n",channel);
      NETSTACK_RADIO.send(buf,30);

      leds_toggle(LEDS_RED);
      ch_num++;

      NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,ch_num%17);
      NETSTACK_RADIO.off();
      etimer_set(&periodic,SEND_INTERVAL);
  	}
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
