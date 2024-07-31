#include "vm.h"
#include "vmlib.h"
#include <stdio.h>

void *vmalloc(size_t size)
{
    if (size <= 0) {
        return NULL;
    }

    size_t totalSize = size + sizeof(struct block_header);

    totalSize = ROUND_UP(totalSize, BLKALIGN); // rounding the size to 16

    struct block_header *block = heapstart; // pointer to the start of the heap
    struct block_header *smallestBlock = NULL;
    size_t blockSize;
    int busy;
    int prev_busy;

    while (block->size_status !=
           VM_ENDMARK) { // traversing the heap to find the best fit
        blockSize = BLKSZ(block);

        busy =
            block->size_status & VM_BUSY ? 1 : 0; // updating the value of busy

        if (blockSize >= totalSize && !busy) { // if the block is ideal

            if (smallestBlock == NULL || blockSize < BLKSZ(smallestBlock)) {
                // if the block is first one or better than the last fit

                prev_busy = block->size_status & VM_PREVBUSY
                                ? 1
                                : 0; // updating the value of prev_busy

                smallestBlock = block;
            }
        }

        block = (struct block_header *)((char *)block +
                                        blockSize); // go to the next block
    }

    if (smallestBlock ==
        NULL) { // if there is no ideal block in the entire memory
        return NULL;
    }

    struct block_header *nextBlock =
        (struct block_header *)((char *)smallestBlock + BLKSZ(smallestBlock));

    size_t bstfitz = BLKSZ(smallestBlock);

    if (bstfitz > totalSize) { // if block needs to be split
        size_t newsplit = bstfitz - totalSize;
        smallestBlock->size_status = totalSize | VM_BUSY;

        if (prev_busy) {

            smallestBlock->size_status |= VM_PREVBUSY;
        }

        struct block_header *new_block =
            (struct block_header *)((char *)smallestBlock +
                                    totalSize); // updating the block header to
                                                // that of the new block

        new_block->size_status = newsplit | VM_PREVBUSY;

        // new - setting footer for the free block
        struct block_footer *new_footer =
            (struct block_footer *)((char *)new_block + newsplit -
                                    sizeof(struct block_footer));
        new_footer->size = newsplit;

    } else { // allocate the block

        smallestBlock->size_status |= VM_BUSY;
        if (nextBlock->size_status != VM_ENDMARK) {
            nextBlock->size_status |= VM_PREVBUSY;
        }
    }

    smallestBlock =
        (void *)((char *)smallestBlock + sizeof(struct block_header));

    return smallestBlock;
}
