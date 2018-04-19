#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "dev/interface.h"
#include <stdio.h>
#include <string.h>

PROCESS(radio_receiver_process,"radio receiver process");
AUTOSTART_PROCESSES(&radio_receiver_process);
PROCESS_THREAD(radio_receiver_process,ev,data)
{
	
	PROCESS_BEGIN();
	printf("%s\n","Startting receiver");
	NETSTACK_RADIO.on();
	
	PROCESS_END();
}