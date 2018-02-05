#include "hitmac-frame.h"

#include <stdio.h>
#if 1
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]", (lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3], (lladdr)->addr[4], (lladdr)->addr[5])
#else 
#define PRINTF(...)
#define PRINTLLADDR(...)
#endif 

/*---------------------------------------------------------------------------*/
/* MLME sub-IE. HITMAC synchronization. Used in EBs: ASN */
int
frame80215e_create_ie_hitmac_synchronization(uint8_t *buf, int len,
    struct ieee802154_ies *ies)
{
  int ie_len = 5;
  if(len >= ie_len && ies != NULL) {
    buf[0] = ies->ie_asn.ls4b;
    buf[1] = ies->ie_asn.ls4b >> 8;
    buf[2] = ies->ie_asn.ls4b >> 16;
    buf[3] = ies->ie_asn.ls4b >> 24;
    buf[4] = ies->ie_asn.ms1b;
    // buf[5] = ies->ie_join_priority;
    return ie_len;
  } else {
    PRINTF("frame802154e: failed to parse ie\n");
    return -1;
  }
}
/*---------------------------------------------------------------------------*/
/* Create an EB packet */
int
hitmac_packet_create_eb(uint8_t *buf, int buf_size,struct hitmac_asn_t asn)
{
  int ret = 0;
  uint8_t curr_len = 0;

  frame802154_t p;
  struct ieee802154_ies ies;

  if(buf_size < HITMAC_PACKET_MAX_LEN) {
    return 0;
  }

  /* Create 802.15.4 header */
  memset(&p, 0, sizeof(p));
  p.fcf.frame_type = FRAME802154_BEACONFRAME;
  p.fcf.ie_list_present = 1;
  p.fcf.frame_version = FRAME802154_IEEE802154E_2012;
  p.fcf.src_addr_mode = LINKADDR_SIZE > 2 ? FRAME802154_LONGADDRMODE : FRAME802154_SHORTADDRMODE;
  p.fcf.dest_addr_mode = FRAME802154_SHORTADDRMODE;
  p.fcf.sequence_number_suppression = 1;
  /* It is important not to compress PAN ID, as this would result in not including either
   * source nor destination PAN ID, leaving potential joining devices unaware of the PAN ID. */
  p.fcf.panid_compression = 0;

  p.src_pid = frame802154_get_pan_id();
  p.dest_pid = frame802154_get_pan_id();
  linkaddr_copy((linkaddr_t *)&p.src_addr, &linkaddr_node_addr);
  p.dest_addr[0] = 0xff;
  p.dest_addr[1] = 0xff;

  if((curr_len = frame802154_create(&p, buf)) == 0) {
    return 0;
  }

  /* Prepare Information Elements for inclusion in the EB */
  memset(&ies, 0, sizeof(ies));

  ies.ie_asn.ls4b =  asn.ls4b;
  ies.ie_asn.ms1b =  asn.ms1b;

  if((ret = frame80215e_create_ie_hitmac_synchronization(buf + curr_len, buf_size - curr_len, &ies)) == -1) {
    return -1;
  }
  curr_len += ret;

  return curr_len;
}
/*---------------------------------------------------------------------------*/
/*MAC CMD operation*/
int
frame80215e_create_cmd_hitmac(uint8_t *buf, int len, uint8_t cmd_type)
{
  if(cmd_type == FRAME802154_UNKNOW_CMDID){
    PRINTF("frame802154e: unknow cmd type\n");
    return -1;
  }

  if(cmd_type == FRAME802154_REQUEST_ASSOCIATE_CMDID && len >= 1){

    buf[0] = FRAME802154_REQUEST_ASSOCIATE_CMDID;
    return 1;
  }else{
    return -1;
  }
}
/*---------------------------------------------------------------------------*/
/*Create a request associate*/
int
hitmac_packet_create_request_associate(uint8_t *buf, int buf_size, uint16_t addr)
{
  int ret = 0;
  uint8_t curr_len = 0;

  frame802154_t p;

  if(buf_size < HITMAC_PACKET_MAX_LEN) {
    return 0;
  }

  /* Create 802.15.4 header */
  memset(&p, 0, sizeof(p));
  p.fcf.frame_type = FRAME802154_CMDFRAME;
 
  p.fcf.frame_version = FRAME802154_IEEE802154E_2012;
  p.fcf.src_addr_mode = LINKADDR_SIZE > 2 ? FRAME802154_LONGADDRMODE : FRAME802154_SHORTADDRMODE;
  p.fcf.dest_addr_mode = FRAME802154_SHORTADDRMODE;
  p.fcf.sequence_number_suppression = 1;

  p.src_pid = frame802154_get_pan_id();
  p.dest_pid = frame802154_get_pan_id();


  linkaddr_copy((linkaddr_t *)&p.src_addr, &linkaddr_node_addr);
  p.dest_addr[0] = addr & 0xff;
  p.dest_addr[1] = (addr>>8) & 0xff;

  if((curr_len = frame802154_create(&p, buf)) == 0) {
    return 0;
  }
  
  if((ret = frame80215e_create_cmd_hitmac(buf + curr_len, buf_size - curr_len, 
    FRAME802154_REQUEST_ASSOCIATE_CMDID)) == -1){
    return -1;
  }
  curr_len += ret;

  return curr_len;

}
/*---------------------------------------------------------------------------*/
/* Parse all IEEE 802.15.4e Information Elements (IE) from a frame */
int
hitmac_parse_information_elements(const uint8_t *buf, uint8_t buf_size,
    struct ieee802154_ies *ies)
{
	uint16_t len = buf_size;
	if(ies == NULL) {
		return -1;
	}
	if(len ==  5) {
		if(ies != NULL) {
		  ies->ie_asn.ls4b = (uint32_t)buf[0];
		  ies->ie_asn.ls4b |= (uint32_t)buf[1] << 8;
		  ies->ie_asn.ls4b |= (uint32_t)buf[2] << 16;
		  ies->ie_asn.ls4b |= (uint32_t)buf[3] << 24;
		  ies->ie_asn.ms1b = (uint8_t)buf[4];
		  // ies->ie_join_priority = (uint8_t)buf[5];
		}
	}

	return len;
}
/*---------------------------------------------------------------------------*/
/* Parse a IEEE 802.15.4e TSCH Enhanced Beacon (EB) */
int
hitmac_packet_parse_eb(const uint8_t *buf, int buf_size,
                     frame802154_t *frame, struct ieee802154_ies *ies)
{
  uint8_t curr_len = 0;
  int ret;

  if(frame == NULL || buf_size < 0) {
    return 0;
  }

  /* Parse 802.15.4-2006 frame, i.e. all fields before Information Elements */
  if((ret = frame802154_parse((uint8_t *)buf, buf_size, frame)) == 0) {
    PRINTF("HITMAC:! parse_eb: failed to parse frame\n");
    return 0;
  }

  if(frame->fcf.frame_version < FRAME802154_IEEE802154E_2012
     || frame->fcf.frame_type != FRAME802154_BEACONFRAME) {
    PRINTF("HITMAC:! parse_eb: frame is not a valid TSCH beacon. Frame version %u, type %u, FCF %02x %02x\n",
           frame->fcf.frame_version, frame->fcf.frame_type, buf[0], buf[1]);
    PRINTF("HITMAC:! parse_eb: frame was from 0x%x/", frame->src_pid);
    PRINTLLADDR((const uip_lladdr_t *)&frame->src_addr);
    PRINTF(" to 0x%x/", frame->dest_pid);
    PRINTLLADDR((const uip_lladdr_t *)&frame->dest_addr);
    PRINTF("\n");
    return 0;
  }

  curr_len += ret;

  if(ies != NULL) {
    memset(ies, 0, sizeof(struct ieee802154_ies));
    ies->ie_join_priority = 0xff; /* Use max value in case the Beacon does not include a join priority */
  }
  if(frame->fcf.ie_list_present) {
  	/* Parse information elements.*/
    if((ret = hitmac_parse_information_elements(buf + curr_len, buf_size - curr_len, ies)) == -1){
      PRINTF("HITMAC:! parse_eb: failed to parse IEs\n");
      return 0;
    }
    
    curr_len += ret;
  }

  return curr_len;
}
/*---------------------------------------------------------------------------*/
/* Parse all IEEE 802.15.4e Information Elements (IE) from a frame */
int
hitmac_parse_cmd_elements(const uint8_t *buf, uint8_t buf_size,
    uint8_t *cmd)
{
  uint16_t len = buf_size;
  if(cmd == NULL) {
    return -1;
  }
  if(len ==  1) {
    if(cmd != NULL) {
      *cmd = (uint32_t)buf[0];
      
    }
  }

  return len;
}
/*---------------------------------------------------------------------------*/
/* Parse a IEEE 802.15.4e REQUEST CMD  */
int
hitmac_packet_parse_cmd(const uint8_t *buf, int buf_size,
                     frame802154_t *frame, uint8_t *cmdtype)
{
  uint8_t curr_len = 0;
  int ret;

  if(frame == NULL || buf_size < 0) {
    return 0;
  }

  /* Parse 802.15.4-2006 frame, i.e. all fields before Information Elements */
  if((ret = frame802154_parse((uint8_t *)buf, buf_size, frame)) == 0) {
    PRINTF("HITMAC:! parse_cmd: failed to parse frame\n");
    return 0;
  }

  if(frame->fcf.frame_version < FRAME802154_IEEE802154E_2012
     || frame->fcf.frame_type != FRAME802154_CMDFRAME) {
    PRINTF("HITMAC:! parse_eb: frame is not a valid HITMAC CMD. Frame version %u, type %u, FCF %02x %02x\n",
           frame->fcf.frame_version, frame->fcf.frame_type, buf[0], buf[1]);
    PRINTF("HITMAC:! parse_cmd: frame was from 0x%x/", frame->src_pid);
    PRINTLLADDR((const uip_lladdr_t *)&frame->src_addr);
    PRINTF(" to 0x%x/", frame->dest_pid);
    PRINTLLADDR((const uip_lladdr_t *)&frame->dest_addr);
    PRINTF("\n");
    return 0;
  }

  curr_len += ret;

  if(cmdtype != NULL) {
   *cmdtype = 0;
    
  }
  
  if((ret = hitmac_parse_cmd_elements(buf + curr_len, buf_size - curr_len, cmdtype)) == -1){
    PRINTF("HITMAC:! parse_cmd: failed to parse CMD\n");
    return 0;
  }
  curr_len += ret;

  return curr_len;

}
/*---------------------------------------------------------------------------*/
char
hitmac_packet_is_broadcast(uint8_t *addr)
{
  if( addr[0] == 0xFF && addr[1] == 0xFF){
    return 1;
  }
  return 0;
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/