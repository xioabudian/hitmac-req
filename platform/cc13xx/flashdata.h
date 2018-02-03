#ifndef FLASHDATA_H_
#define FLASHDATA_H_
#include <inttypes.h>
/*Flash defines*/

#define APP_DATA_SIZE 200
#define SECTOR_SIZE      4096 /* the last page 30,size 4kb */
#define SECTOR_TO_ERASE_START_ADDR   0x1e000
#define SECTOR_TO_ERASE_LENGTH 0x1000  /* waring: Don't overstep the boundary,this area contain net configuration information. */
/* they are very important */
int write_to_flash(uint8_t data[], uint32_t start_addr, uint32_t len);
void read_flash(uint32_t wAddr, uint8_t *bBuf, uint8_t bLen);

#endif /* FLASHDATA_H_ */