/* Viziru Luciana - 332CA */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>

#include "vmsim.h"
#include "linkedlist.h"
#include "util.h"
#include "helpers.h"

/* typedefs for shorter variable names */
typedef struct page_table_entry pgtbl_entry;
typedef struct zone_info zone;

/* global list of all allocated virtual memory zones */
static list *zones;

/* function that searches for page containing the address addr*/
static pgtbl_entry* get_page_for_adress(w_ptr_t addr)
{
	int i, j, n, m, page_size;
	zone **all_zones;
	pgtbl_entry *entry = NULL;

	page_size = w_get_page_size();

	/* get array from zones list */
	all_zones = (zone**)get_values(zones);
	DIE(all_zones == NULL, "get values on list failed");
	n = get_size(zones);
	DIE(n < 0, "get size on list failed");

	/* for every zone, for every page in zone, check if addr is */
	/* contained in page and save needed page in entry variable*/
	for (i = 0; i < n && entry == NULL; i++) {
		m = all_zones[i]->num_pages;
		for (j = 0; j < m; j++) {
			if (addr >= all_zones[i]->page_table[j].start &&
				addr < all_zones[i]->page_table[j].start + page_size) {
				entry = &all_zones[i]->page_table[j];
				break;
			}
		}
	}
	/* if no page was found, entry is NULL */
	return entry;
}

/* function looks for first free frame in ram */
/* returns NULL for no free frames */
static struct frame* get_free_frame(zone *myzone)
{
	int i, n;
	/* get number of frames in zone */
	/* for every frame in zone, if it's empty, return */
	n = myzone->num_frames;
	for (i = 0; i < n; i++) {
		if (myzone->frames[i].pte == NULL) {
			return &myzone->frames[i];
		}
	}
	return NULL;
}

/* function that swaps out a random frame in zone */
/* and returns a reference to it */
static struct frame* swap_out(zone* myzone)
{
	int frame_index, rc, page_size;
	w_boolean_t ret;
	w_ptr_t start;
	struct frame *frame;
	page_size = w_get_page_size();

	/* generate random index frame to be swapped out */
	frame_index = rand() % myzone->num_frames;

	/* evacuate frame from ram, move page to swap */
	/* if necessary, copy frame data to swap */
	frame = &myzone->frames[frame_index];
	if (frame->pte->dirty == TRUE || (frame->pte->dirty == FALSE
					&& frame->pte->prev_state == STATE_NOT_ALLOC)) {

		/* copy page data to buff and then write to swap file */
		char* buff = calloc(page_size, sizeof(char));
		DIE(buff == NULL, "malloc failed");
		memcpy(buff, frame->pte->start, page_size);

		ret = w_set_file_pointer(myzone->swap,
							frame->pte->index * page_size);
		DIE(ret == FALSE, "set file pointer failed");
		ret = w_write_file(myzone->swap, buff, page_size);
		DIE(ret == FALSE, "write file failed");
		free(buff);
	}

	/* unmap from ram memory */
	rc = munmap(frame->pte->start, page_size);
	DIE(rc < 0, "munmap failed");

	/* map back to virtual memory */
	start = mmap(frame->pte->start, page_size, PROT_NONE,
					MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
	DIE(start == MAP_FAILED, "mmap failed");

	/* update page info */
	frame->pte->prev_state = frame->pte->state;
	frame->pte->state = STATE_IN_SWAP;
	frame->pte->dirty = FALSE;
	frame->pte->protection = PROTECTION_NONE;
	frame->pte->start = start;
	frame->pte->frame = NULL;

	/* update frame info */
	frame->pte = NULL;

	return frame;
}

/* function that handles write page faults, adding write rigths to page */
static void change_to_prot_write(pgtbl_entry *page)
{
	int rc, page_size;
	page_size = w_get_page_size();
	/* change protection for given page */
	rc = mprotect(page->start, page_size, PROT_READ | PROT_WRITE);
	DIE(rc < 0, "mprotect failed");
	/* update page info */
	page->protection = PROTECTION_WRITE;
	page->dirty = TRUE;
}

/* page fault handler */
void vmsim_handler(int signum, siginfo_t *info, void *context)
{
	w_ptr_t addr;
	w_boolean_t ret;
	w_ptr_t start;
	int i, n, page_size, rc;
	struct frame *frame;
	pgtbl_entry* page = NULL;
	page_size = w_get_page_size();

	if (signum == SIGSEGV || info->si_signo == SIGSEGV) {

		/* handle signal */
		/* look for the page that caused the signal */
		page = get_page_for_adress(info->si_addr);
		DIE(page == NULL, "sigsegv for page not allocd by simulator");

		/* fix cause of page fault */
		if (page->state == STATE_IN_RAM) {
			/* this means page is in RAM and protection has to change */
			change_to_prot_write(page);
		}
		else {
			/* page needs to be mapped in ram */
			/* look for free frame */
			frame = get_free_frame(page->zone);

			/* if no free frames, make room */
			if (frame == NULL) {
				frame = swap_out(page->zone);
			}

			/* set frame to point to page */
			frame->pte = page;
			/* if demand paging needed */
			if (page->state == STATE_NOT_ALLOC) {

				/* clear frame for first alloc of page */
				ret = w_set_file_pointer(page->zone->ram,
											frame->index * page_size);
				DIE(ret == FALSE, "set file pointer failed");

				char* buff = calloc(page_size, sizeof(char));
				DIE(buff == NULL, "malloc failed");

				ret = w_write_file(page->zone->ram, buff, page_size);
				DIE(ret == FALSE, "write file failed");
				free(buff);
			}
			else {
				/* if swap in needed */
				/* initialize buffer */
				char* buff = calloc(page_size, sizeof(char));
				DIE(buff == NULL, "malloc failed");
				/* read from swap file to buffer */
				ret = w_set_file_pointer(page->zone->swap,
									page->index * page_size);
				DIE(ret == FALSE, "set file pointer failed");
				ret = w_read_file(page->zone->swap, buff, page_size);
				DIE(ret == FALSE, "write file failed");

				/* write from buffer to ram */
				ret = w_set_file_pointer(page->zone->ram,
									frame->index * page_size);
				DIE(ret == FALSE, "set file pointer failed");
				ret = w_write_file(page->zone->ram, buff, page_size);
				DIE(ret == FALSE, "write file failed");
				free(buff);
			}

			/* unmap from virtual memory */
			rc = munmap(page->start, page_size);
			DIE(rc < 0, "munmap failed");

			/* map virtual page to ram */
			start = mmap(page->start, page_size, PROT_READ, MAP_SHARED |
					MAP_FIXED,	page->zone->ram, frame->index * page_size);
			DIE(start == MAP_FAILED, "mmap failed");

			/* update page info */
			page->start = start;
			page->frame = frame;
			page->prev_state = page->state;
			page->state = STATE_IN_RAM;
			page->protection = PROTECTION_READ;
		}
	}
}

w_boolean_t vmsim_init(void)
{
	w_boolean_t ret;
	/* redirect exception handler */
	ret = w_set_exception_handler(vmsim_handler);
	DIE(ret == FALSE, "set exception handler failed");

	/* internal logic initializations */
	zones = create_list();
	DIE(zones == NULL, "create list failed");

	return TRUE;
}

w_boolean_t vmsim_cleanup(void)
{
	w_boolean_t ret;
	w_exception_handler_t handler;
	/* set previous exception handler */
	ret = w_get_previous_exception_handler(&handler);
	DIE(ret == FALSE, "get previous exception handler failed");
	ret = w_set_exception_handler(handler);
	DIE(ret == FALSE, "set exception handler failed");

	/* internal logic cleanup */
	destruct_list(zones);

	return TRUE;
}

w_boolean_t vm_alloc(w_size_t num_pages, w_size_t num_frames, vm_map_t *map)
{
	static int count_allocs = 0;
	w_handle_t ram;
	w_handle_t swap;
	w_ptr_t start;
	int i, ret;
	int page_size;
	zone *current_zone;
	pgtbl_entry *page_table;
	struct frame *frames;

	/* check if alloc arguments are valid */
	if (num_pages < num_frames) {
		return FALSE;
	}

	page_size = w_get_page_size();

	/* memory zone initializations */
	count_allocs++;
	char swap_name[16], ram_name[16];
	sprintf(swap_name, "swap%d", count_allocs);
	sprintf(ram_name, "ram%d", count_allocs);

	/* set swap file */
	swap = open(swap_name, O_RDWR | O_CREAT, 0600);
	DIE(swap < 0, "swap open failed");
	ret = ftruncate(swap, num_pages * page_size);
	DIE(ret != 0, "ftruncate swap failed");
	map->swap_handle = swap;

	/* set ram file */
	ram = open(ram_name, O_RDWR | O_CREAT, 0600);
	DIE(swap < 0, "ram open failed");
	ret = ftruncate(ram, num_frames * page_size);
	DIE(ret != 0, "ftruncate ram failed");
	map->ram_handle = ram;

	/* map virtual memory */
	/* create and populate page table for zone */
	page_table = malloc(num_pages * sizeof(pgtbl_entry));
	DIE(page_table == NULL, "page table malloc failed");

	/* alloc virtual memory in process memory */
	start = mmap(NULL, page_size * num_pages, PROT_NONE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	DIE(start == MAP_FAILED, "mmap failed");

	/* set start of memory zone in map struct */
	map->start = start;

	/* alloc memory zone */
	current_zone = malloc(sizeof(zone));
	DIE(current_zone == NULL, "malloc failed");

	/* add info to page table entry for every virtual page */
	for (i = 0; i < num_pages; i++) {
		page_table[i].state = STATE_NOT_ALLOC;
		page_table[i].prev_state = STATE_NOT_ALLOC;
		page_table[i].dirty = FALSE;
		page_table[i].protection = PROTECTION_NONE;
		page_table[i].start = start + i * page_size;
		page_table[i].index = i;
		page_table[i].frame = NULL;
		page_table[i].zone = current_zone;
	}

	/* alloc frames array of zone */
	frames = malloc(num_frames * sizeof(struct frame));
	DIE(frames == NULL, "frames malloc failed");

	/* initialize frames */
	for (i = 0; i < num_frames; i++) {
		frames[i].index = i;
		frames[i].pte = NULL;
	}

	/* set and add zone to zone container */
	current_zone->page_table = page_table;
	current_zone->frames = frames;
	current_zone->num_pages = num_pages;
	current_zone->num_frames = num_frames;
	current_zone->ram = ram;
	current_zone->swap = swap;
	current_zone->zone_nr = count_allocs;
	insert(zones, current_zone, sizeof(zone));

	return TRUE;
}

w_boolean_t vm_free(w_ptr_t start)
{
	char fname[16];
	int i, n, page_size, rc;
	zone *to_free = NULL;
	zone **entries = (zone**)get_values(zones);

	page_size = w_get_page_size();

	n = get_size(zones);
	DIE(n < 0, "get size on null list");

	/* look for memory zone starting at adress start */
	for (i = 0; i < n; i++) {
		if (entries[i]->page_table[0].start == start) {
			to_free = entries[i];
			break;
		}
	}
	/* found zone starting at start adress */
	if (to_free != NULL) {
		/* unmap all pages in zone */
		n = to_free->num_pages;
		for (i = 0; i < n; i++) {
			rc = munmap(to_free->page_table[i].start, page_size);
			DIE(rc < 0, "munmap failed");
		}
		/* free previously malloc'd arrays and delete zone from list */
		free(to_free->page_table);
		free(to_free->frames);
		remove_value(zones, to_free, sizeof(zone));

		/* remove ram file */
		close(to_free->ram);
		sprintf(fname, "ram%d", to_free->zone_nr);
		unlink(fname);

		/* remove swap file */
		close(to_free->swap);
		sprintf(fname, "swap%d", to_free->zone_nr);
		unlink(fname);

		return TRUE;
	}

	return FALSE;
}
