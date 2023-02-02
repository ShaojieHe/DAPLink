#ifndef __AIR_USB_ENHANCE_REGS_H
#define __AIR_USB_ENHANCE_REGS_H

#define RegBase  (0x40005C00L)  /* USB_IP Peripheral Registers base address */

#define DP_PUUP  *((__IO unsigned *)(RegBase + 0x54))
	
#endif
