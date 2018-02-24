#ifndef __HITMAC_FRAME_H__
#define __HITMAC_FRAME_H__
#include "contiki.h"
#include "contiki-conf.h"
#include "net/packetbuf.h"
#include "net/mac/tsch/tsch-private.h"
#include "net/mac/frame802154.h"
#include "net/mac/frame802154e-ie.h"
#include "net/mac/hitmac/hitmac.h"
#include "net/mac/mac.h"


#define FRAME802154_REQUEST_ASSOCIATE_CMDID  0x01
#define FRAME802154_RESPONSE_ASSOCIATE_CMDID  0x02
#define FRAME802154_UNKNOW_CMDID  0xFF

/*---------------------------------------------------------------------------*/
/* Create an EB packet */
int hitmac_packet_create_eb(uint8_t *buf, int buf_size,struct hitmac_asn_t asn);
/* Parse an EB packet */
int hitmac_packet_parse_eb(const uint8_t *buf, int buf_size,
                     frame802154_t *frame, struct ieee802154_ies *ies);
/* Create an REQUEST packet */
int hitmac_packet_create_request_associate(uint8_t *buf, int buf_size, uint16_t addr);

/* Parse a IEEE 802.15.4e REQUEST CMD  */
int hitmac_packet_parse_cmd(const uint8_t *buf, int buf_size,
                     frame802154_t *frame, uint8_t *cmdtype);
char hitmac_packet_is_broadcast(uint8_t *addr);

uint32_t get_asn_mod_val(struct hitmac_asn_t asn,uint32_t MOD);
/* Add packet to queue.*/
int hitmac_queue_add_packet(mac_callback_t sent, void *ptr);
/* Remove packet from queue */
int hitmac_queue_remove_packet();
/* Get packet from queue */
struct  hitmac_packet * hitmac_queue_get_packet();
/*Create a data frame*/
int hitmac_packet_create_dataframe(uint8_t *buf, int buf_size,uint16_t addr);

void hitmac_queue_init();

void hitmac_queue_reset();

#endif