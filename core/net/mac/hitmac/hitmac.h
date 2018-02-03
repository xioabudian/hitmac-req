#ifndef __HITMAC_H__
#define __HITMAC_H__
#include "hitmac-conf.h"

//define scheduler
struct hitmac_scheduler_t{
	uint32_t up_load;
	uint32_t down_load;
	uint32_t sleep_time;
	uint32_t time_length;
	uint32_t sync_period;

};

//define absolute slot number
struct hitmac_asn_t{
	uint32_t ls4b;
	uint8_t ms1b;
};

/* Initialize ASN */
#define HITMAC_ASN_INIT(asn, ms1b_, ls4b_) do { \
    (asn).ms1b = (ms1b_); \
    (asn).ls4b = (ls4b_); \
} while(0);

/* Increment an ASN by inc (32 bits) */
#define HITMAC_ASN_INC(asn, inc) do { \
    uint32_t new_ls4b = (asn).ls4b + (inc); \
    if(new_ls4b < (asn).ls4b) { (asn).ms1b++; } \
    (asn).ls4b = new_ls4b; \
} while(0);

/* Decrement an ASN by inc (32 bits) */
#define HITMAC_ASN_DEC(asn, dec) do { \
    uint32_t new_ls4b = (asn).ls4b - (dec); \
    if(new_ls4b > (asn).ls4b) { (asn).ms1b--; } \
    (asn).ls4b = new_ls4b; \
} while(0);

/*Initialize SCHEDULER*/
#define HITMAC_SCHDELER_INIT(schedule,up_load_,down_load_,sleep_time_,time_length_,sync_period_) do{ \
	(schedule).up_load = (up_load_); \
	(schedule).down_load = (down_load_); \
	(schedule).sleep_time = (sleep_time_); \
	(schedule).time_length = (time_length_); \
	(schedule).sync_period = (sync_period_); \
} while(0);

/*Define asn mode operation:asn % time_length*/

#define HITMAC_ASN_MOD(asn,div,res) \
	uint32_t hitmac_front_ms = (uint32_t)((asn).ms1b<<16); \
	uint32_t hitmac_tail_ms = 0x10000; \
	res = ((hitmac_front_ms%div*hitmac_tail_ms%div)%div%div + asn.ls4b%div)%div;

/* The HIT MAC driver */
extern int hitmac_is_root;
extern const struct mac_driver hitmac_driver;

#endif