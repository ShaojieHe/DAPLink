#include "air32f10x.h"
#include "air32f10x_flash.h"
#include "DAP_config.h"
#include "gpio.h"
#include "daplink.h"
#include "misc.h"
#include "util.h"
#include "cortex_m.h"

void sdk_init()
{
    NVIC_SetPriorityGrouping(NVIC_PriorityGroup_4);
    RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_4Div5);
}
