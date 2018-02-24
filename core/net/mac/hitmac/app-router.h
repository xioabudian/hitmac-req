#ifndef _APP_ROUTER_H_
#define _APP_ROUTER_H_
enum{
	PACKET_INPUT
};

struct process *conn_process; 

void hitmac_set_conn_process(struct process *p);

#endif