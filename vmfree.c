#include "vm.h"
#include "vmlib.h"
#include <stddef.h>

/**
 * The vmfree() function frees the memory space pointed to by ptr,
 * which must have been returned by a previous call to vmalloc().
 * Otherwise, or if free(ptr) has already been called before,
 * undefined behavior occurs.
 * If ptr is NULL, no operation is performed.
 */
void vmfree(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    // getting the header of the block
    struct block_header *block =
        (struct block_header *)((char *)ptr - sizeof(struct block_header));

    // the block is marked as free
    block->size_status &= ~VM_BUSY;

    // coalescing when the next block is free
    struct block_header *next_block =
        (struct block_header *)((char *)block + BLKSZ(block));
    if (!(next_block->size_status & VM_BUSY) &&
        next_block->size_status != VM_ENDMARK) {
        // add the size of the next block to the current block
        block->size_status += BLKSZ(next_block);
    }

    // coalescing when the previous block is free
    if (next_block->size_status != VM_ENDMARK) {
        next_block = (struct block_header *)((char *)block + BLKSZ(block));
        next_block->size_status &= ~VM_PREVBUSY;
    }

    if (!(block->size_status & VM_PREVBUSY)) {
        // add the size of the current block to the previous block
        struct block_footer *prev_footer =
            (struct block_footer *)((char *)block -
                                    sizeof(struct block_footer));
        struct block_header *prev_block =
            (struct block_header *)((char *)block - prev_footer->size);

        if (!(prev_block->size_status & VM_BUSY)) {
            // add the size of the current block to the previous block
            prev_block->size_status += BLKSZ(block);
            block = prev_block;
        }
    }

    // updating the footer of the block was coalesced
    struct block_footer *footer =
        (struct block_footer *)((char *)block + BLKSZ(block) -
                                sizeof(struct block_footer));
    footer->size = BLKSZ(block);
}
