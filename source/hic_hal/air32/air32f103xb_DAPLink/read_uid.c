#include "air32f10x.h"
#include "read_uid.h"

void read_unique_id(uint32_t *id)
{
    uint32_t Device_Serial0, Device_Serial1, Device_Serial2;    
    
    Device_Serial0 = *(uint32_t*)(0x1FFFF7E8); 
    Device_Serial1 = *(uint32_t*)(0x1FFFF7EC); 
    Device_Serial2 = *(uint32_t*)(0x1FFFF7F0);    
  
    id[0] = Device_Serial0;
    id[1] = Device_Serial1;
    id[2] = Device_Serial2;
    id[3] = 0xA5A5A5A5;
}
