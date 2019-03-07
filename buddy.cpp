/*
 * Buddy Page Allocation Algorithm
 * SKELETON IMPLEMENTATION -- TO BE FILLED IN FOR TASK (2)
 */

/*
 * STUDENT NUMBER: s1620208
 */
#include <infos/mm/page-allocator.h>
#include <infos/mm/mm.h>
#include <infos/kernel/kernel.h>
#include <infos/kernel/log.h>
#include <infos/util/math.h>
#include <infos/util/printf.h>

using namespace infos::kernel;
using namespace infos::mm;
using namespace infos::util;

#define MAX_ORDER	17

#define DEBUG_ENABLED true

#ifdef DEBUG_ENABLED
	#define debugf(...) mm_log.messagef(LogLevel::DEBUG, __VA_ARGS__);
#else
	#define debugf(...)
#endif

/**
 * A buddy page allocation algorithm.
 */
class BuddyPageAllocator : public PageAllocatorAlgorithm
{
private:
	/**
	 * Returns the number of pages that comprise a 'block', in a given order.
	 * @param order The order to base the calculation off of.
	 * @return Returns the number of pages in a block, in the order.
	 */
	static inline constexpr uint64_t pages_per_block(int order)
	{
		/* The number of pages per block in a given order is simply 1, shifted left by the order number.
		 * For example, in order-2, there are (1 << 2) == 4 pages in each block.
		 */
		return (1 << order);
	}
	
	/**
	 * Returns TRUE if the supplied page descriptor is correctly aligned for the 
	 * given order.  Returns FALSE otherwise.
	 * @param pgd The page descriptor to test alignment for.
	 * @param order The order to use for calculations.
	 */
	static inline bool is_correct_alignment_for_order(const PageDescriptor *pgd, int order)
	{
		// Calculate the page-frame-number for the page descriptor, and return TRUE if
		// it divides evenly into the number pages in a block of the given order.
		return (sys.mm().pgalloc().pgd_to_pfn(pgd) % pages_per_block(order)) == 0;
	}
	
	/** Given a page descriptor, and an order, returns the buddy PGD.  The buddy could either be
	 * to the left or the right of PGD, in the given order.
	 * @param pgd The page descriptor to find the buddy for.
	 * @param order The order in which the page descriptor lives.
	 * @return Returns the buddy of the given page descriptor, in the given order.
	 */
	PageDescriptor *buddy_of(PageDescriptor *pgd, int order)
	{
		// (1) Make sure 'order' is within range
		if (order >= MAX_ORDER) {
			return NULL;
		}

		// (2) Check to make sure that PGD is correctly aligned in the order
		if (!is_correct_alignment_for_order(pgd, order)) {
			return NULL;
		}
				
		// (3) Calculate the page-frame-number of the buddy of this page.
		// * If the PFN is aligned to the next order, then the buddy is the next block in THIS order.
		// * If it's not aligned, then the buddy must be the previous block in THIS order.
		uint64_t buddy_pfn = is_correct_alignment_for_order(pgd, order + 1) ?
			sys.mm().pgalloc().pgd_to_pfn(pgd) + pages_per_block(order) : 
			sys.mm().pgalloc().pgd_to_pfn(pgd) - pages_per_block(order);
		
		// (4) Return the page descriptor associated with the buddy page-frame-number.
		return sys.mm().pgalloc().pfn_to_pgd(buddy_pfn);
	}
	
	/**
	 * Inserts a block into the free list of the given order.  The block is inserted in ascending order.
	 * @param pgd The page descriptor of the block to insert.
	 * @param order The order in which to insert the block.
	 * @return Returns the slot (i.e. a pointer to the pointer that points to the block) that the block
	 * was inserted into.
	 */
	PageDescriptor **insert_block(PageDescriptor *pgd, int order)
	{
		debugf("insert_block(%p, %d)", pgd, order);

		// Starting from the _free_area array, find the slot in which the page descriptor
		// should be inserted.
		PageDescriptor **slot = &_free_areas[order];
		
		// Iterate whilst there is a slot, and whilst the page descriptor pointer is numerically
		// greater than what the slot is pointing to.
		while (*slot && pgd > *slot) {
			slot = &(*slot)->next_free;
		}
		
		// Insert the page descriptor into the linked list.
		pgd->next_free = *slot;
		*slot = pgd;
		
		// Return the insert point (i.e. slot)
		return slot;
	}
	
	/**
	 * Removes a block from the free list of the given order.  The block MUST be present in the free-list, otherwise
	 * the system will panic.
	 * @param pgd The page descriptor of the block to remove.
	 * @param order The order in which to remove the block from.
	 */
	void remove_block(PageDescriptor *pgd, int order)
	{
		// Starting from the _free_area array, iterate until the block has been located in the linked-list.
		PageDescriptor **slot = &_free_areas[order];
		while (*slot && pgd != *slot) {
			slot = &(*slot)->next_free;
		}

		// Make sure the block actually exists.  Panic the system if it does not.
		assert(*slot == pgd);
		
		// Remove the block from the free list.
		*slot = pgd->next_free;
		pgd->next_free = NULL;
	}
	
	/**
	 * Given a pointer to a block of free memory in the order "source_order", this function will
	 * split the block in half, and insert it into the order below.
	 * @param block_pointer A pointer to a pointer containing the beginning of a block of free memory.
	 * @param source_order The order in which the block of free memory exists.  Naturally,
	 * the split will insert the two new blocks into the order below.
	 * @return Returns the left-hand-side of the new block.
	 */
	PageDescriptor *split_block(PageDescriptor **block_pointer, int source_order)
	{
		debugf("SPLIT_BLOCK: block_pointer: %p, *block_pointer: *%p, source_order: %d", block_pointer, *block_pointer, source_order);

		// Make sure there is an incoming pointer.
		assert(*block_pointer);
		
		// Make sure the block_pointer is correctly aligned.
		assert(is_correct_alignment_for_order(*block_pointer, source_order));

		// Ensure source_order is greater than 0, as we can't insert into negative order
		assert(source_order > 0);

		// Mark the target_order
		int target_order = source_order - 1;

		// Get the blocks
		auto left = *block_pointer;
		auto right = buddy_of(left, target_order);

		// The LHS must be less than the RHS
		assert(left < right);

		// Remove this block
		remove_block(*block_pointer, source_order);

		// Add the new blocks
		insert_block(left, target_order);
		insert_block(right, target_order);
		
		return left;
	}

	/**
	 * Reorders two page descriptors to be in order
	 * @param left A pointer to a page descriptor (what is believed to be the LHS)
	 * @param right A pointer to another page descriptor (what is believed to be the RHS)
	 * @action Do left,right = right,left if left > right
	 */
	void order_pages(PageDescriptor* &left, PageDescriptor* &right) {
		if (left > right) {
			auto temp = left;
			left = right;
			right = temp;
		}
	}
	
	/**
	 * Takes a block in the given source order, and merges it (and it's buddy) into the next order.
	 * This function assumes both the source block and the buddy block are in the free list for the
	 * source order.  If they aren't this function will panic the system.
	 * @param block_pointer A pointer to a pointer containing a block in the pair to merge.
	 * @param source_order The order in which the pair of blocks live.
	 * @return Returns the new slot that points to the merged block.
	 */
	PageDescriptor **merge_block(PageDescriptor **block_pointer, int source_order)
	{
		assert(*block_pointer);
		
		// Make sure the area_pointer is correctly aligned.
		assert(is_correct_alignment_for_order(*block_pointer, source_order));

		// Ensure source_order is less than the max order (can't merge two largest orders)
		assert(source_order < MAX_ORDER);

		// Mark the target order
		int target_order = source_order + 1;

		// Get the blocks
		auto left = *block_pointer;
		auto right = buddy_of(left, source_order);

		// buddy_of may actually return the buddy on the "wrong" side, so reorder variables
		order_pages(left, right);

		// Remove the old blocks
		remove_block(left, source_order);
		remove_block(right, source_order);

		// Add the new block and return it
		return insert_block(left, target_order);
	}
	
public:
	/**
	 * Constructs a new instance of the Buddy Page Allocator.
	 */
	BuddyPageAllocator() {
		// Iterate over each free area, and clear it.
		for (unsigned int i = 0; i < ARRAY_SIZE(_free_areas); i++) {
			_free_areas[i] = NULL;
		}
	}
	
	/**
	 * Allocates 2^order number of contiguous pages
	 * @param order The power of two, of the number of contiguous pages to allocate.
	 * @return Returns a pointer to the first page descriptor for the newly allocated page range, or nullptr if
	 * allocation failed.
	 */
	PageDescriptor *alloc_pages(int target_order) override
	{
		debugf("ALLOC_PAGES: target_order: %d", target_order)

		// Ensure order is valid
		assert(target_order >= 0);
		assert(target_order <= MAX_ORDER);

		debugf("ALLOC_PAGES: assertion success");

		// Start off with the target order
		int current_order = target_order;
		auto free_block = _free_areas[current_order];

		while (!free_block || current_order > target_order) {
			// debugf("***** WHILE DUMP START")
			// dump_state();
			// debugf("***** WHILE DUMP END")

			// Short-circuit if the current order is invalid (cannot allocate page)
			if (current_order > MAX_ORDER || current_order < 0) {
				debugf("ALLOC_PAGES: cannot allocate page. current_order is %d (this is the eventual order)", current_order)
				return nullptr;
			}

			// If the current order is splittable...
			if (_free_areas[current_order]) {
				// Split and decrement
				debugf("ALLOC_PAGES: splitting up free area %p (current_order: %d)", _free_areas[current_order], current_order);
				free_block = split_block(&_free_areas[current_order], current_order);
				current_order--;
			} else {
				// Split the larger size (later)
				current_order++;
			}
		}

		// Remove the block from the free areas, and return it
		remove_block(free_block, target_order);

		debugf("ALLOC_PAGES: returning %p", free_block)
		return free_block;
	}
	
	/**
	 * Checks whether a given page is free. (Student defined.)
	 * @param pgd The page descriptor of the page to check is free.
	 * @param order The power of two number of contiguous pages to check
	 * @return Return TRUE if the page exists in _free_areas, FALSE otherwise.
	 */
	bool is_page_free(PageDescriptor* pgd, int order)
	{
		auto page = _free_areas[order];
		while (page != nullptr) {
			if (page == pgd) {
				return true;
			}

			page = page->next_free;
		}

		return false;
	}

	/**
	 * Get the smallest page descriptor (Student defined.)
	 * @param pgd_a A pointer to the page descriptor for a page
	 * @param pgd_b A pointer to another page descriptor (usually its buddy)
	 * @return Returns the smallest page descriptor
	 */
	PageDescriptor* get_smallest_pgd(PageDescriptor* pgd_a, PageDescriptor* pgd_b) {
		return (pgd_a > pgd_b) ? pgd_b : pgd_a;
	}

	/**
	 * Check whether a block contains a page. (Student defined.)
	 * @param block A pointer to an array of page descriptors
	 * @param order The power of two of the number of contiguous pages in the block
	 * @param pgd The page to check if is inside the block
	 * @return Returns TRUE if the page is inside the block, FALSE otherwise
	 */
	bool does_block_contain_page(PageDescriptor* block, int order, PageDescriptor* pgd) {
		auto size = pages_per_block(order);
		PageDescriptor* last_page = block + size;
		return (pgd >= block) && (pgd <= last_page);
	}

	/**
	 * The result of a coalesce call. This is useful so you know what the new order is.
	 * @param pgd A pointer to the first page descriptor for the page.
	 * @param order The order of that page descriptor
	 */
	struct CoalesceResult
	{
		PageDescriptor* pgd;
		int order;
	};

	/**
	 * Merges pages as much as possible. (Student defined.)
	 * @param pgd A pointer to an array of page descriptors.
	 * @param order The power of two of number of contiguous pages.
	 * @return Returns the result of the coalesce request (see CoalesceResult for more details)
	 */
	CoalesceResult coalesce(PageDescriptor* pgd, int order) {
		// Make sure that the incoming page descriptor is correctly aligned
		// for the order on which it is being freed, for example, it is
		// illegal to free page 1 in order-1.
		assert(is_correct_alignment_for_order(pgd, order));

		// Ensure order is valid
		assert(order >= 0);
		assert(order <= MAX_ORDER);

		// If we are on largest order, we can't coalesce further, so short-circuit.
		if (order == MAX_ORDER) {
			return CoalesceResult{pgd, order};
		}

		auto buddy = buddy_of(pgd, order);
		while (is_page_free(buddy, order)) {
			// Since the buddy is free, merge ourselves and the buddy. Always returns the LHS.
			pgd = *merge_block(&pgd, order);

			// Now pgd refers to the free pgd in an order above, so bump the order
			order++;

			// Update our buddy reference to be of the higher order
			buddy = buddy_of(pgd, order);

			// If we have hit the max order, we shouldn't continue
			if (order == MAX_ORDER) {
				break;
			}
		}

		return CoalesceResult{pgd, order};
	}

	/**
	 * Frees 2^order contiguous pages.
	 * @param pgd A pointer to an array of page descriptors to be freed.
	 * @param order The power of two number of contiguous pages to free.
	 */
	void free_pages(PageDescriptor *pgd, int order) override
	{
		// Make sure that the incoming page descriptor is correctly aligned
		// for the order on which it is being freed, for example, it is
		// illegal to free page 1 in order-1.
		assert(is_correct_alignment_for_order(pgd, order));
		
		// Ensure order is valid
		assert(order >= 0);
		assert(order <= MAX_ORDER);

		// Free these pages straight away.
		insert_block(pgd, order);

		// Now coalesce
		coalesce(pgd, order);
	}

	/**
	 * Reserves a specific page, so that it cannot be allocated.
	 * @param pgd The page descriptor of the page to reserve.
	 * @return Returns TRUE if the reservation was successful, FALSE otherwise.
	 */
	bool reserve_page(PageDescriptor *pgd)
	{
		auto order = MAX_ORDER;
		PageDescriptor* current_block = nullptr;

		// For each order, starting from largest
		while (order >= 0)
		{
			// If the pgd matches and order is 0, we are done!
			if (current_block == pgd && order == 0) {
				remove_block(current_block, order);
				return true;
			}

			// If the block containing the page has been found...
			if (current_block != nullptr) {
				auto left = split_block(&current_block, order);
				auto new_order = order - 1;

				// If the LHS-block contains the page...
				if (does_block_contain_page(left, new_order, pgd)) {
					// ...update the current block!
					current_block = left;
				} else {
					auto right = buddy_of(left, new_order);

					// The RHS must contain the block
					assert(does_block_contain_page(right, new_order, pgd));

					current_block = right;
				}

				// Split further down, on the next loop.
				order--;
				continue;
			}

			// Search through the free areas, starting off with the first free area of this order
			current_block = _free_areas[order];
			while (current_block != nullptr) {

				// If pgd is between (inclusive) the current block and the last page of that block...
				if (does_block_contain_page(current_block, order, pgd)) {
					// ... stop searching, we've discovered the block!
					break;
				}

				current_block = current_block->next_free;
			}

			// Search lower down if the block was not found.
			if (current_block == nullptr) {
				order--;
			}
		}

		// Couldn't find, so we're done!
		return false;
	}
	
	/**
	 * Initialises the allocation algorithm.
	 * @return Returns TRUE if the algorithm was successfully initialised, FALSE otherwise.
	 */
	bool init(PageDescriptor *page_descriptors, uint64_t nr_page_descriptors) override
	{
		mm_log.messagef(LogLevel::DEBUG, "Buddy Allocator Initialising pd=%p, nr=0x%lx", page_descriptors, nr_page_descriptors);
		
		// TODO: Initialise the free area linked list for the maximum order
		// to initialise the allocation algorithm.
		auto largest_block_size = pages_per_block(MAX_ORDER);
		auto remaining_pages = nr_page_descriptors % largest_block_size;
		auto block_count = (nr_page_descriptors - remaining_pages) / largest_block_size;

		auto last_block = page_descriptors + (block_count * largest_block_size);

		debugf("INIT: pages per block: %d, blocks to allocate: %d", largest_block_size, block_count);

		for (auto block = page_descriptors; block <= last_block; block += largest_block_size) {
			insert_block(block, MAX_ORDER);
		}

		debugf("INIT: done inserting pages.")

		// todo
		if (remaining_pages != 0) {
			assert(false && "init function - remaining_pages is 0 - NOT IMPLEMENTED");
		}

		debugf("INIT: done initialising buddy algorithm")
		return true;
	}

	/**
	 * Returns the friendly name of the allocation algorithm, for debugging and selection purposes.
	 */
	const char* name() const override { return "buddy"; }
	
	/**
	 * Dumps out the current state of the buddy system
	 */
	void dump_state() const override
	{
		// Print out a header, so we can find the output in the logs.
		mm_log.messagef(LogLevel::DEBUG, "BUDDY STATE:");
		
		// Iterate over each free area.
		for (unsigned int i = 0; i < ARRAY_SIZE(_free_areas); i++) {
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "[%d] ", i);
						
			// Iterate over each block in the free area.
			PageDescriptor *pg = _free_areas[i];
			while (pg) {
				// Append the PFN of the free block to the output buffer.
				snprintf(buffer, sizeof(buffer), "%s%lx ", buffer, sys.mm().pgalloc().pgd_to_pfn(pg));
				pg = pg->next_free;
			}
			
			mm_log.messagef(LogLevel::DEBUG, "%s", buffer);
		}
	}

	
private:
	PageDescriptor *_free_areas[MAX_ORDER];
};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

/*
 * Allocation algorithm registration framework
 */
RegisterPageAllocator(BuddyPageAllocator);
