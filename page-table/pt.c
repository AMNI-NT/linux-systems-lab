#include "os.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <sys/mman.h>

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn){

	uint64_t walk_ppn = pt;
	uint64_t level_vpn;
	uint64_t* node;
	uint64_t pte;

	for(int i = 0; i < 5; i++){

		node = (uint64_t*) phys_to_virt(walk_ppn << 12);
		level_vpn = (vpn >> (36 - 9*i)) & 0x1ff;
		pte = node[level_vpn];

		/* we haven't reached a leaf yet */
		if(i != 4){ 
			if( (pte & 1) == 0){	
				walk_ppn = alloc_page_frame();
				node[level_vpn] = (walk_ppn << 12) | 1;
			}
			else
				walk_ppn = pte >> 12;
			
		}
		/* we have reached a leaf*/
		else{
			if(ppn == NO_MAPPING)
				node[level_vpn] = 0;
			
			else
				node[level_vpn] = (ppn << 12) | 1;
		}
	}
}

uint64_t page_table_query(uint64_t pt, uint64_t vpn){

	uint64_t ppn = pt;
	uint64_t level_vpn;
	uint64_t* node;
	uint64_t pte;

	for(int i = 0; i < 5; i++){

		node = (uint64_t*) phys_to_virt(ppn << 12);
		level_vpn = (vpn >> (36 - 9*i)) & 0x1ff;
		pte = node[level_vpn];

		if( (pte & 1) == 0)
			return NO_MAPPING;

		ppn = pte >> 12;
	}

	return ppn;
}
