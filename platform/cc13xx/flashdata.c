#include "ti-lib.h"
#include "flashdata.h"
#include "cpu/cc26xx-cc13xx/lib/cc13xxware/driverlib/flash.h"
#include <stdio.h>
/*--------------------------------------------------------------------------*/
void
read_flash(uint32_t wAddr, uint8_t *bBuf, uint8_t bLen)
{

  uint8_t *Flash_Ptr = (uint8_t *)wAddr;

  /*exceed legal address*/
  if(wAddr < SECTOR_TO_ERASE_START_ADDR) {
    printf("exceed legal address\n");
    return;
  }

  /*exceed legal length*/
  if((wAddr + bLen) >= SECTOR_TO_ERASE_LENGTH + SECTOR_TO_ERASE_START_ADDR) {
    printf("exceed legal length\n");
    return;
  }

  while(bLen--) {
    *(bBuf) = *(Flash_Ptr);
    bBuf++;
    Flash_Ptr++;
  }
}
/*--------------------------------------------------------------------------*/
int
write_to_flash(uint8_t data[], uint32_t start_addr, uint32_t len)
{

  int32_t result;

  uint32_t i = 0;
  uint32_t start;
  uint8_t data1[APP_DATA_SIZE] = { 0 };

  /*exceed legal address*/
  if(start_addr < SECTOR_TO_ERASE_START_ADDR) {
    printf("exceed legal address\n");
    return 1;
  }

  /*exceed legal length*/
  if((start_addr + len) >= APP_DATA_SIZE + SECTOR_TO_ERASE_START_ADDR) {
    printf("exceed legal length: %d\n", (int)len);
    return 1;
  }

  /*before erasing ,please copy and store the appdata*/
  read_flash(SECTOR_TO_ERASE_START_ADDR, data1, APP_DATA_SIZE);

  start = (start_addr - SECTOR_TO_ERASE_START_ADDR);

  for(i = start; i < len + start; i++) {
    data1[(int)i] = data[(int)(i - start)];
  }

  /*before write,please keep the sector erased*/
  result = FlashSectorErase(SECTOR_TO_ERASE_START_ADDR);

  if(result != FAPI_STATUS_SUCCESS) {
    printf("erase flash error\n");
    return 1;
  }

  /*Disable the cache*/
  VIMSModeSet(VIMS_BASE, VIMS_MODE_DISABLED);
  while(VIMSModeGet(VIMS_BASE) != VIMS_MODE_DISABLED) ;

  /*make sure the sector isn't write protected*/
  result = FlashProtectionGet(start_addr);
  if(result == FLASH_WRITE_PROTECT) {
    printf("write into protect area\n");
    return 1;
  }

  /*Disable all interrupts when accessing the flash*/
  CPUcpsid();

  /* Program 128 bytes in chunks of 8 bytes */

  /*Programs unprotected main bank flash sectors*/
  result = FlashProgram(data1, SECTOR_TO_ERASE_START_ADDR, APP_DATA_SIZE);

  if(result != FAPI_STATUS_SUCCESS) {
    printf("write flash area failed\n");
    return 1;
  }

  CPUcpsie();

  /*Re-enable the cache*/
  VIMSModeSet(VIMS_BASE, VIMS_MODE_ENABLED);

  return 0;
}
/*---------------------------------------------------------------------------*/