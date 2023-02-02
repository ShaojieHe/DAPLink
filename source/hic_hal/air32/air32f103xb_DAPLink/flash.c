#include "flash_hal.h"        // FlashOS Structures
#include "target_config.h"    // target_device
#include "air32f10x.h"
#include "air32f10x_flash.h"
#include "daplink_addr.h"
#include "util.h"
#include "string.h"
#include "target_board.h"

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
uint32_t Init(uint32_t adr, uint32_t clk, uint32_t fnc)
{
    //
    // No special init required
    //
    return (0);
}

uint32_t UnInit(uint32_t fnc)
{
    //
    // No special uninit required
    //
    return (0);
}

uint32_t EraseChip(void)
{
    uint32_t error;
    uint32_t ret = 0;
    uint32_t FlashSize = DAPLINK_SECTOR_SIZE;

    if (g_board_info.target_cfg) {
        FLASH_Unlock();

        util_assert((g_board_info.target_cfg->flash_regions[0].end - g_board_info.target_cfg->flash_regions[0].start) %
                FlashSize == 0);

        uint32_t PageStartAddress = g_board_info.target_cfg->flash_regions[0].start;
        uint32_t PageNumbers = (g_board_info.target_cfg->flash_regions[0].end - g_board_info.target_cfg->flash_regions[0].start) % FlashSize;
        
        uint32_t address;
        for(address = PageStartAddress;
            address < (PageNumbers * FlashSize) + PageStartAddress;
            address += FlashSize){

            if(FLASH_ErasePage(address) != FLASH_COMPLETE){
                ret = 1;
            }
        }
        FLASH_Lock();
    }   
    return ret;
}

uint32_t EraseSector(uint32_t adr)
{
    uint32_t ret = 0;

    FLASH_Unlock();

    if(FLASH_ErasePage(adr) != FLASH_COMPLETE) {
        ret = 1;
    }

    FLASH_Lock();
    return ret;
}

uint32_t ProgramPage(uint32_t adr, uint32_t sz, uint32_t *buf)
{
    uint32_t i;
    uint32_t ret = 0;  // O.K.

    FLASH_Unlock();

    util_assert(sz % 4 == 0);

    for(i = 0; i < sz / 4; i++){
        if(FLASH_ProgramWord(adr + i * 4, buf[i]) != FLASH_COMPLETE) {
            ret = 1;
            break;
        }
    }

    FLASH_Lock();
    return ret;
}
