# Segregated Free Lists (SFL) - Dynamic Heap Management System

**Author:** Greere Ioan Stefan

## Overview

This project implements a simulation of a memory allocation and management system using the **Segregated Free Lists (SFL)** approach[cite: 1]. The SFL method is a sophisticated technique for managing free memory blocks, designed to improve the performance of heap operations like `malloc` and `free` by grouping blocks of similar sizes.

The core of the simulation is built upon dynamic data structures in C, primarily **doubly-linked lists (DLLs)**, to represent and manipulate the memory blocks[cite: 1, 21].

## Core Data Structures

The simulation revolves around two primary structures: the SFL vector for free blocks and a separate DLL for allocated blocks[cite: 1].

1. **`sfl` (Segregated Free List Heap):** This structure manages the overall heap and the collection of free lists[cite: 1].
    * `memory (dl_list_t **)`: A dynamic array of pointers to doubly-linked lists. This is the **vector of doubly-linked lists**.
    * `capacity (int *)`: An array storing the size (in bytes) of the blocks contained within each respective list. This structure allows for fast lookup of a suitable free block.

2. **`mem_alloc` (Allocated Blocks List - `dl_list_t *`):** A single doubly-linked list used to store *all* the memory blocks that have been allocated via the `MALLOC` command.
    * Blocks in this list are stored in **ascending order of their memory addresses** to facilitate contiguous memory checks during `READ` and `WRITE` operations.

3.  **`info` (Block Information):** The data carried by each node (block) in both the SFL lists and the allocated list.
    * `address (int)`: The starting memory address of the block.
    * `dimension (int)`: The total size of the block (in bytes)[cite: 2].

## Command Implementation Details

### `INIT_HEAP`

* The two main data structures are created: the **vector of doubly-linked lists** and a separate doubly-linked list for allocated memory blocks[cite: 1].
* The vector of lists is created by calculating the necessary data, including the block size, the starting address for each block, and the number of nodes per list[cite: 2].
* The list of allocated blocks is initialized without any nodes[cite: 3].

### `MALLOC`

* The search for a suitable memory block in the vector of lists begins, initially presuming the block won't fragment[cite: 4, 5].
* Once a block is found, it is removed from its list[cite: 6]. A new node for the allocated portion is created and added to the `mem_alloc` list[cite: 6].
* **Fragmentation Handling:** If the block fragments, the unallocated remainder is added back to the appropriate free list[cite: 7]. If a list of that size doesn't exist, the vector of lists is resized and re-sorted[cite: 7].

### `FREE`

* The block is located in the `mem_alloc` list using its address[cite: 8].
* The freed block is checked against the SFL vector for a list matching its dimension[cite: 9].
* The freed node is added back to the appropriate list in **ascending order of addresses**.
* If a list for that dimension doesn't exist, a new one is created in the vector, which is then re-sorted by block size.

### `READ` / `WRITE`

* Both commands first verify if a **continuous memory area** exists within the allocated blocks (`mem_alloc`) for the requested number of bytes.
* **Reading:** If valid, the function iterates through consecutive allocated blocks to read the data based on the number of bytes required[cite: 12, 13].
* **Writing:** If valid, data is written across consecutive allocated blocks[cite: 15, 16].
* **Segmentation Fault:** If the required memory region is not continuous or the address is invalid, a "Segmentation fault" message would be displayed (based on implementation logic) followed by a `DUMP_MEMORY` operation.

### `DUMP_MEMORY`

* Displays comprehensive statistics about the heap state[cite: 17].
* Traverses both the SFL lists and the `mem_alloc` list to print block addresses and capacities[cite: 17].

### `DESTROY_HEAP`

* Stops data reading and frees all dynamically allocated memory for the entire program[cite: 18].

## Build and Run

The project uses a standard `Makefile` for compilation and execution.

```bash
# Build the executable 'sfl'
make build

# Run the project
make run_sfl

# Create the submission archive (if required)
make pack

# Clean up executable and object files
make clean

## Deep Dive into Structural Complexity

### The Role of Doubly-Linked Lists

```

The choice of **Doubly-Linked Lists** (`dl_list_t`) for both the Segregated Free Lists and the Allocated Blocks List is fundamental to the project's complexity and efficiency:

* **Fast Deletion:** DLLs allow for $O(1)$ removal of a node once its location is known, which is crucial for `MALLOC` (removing a free block) and `FREE` (removing an allocated block).
* **Ordered Insertion (Address-Based):**
    * In the **Allocated List** (`mem_alloc`), insertion is $O(N)$ (where $N$ is the number of allocated blocks) to maintain strict ascending address order. This order is essential for the $O(N)$ memory contiguity checks required by `READ` and `WRITE`.
    * In the **Free Lists** (`sfl.memory[i]`), maintaining ascending address order facilitates potential memory coalescing (merging adjacent free blocks) when blocks are returned by `FREE`.

### Dynamic SFL Vector Management

The `sfl` structure implements a highly dynamic system:

* The **`sfl.memory`** array can be **resized dynamically** using `realloc` during a `MALLOC` fragmentation or a `FREE` operation if a block of a new, unexpected size is encountered.
* After resizing, the entire vector of lists and the `sfl.capacity` array must be **re-sorted**. This $O(L^2)$ sort (where $L$ is the number of segregated lists) is performed to ensure that the `MALLOC` **Best-Fit** search can quickly iterate through lists in ascending order of block size. This complexity trade-off prioritizes quick heap operations (`malloc/free`) over less frequent structural updates.