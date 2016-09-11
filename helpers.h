/* Viziru Luciana - 332CA */

#include "common.h"

enum page_state {
	STATE_IN_RAM,
	STATE_IN_SWAP,
	STATE_NOT_ALLOC
};

struct frame;
struct zone_info;

/* handle pages (virtual pages) */
struct page_table_entry {
	enum page_state state;
	enum page_state prev_state;
	w_boolean_t dirty;
	w_prot_t protection;
	w_ptr_t start;
	w_size_t index;			/* index of page in swap space */
	struct frame *frame;	/* NULL in case page is not mapped */
	struct zone_info *zone; /* link to  zone page is a part of */
};

/* handle frames (physical pages) */
struct frame {
	w_size_t index;
	/* "backlink" to page_table_entry; NULL in case of free frame */
	struct page_table_entry *pte;
};

/* handle for a memory zone */
struct zone_info {
	struct page_table_entry *page_table; /* page table array */
	struct frame *frames;	/* frame array */
	int num_pages;
	int num_frames;
	w_handle_t ram;
	w_handle_t swap;
	int zone_nr; /* id of zone */
};
