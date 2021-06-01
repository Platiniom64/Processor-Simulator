/*************************************************************************************|
|   1. YOU ARE NOT ALLOWED TO SHARE/PUBLISH YOUR CODE (e.g., post on piazza or online)|
|   2. Fill memory_hierarchy.c                                                        |
|   3. Do not use any other .c files neither alter mipssim.h or parser.h              |
|   4. Do not include any other library files                                         |
|*************************************************************************************/

#include "mipssim.h"

// * blogbal variables
#define CACHE_BLOCK_SIZE 16 // bytes
#define NUM_BITS_IN_WORD 32


// * structure for a single row of the cache
//* COMPLETE
struct cache_row {

    int valid;
    int tag;
    // ! you store pointers to words not indididual bytes
    int *data_block_pt;

};


// * ------ HELPER METHODS FOR LRU linked lists ------


//* COMPLETE
struct node {
    int index_row;
    struct node* next;
};
typedef struct node node_t;

//* COMPLETE
struct Linked_list {
    node_t* head;
    node_t* last;
};
typedef struct Linked_list linkedlist;


//* COMPLETE
void print_content_list(linkedlist* list) {
    // if list is empty
    if (list->head == NULL && list->last == NULL) {
        printf("the list is empty\n");
    }

    node_t* temp = list->head;

    while(temp != NULL){
        printf("%d - ", temp->index_row);
        temp = temp->next;
    }
    printf("\n");
}

//* COMPLETE
node_t* create_node(int value_index) {

    node_t* pt = (node_t *) malloc(sizeof(node_t));
    pt->index_row = value_index;
    pt->next = NULL;
    return pt;
}

//* COMPLETE
void add_node(linkedlist* list, int value_node) {
    // if list is empty
    if (list->head == NULL && list->last == NULL) { // technically they must be at the same time
        node_t* new_node = create_node(value_node);
        list->head = new_node;
        list->last = new_node;

        return;
    }

    // if the linked list already has elements
    node_t* new_node = create_node(value_node);

    new_node->next = list->head;
    list->head = new_node;
}

//* COMPLETE
void delete_node(linkedlist* list, int value_node) {

    // * check if the given list is not empty
    if (list->head == NULL && list->last == NULL) {
        printf("could not remove node as list is empty\n");
        return;
    }

    // * check if there is only one element
    if (list->head == list->last) {
        if (list->head->index_row == value_node) {
            node_t* tofree = list->head;
            list->head = NULL;
            list->last = NULL;
            free(tofree);
            return;
        }
        printf("value to remove not in the list\n");
        return;
    }

    // * not it has at least two nodes
    node_t* temp = list->head;

    // in the case that we want to remove the first item of the list
    if (temp->index_row == value_node) {
        
        list->head = temp->next;
        free(temp);
        return;
    }

    // could remove the second to second to last node
    while (temp->next->next != NULL) {

        if (temp->next->index_row == value_node) {
            node_t* tofree = temp->next;
            temp->next = temp->next->next;
            free(tofree);
            return;
        }
        temp = temp->next;
    }

    // if this is executed you haven't found the value or it is the last one
    //if it is the last one
    if (list->last->index_row == value_node) {
        // you still have the pointer to the second last node
        node_t* tofree = list->last;
        list->last = temp;
        temp->next = NULL;
        free(tofree);
        return;
    }

    // at this point you haven't found the value
    printf("have not found value to remove in the list\n");

}

//* COMPLETE
int get_last_value(linkedlist* list) {
    if (list->head == NULL && list->last == NULL) {
        printf("the list is empty, there is no last value\n");
        return -1;
    }
    return list->last->index_row;
}




// * global variables for the cache
struct cache_row *cache_pt; // pointer to all the rows of the cache
linkedlist *LRU_pt; // pointer to the LRS linked lists

int num_bits_offet;
int num_cache_rows;
int num_bits_index;
int num_bits_tag;
int num_cache_sets;
int num_row_per_set;




//* --- helper methods for the main methods under

// * this method prints the content of the cache (valid bit and everything)
//* COMPLETE
void print_content_cache() {
    
    // iterate through each cache row
    for (struct cache_row *curr_cache_row = cache_pt; curr_cache_row < cache_pt + num_cache_rows; curr_cache_row++) {
        
        printf("V: %d, ", curr_cache_row->valid);
        printf("tag: %d, ", curr_cache_row->tag);

        // print data from the data block
        printf("data: ");
        for (int* curr_word_pt = curr_cache_row->data_block_pt; curr_word_pt < curr_cache_row->data_block_pt + (CACHE_BLOCK_SIZE / 4); curr_word_pt++) {
            printf("%d ", *curr_word_pt);
        }

        printf("\n");
    }

}

// * this is called when we have a cache miss, so we load the whole row
//* COMPLETE
void load_cache_row(int addr_mem_first_word, int* where_to_write_cache) {
    
    for (int times = 0; times < CACHE_BLOCK_SIZE/4; times++) {
        *where_to_write_cache = arch_state.memory[addr_mem_first_word / 4];

        where_to_write_cache++;
        addr_mem_first_word = addr_mem_first_word + 4;

    }
} 

// * this is called whenever we access memory and we found the data, so we set the given row as most recenetly used
//* COMPLETE
void update_LRU(int index_addr, int row_num) {

    // * firt we need to find the right linked list
    linkedlist *list_pt = LRU_pt + index_addr;

    // * in the case that the given row was already used in the past, it is already in the list, so we need to remove it
    delete_node(list_pt, row_num);

    // * then add it to the front
    add_node(list_pt, row_num);
}

//* COMPLETE
int get_num_row_to_evict(int index_addr) {

    // * firt we need to find the right linked list
    linkedlist *list_pt = LRU_pt + index_addr;

    int last_value = get_last_value(list_pt);

    if (last_value == -1) {
        printf("you asked to evict a row when the row is free");
        assert(0);
    }

    update_LRU(index_addr, last_value); // because now that row is the most recently used

    return last_value;
}





//* ------------------------- CACHE INIT -------------------------

// * initialse the directly mapped cache
//* COMPLETE - last part is common
void initialise_direct_cache(struct architectural_state *arch_state_ptr) {

    //* ONLY FOR DIRECTLY MAPPED CACHE
    num_bits_offet = log(CACHE_BLOCK_SIZE) / log(2);
    num_cache_rows = cache_size / CACHE_BLOCK_SIZE;
    num_bits_index = log(num_cache_rows) / log(2);
    num_cache_sets = num_cache_rows;
    num_bits_tag = NUM_BITS_IN_WORD - num_bits_index - num_bits_offet;
    num_row_per_set = 1;

    //* common to the three caches:

    memory_stats_init(arch_state_ptr, num_bits_tag);

    // * initialise content of the cache
    cache_pt = (struct cache_row *) malloc(sizeof(struct cache_row) * num_cache_rows);
    // clean the memory received above
    memset(cache_pt, 0, sizeof(struct cache_row) * num_cache_rows);

    //* initialise the data block of each row
    for (struct cache_row *curr_cache_row = cache_pt; curr_cache_row < cache_pt + num_cache_rows; curr_cache_row++) {
        curr_cache_row->data_block_pt = (int *) malloc( sizeof(int) * (CACHE_BLOCK_SIZE / 4) );
        // clean the memory received above
        memset(curr_cache_row->data_block_pt, 0, sizeof(int) * (CACHE_BLOCK_SIZE / 4));
    }

    // * initialise the LRU linked lists
    LRU_pt = (linkedlist *) malloc(sizeof(linkedlist) * num_cache_sets);

    //* intialise the content of each list
    for (linkedlist *curr_list = LRU_pt; curr_list < LRU_pt + num_cache_sets; curr_list++) {
        curr_list->head = NULL;
        curr_list->last = NULL;
    }
}

// * initiallise the fully associative cache
//* COMPLETE - last part is common
void initialise_associative_cache(struct architectural_state *arch_state_ptr) {

    //* ONLY FOR THIS TYPE OF CACHE
    num_bits_offet = log(CACHE_BLOCK_SIZE) / log(2);
    num_cache_rows = cache_size / CACHE_BLOCK_SIZE;
    num_cache_sets = 1;
    num_bits_index = 0;
    num_bits_tag = NUM_BITS_IN_WORD - num_bits_offet;
    num_row_per_set = num_cache_rows;

    //* common to the three caches:

    memory_stats_init(arch_state_ptr, num_bits_tag);

    // * initialise content of the cache
    cache_pt = (struct cache_row *) malloc(sizeof(struct cache_row) * num_cache_rows);
    // clean the memory received above
    memset(cache_pt, 0, sizeof(struct cache_row) * num_cache_rows);

    //* initialise the data block of each row
    for (struct cache_row *curr_cache_row = cache_pt; curr_cache_row < cache_pt + num_cache_rows; curr_cache_row++) {
        curr_cache_row->data_block_pt = (int *) malloc( sizeof(int) * (CACHE_BLOCK_SIZE / 4) );
        // clean the memory received above
        memset(curr_cache_row->data_block_pt, 0, sizeof(int) * (CACHE_BLOCK_SIZE / 4));
    }

    // * initialise the LRU linked lists
    LRU_pt = (linkedlist *) malloc(sizeof(linkedlist) * num_cache_sets);

    //* intialise the content of each list
    for (linkedlist *curr_list = LRU_pt; curr_list < LRU_pt + num_cache_sets; curr_list++) {
        curr_list->head = NULL;
        curr_list->last = NULL;
    }
}

// * initiallise the set associative cache
//* COMPLETE - last part is common
void initialise_set_associative_cache(struct architectural_state *arch_state_ptr) {

    //* ONLY FOR SET ASSOCIATIVE CACHE
    num_row_per_set = 2; // defined that way
    num_bits_offet = log(CACHE_BLOCK_SIZE) / log(2);
    num_cache_rows = cache_size / CACHE_BLOCK_SIZE;
    num_cache_sets = num_cache_rows / num_row_per_set;
    num_bits_index = log(num_cache_sets) / log(2);
    num_bits_tag = NUM_BITS_IN_WORD - num_bits_index - num_bits_offet;
    
    //* common to the three caches:

    memory_stats_init(arch_state_ptr, num_bits_tag);

    // * initialise content of the cache
    cache_pt = (struct cache_row *) malloc(sizeof(struct cache_row) * num_cache_rows);
    // clean the memory received above
    memset(cache_pt, 0, sizeof(struct cache_row) * num_cache_rows);

    //* initialise the data block of each row
    for (struct cache_row *curr_cache_row = cache_pt; curr_cache_row < cache_pt + num_cache_rows; curr_cache_row++) {
        curr_cache_row->data_block_pt = (int *) malloc( sizeof(int) * (CACHE_BLOCK_SIZE / 4) );
        // clean the memory received above
        memset(curr_cache_row->data_block_pt, 0, sizeof(int) * (CACHE_BLOCK_SIZE / 4));
    }

    // * initialise the LRU linked lists
    LRU_pt = (linkedlist *) malloc(sizeof(linkedlist) * num_cache_sets);

    //* intialise the content of each list
    for (linkedlist *curr_list = LRU_pt; curr_list < LRU_pt + num_cache_sets; curr_list++) {
        curr_list->head = NULL;
        curr_list->last = NULL;
    }
}

//* this method is called at the start in order to initialise the cache
//* COMPLETE
uint32_t cache_type = 0;

void memory_state_init(struct architectural_state *arch_state_ptr) {
    arch_state_ptr->memory = (uint32_t *) malloc(sizeof(uint32_t) * MEMORY_WORD_NUM);
    memset(arch_state_ptr->memory, 0, sizeof(uint32_t) * MEMORY_WORD_NUM);

    // * cache disabled
    if (cache_size == 0) {
        memory_stats_init(arch_state_ptr, 0); // WARNING: we initialize for no cache 0
    }
    
    // * cache enabled
    else {
        
        /// @students: memory_stats_init(arch_state_ptr, X); <-- fill # of tag bits for cache 'X' correctly
        
        switch(cache_type) {
            case CACHE_TYPE_DIRECT: // direct mapped
                initialise_direct_cache(arch_state_ptr);
                break;

            case CACHE_TYPE_FULLY_ASSOC: // fully associative
                initialise_associative_cache(arch_state_ptr);
                break;

            case CACHE_TYPE_2_WAY: // 2-way associative
                initialise_set_associative_cache(arch_state_ptr);
                break;
        }
    }
}






//* ------------------------- MEMORY READ -------------------------
//* COMPLETE

// * this method should return the data at address, of a DIRECT CACHE
// * if the cache does not contain it, then we can ask the memory to read it
//* COMPLERE -- common to the three types of caches
int read_direct_cache(int address) {
    
    // * get necessary information from the address in order to write to cache
    int byte_offset_addr = get_piece_of_a_word(address, 0, num_bits_offet);
    int index_addr = get_piece_of_a_word(address, num_bits_offet, num_bits_index);
    int tag_addr = get_piece_of_a_word(address, num_bits_offet+num_bits_index, num_bits_tag);

    // * we first have to find which set we want to go to (find the address of the first row in cache of the corresponding set)
    // for fully associative, as index_addr is 0 it is simply the first row in the cache
    // for directly mapped cache, index_addr is the index of the single row
    // for set associative it is the first row in the correct set
    struct cache_row *first_row_set = cache_pt + (index_addr * num_row_per_set);

    // * now we try to find the tag at given address in the set
    // iterate through rows of set
    int row_num = 0; // to know in the LRU policy which line we found

    for (struct cache_row *curr_row = first_row_set; curr_row < first_row_set + num_row_per_set; curr_row++) {

        // check that valid bit it 1
        if (curr_row->valid == 1) {

            // check that the tag is the same
            if (curr_row->tag == tag_addr) {
                
                // * YOU FOUND THE DATA IN THE CACHE
                printf("chache hit on read! \n");

                // * stats
                arch_state.mem_stats.lw_cache_hits++;

                // * update LRU policy
                update_LRU(index_addr, row_num);

                // * return data found in cache
                int index_word_in_data_block = byte_offset_addr / 4;
                int *read_data_pt = curr_row->data_block_pt + index_word_in_data_block;

                return (int) *read_data_pt; // return the read data
            }
        }

        row_num++;
    }

    // * at this point if this is executed then the cache did not deliver a hit
    printf("chache miss on read. address requested: %d \n", address);

    // * first we load the data in a free row of the current set
    // so we try to find a row that is empty so we can load it there
    // inside the given set, we need to find a row that had a valid bit set to zero

    // iterate through rows of set
    row_num = 0; // to know in the LRU policy which line we found

    for (struct cache_row *curr_row = first_row_set; curr_row < first_row_set + num_row_per_set; curr_row++) {

        // check that valid bit it 0 to write to that row
        if (curr_row->valid == 0) {
            
            //* found a row in the correct set that is free
            int addr_mem_first_word = address - byte_offset_addr;
            load_cache_row(addr_mem_first_word, curr_row->data_block_pt);

            //* update LRU policy
            update_LRU(index_addr, row_num);

            //* update the tag of the row of the cache
            curr_row->tag = tag_addr;

            //* update the valid bit
            curr_row->valid = 1;

            // then just return the data that must have been read (which is also the one found in memory)
            return (int) arch_state.memory[address / 4];
        }

        row_num++;
    }

    //* at this point we did not find the data in cache, and there is no free row in the cache
    // hence we need to use the LRU policy to tell us which row to evict
    int num_row_to_evict = get_num_row_to_evict(index_addr); // this also does the LRU upate

    // now we need to load the data at that row from the set
    struct cache_row *row_to_evict_pt = first_row_set + num_row_to_evict;
    int *where_to_write_cache = row_to_evict_pt->data_block_pt;
    int addr_mem_first_word = address - byte_offset_addr;
    load_cache_row(addr_mem_first_word, where_to_write_cache);

    // * update the tag of the row of the cache
    row_to_evict_pt->tag = tag_addr;

    // * update the valid bit
    row_to_evict_pt->valid = 1;

    // then just return the data that must have been read (which is also the one found in memory)
    return (int) arch_state.memory[address / 4];
}

// * this method should return the data at address, of a FULLY ASSOCIATIVE CACHE
// * if the cache does not contain it, then we can ask the memory to read it and load the cache too
//* COMPLERE -- common to the three types of caches
int read_associative_cache(int address) {
    
    // * get necessary information from the address in order to write to cache
    int byte_offset_addr = get_piece_of_a_word(address, 0, num_bits_offet);
    int index_addr = get_piece_of_a_word(address, num_bits_offet, num_bits_index);
    int tag_addr = get_piece_of_a_word(address, num_bits_offet+num_bits_index, num_bits_tag);

    // * we first have to find which set we want to go to (find the address of the first row in cache of the corresponding set)
    // for fully associative, as index_addr is 0 it is simply the first row in the cache
    // for directly mapped cache, index_addr is the index of the single row
    // for set associative it is the first row in the correct set
    struct cache_row *first_row_set = cache_pt + (index_addr * num_row_per_set);

    // * now we try to find the tag at given address in the set
    // iterate through rows of set
    int row_num = 0; // to know in the LRU policy which line we found

    for (struct cache_row *curr_row = first_row_set; curr_row < first_row_set + num_row_per_set; curr_row++) {

        // check that valid bit it 1
        if (curr_row->valid == 1) {

            // check that the tag is the same
            if (curr_row->tag == tag_addr) {
                
                // * YOU FOUND THE DATA IN THE CACHE
                printf("chache hit on read! \n");

                // * stats
                arch_state.mem_stats.lw_cache_hits++;

                // * update LRU policy
                update_LRU(index_addr, row_num);

                // * return data found in cache
                int index_word_in_data_block = byte_offset_addr / 4;
                int *read_data_pt = curr_row->data_block_pt + index_word_in_data_block;

                return (int) *read_data_pt; // return the read data
            }
        }

        row_num++;
    }

    // * at this point if this is executed then the cache did not deliver a hit
    printf("chache miss on read. address requested: %d \n", address);

    // * first we load the data in a free row of the current set
    // so we try to find a row that is empty so we can load it there
    // inside the given set, we need to find a row that had a valid bit set to zero

    // iterate through rows of set
    row_num = 0; // to know in the LRU policy which line we found

    for (struct cache_row *curr_row = first_row_set; curr_row < first_row_set + num_row_per_set; curr_row++) {

        // check that valid bit it 0 to write to that row
        if (curr_row->valid == 0) {
            
            //* found a row in the correct set that is free
            int addr_mem_first_word = address - byte_offset_addr;
            load_cache_row(addr_mem_first_word, curr_row->data_block_pt);

            //* update LRU policy
            update_LRU(index_addr, row_num);

            //* update the tag of the row of the cache
            curr_row->tag = tag_addr;

            //* update the valid bit
            curr_row->valid = 1;

            // then just return the data that must have been read (which is also the one found in memory)
            return (int) arch_state.memory[address / 4];
        }

        row_num++;
    }

    //* at this point we did not find the data in cache, and there is no free row in the cache
    // hence we need to use the LRU policy to tell us which row to evict
    int num_row_to_evict = get_num_row_to_evict(index_addr); // this also does the LRU upate

    // now we need to load the data at that row from the set
    struct cache_row *row_to_evict_pt = first_row_set + num_row_to_evict;
    int *where_to_write_cache = row_to_evict_pt->data_block_pt;
    int addr_mem_first_word = address - byte_offset_addr;
    load_cache_row(addr_mem_first_word, where_to_write_cache);

    // * update the tag of the row of the cache
    row_to_evict_pt->tag = tag_addr;

    // * update the valid bit
    row_to_evict_pt->valid = 1;

    // then just return the data that must have been read (which is also the one found in memory)
    return (int) arch_state.memory[address / 4];
}

// * this method should return the data at address, of a SET ASSOCIATIVE CACHE
// * if the cache does not contain it, then we can ask the memory to read it
//* COMPLERE -- common to the three types of caches
int read_set_associative_cache(int address) {
    // * get necessary information from the address in order to write to cache
    int byte_offset_addr = get_piece_of_a_word(address, 0, num_bits_offet);
    int index_addr = get_piece_of_a_word(address, num_bits_offet, num_bits_index);
    int tag_addr = get_piece_of_a_word(address, num_bits_offet+num_bits_index, num_bits_tag);

    // * we first have to find which set we want to go to (find the address of the first row in cache of the corresponding set)
    // for fully associative, as index_addr is 0 it is simply the first row in the cache
    // for directly mapped cache, index_addr is the index of the single row
    // for set associative it is the first row in the correct set
    struct cache_row *first_row_set = cache_pt + (index_addr * num_row_per_set);

    // * now we try to find the tag at given address in the set
    // iterate through rows of set
    int row_num = 0; // to know in the LRU policy which line we found

    for (struct cache_row *curr_row = first_row_set; curr_row < first_row_set + num_row_per_set; curr_row++) {

        // check that valid bit it 1
        if (curr_row->valid == 1) {

            // check that the tag is the same
            if (curr_row->tag == tag_addr) {
                
                // * YOU FOUND THE DATA IN THE CACHE
                printf("chache hit on read! \n");

                // * stats
                arch_state.mem_stats.lw_cache_hits++;

                // * update LRU policy
                update_LRU(index_addr, row_num);

                // * return data found in cache
                int index_word_in_data_block = byte_offset_addr / 4;
                int *read_data_pt = curr_row->data_block_pt + index_word_in_data_block;

                return (int) *read_data_pt; // return the read data
            }
        }

        row_num++;
    }

    // * at this point if this is executed then the cache did not deliver a hit
    printf("chache miss on read. address requested: %d \n", address);

    // * first we load the data in a free row of the current set
    // so we try to find a row that is empty so we can load it there
    // inside the given set, we need to find a row that had a valid bit set to zero

    // iterate through rows of set
    row_num = 0; // to know in the LRU policy which line we found

    for (struct cache_row *curr_row = first_row_set; curr_row < first_row_set + num_row_per_set; curr_row++) {

        // check that valid bit it 0 to write to that row
        if (curr_row->valid == 0) {
            
            //* found a row in the correct set that is free
            int addr_mem_first_word = address - byte_offset_addr;
            load_cache_row(addr_mem_first_word, curr_row->data_block_pt);

            //* update LRU policy
            update_LRU(index_addr, row_num);

            //* update the tag of the row of the cache
            curr_row->tag = tag_addr;

            //* update the valid bit
            curr_row->valid = 1;

            // then just return the data that must have been read (which is also the one found in memory)
            return (int) arch_state.memory[address / 4];
        }

        row_num++;
    }

    //* at this point we did not find the data in cache, and there is no free row in the cache
    // hence we need to use the LRU policy to tell us which row to evict
    int num_row_to_evict = get_num_row_to_evict(index_addr); // this also does the LRU upate

    // now we need to load the data at that row from the set
    struct cache_row *row_to_evict_pt = first_row_set + num_row_to_evict;
    int *where_to_write_cache = row_to_evict_pt->data_block_pt;
    int addr_mem_first_word = address - byte_offset_addr;
    load_cache_row(addr_mem_first_word, where_to_write_cache);

    // * update the tag of the row of the cache
    row_to_evict_pt->tag = tag_addr;

    // * update the valid bit
    row_to_evict_pt->valid = 1;

    // then just return the data that must have been read (which is also the one found in memory)
    return (int) arch_state.memory[address / 4];
}


//* reads data from memory or chache if possible
//* returns data on memory[address / 4]
//* COMPLETE
int memory_read(int address){

    arch_state.mem_stats.lw_total++;
    check_address_is_word_aligned(address);

    // * cache disabled
    if (cache_size == 0) {
        return (int) arch_state.memory[address / 4];

    // * cache inabled
    } else {
        
        /// @students: your implementation must properly increment: arch_state_ptr->mem_stats.lw_cache_hits
        
        switch(cache_type) {

            case CACHE_TYPE_DIRECT: // direct mapped
                return read_direct_cache(address);
                break;

            case CACHE_TYPE_FULLY_ASSOC: // fully associative
                return read_associative_cache(address);
                break;

            case CACHE_TYPE_2_WAY: // 2-way associative
                return read_set_associative_cache(address);
                break;
            
            default:
                printf("cache type not implemented for reading\n");
                assert(0);
        }
    }

    assert(0); // this should never happen
}





//* ------------------------- MEMORY WRITE -------------------------
//* COMPLETE

//* this write to the DIRECT CACHE and to memory if there is a miss
//* COMPLERE -- common to the three types of caches
void write_direct_cache(int address, int write_data) {

    // * get necessary information from the address in order to write to cache
    int byte_offset_addr = get_piece_of_a_word(address, 0, num_bits_offet);
    int index_addr = get_piece_of_a_word(address, num_bits_offet, num_bits_index);
    int tag_addr = get_piece_of_a_word(address, num_bits_offet+num_bits_index, num_bits_tag);

    // * we first have to find which set we want to go to (find the address of the first row in cache of the corresponding set)
    // for fully associative, as index_addr is 0 it is simply the first row in the cache
    // for directly mapped cache, index_addr is the index of the single row
    // for set associative it is the first row in the correct set
    struct cache_row *first_row_set = cache_pt + (index_addr * num_row_per_set);


    // * now we try to find the tag in the set
    int row_num = 0; // remember which row you are at in order to be able to update the LRU policy

    // iterate through rows of set
    for (struct cache_row *curr_row = first_row_set; curr_row < first_row_set + num_row_per_set; curr_row++) {

        // check that valid bit it 1
        if (curr_row->valid == 1) {

            // check that the tag is the same
            if (curr_row->tag == tag_addr) {
                
                //* CACHE HIT on write
                printf("chache hit on write! \n");

                //* stats for memory
                arch_state.mem_stats.sw_cache_hits++;

                //* update LRU policy
                update_LRU(index_addr, row_num);

                //* write data in the cache and memory as we have a write through
                // have to convert as the struct for row is words not bytes
                int index_word_in_data_block = byte_offset_addr / 4;
                int *read_data_pt = curr_row->data_block_pt + index_word_in_data_block;

                // this is where you write in the cache
                *read_data_pt = (uint32_t) write_data; 

                // as we have write through, we also need to write to memory
                arch_state.memory[address / 4] = (uint32_t) write_data;

                return; // you want to exit so that if you normally exit the loop you know there was not cache hit

            }
        }

        row_num++;
    }

    //* at this point if this is executed then the cache did not deliver a hit
    printf("chache miss on write. address requested: %d \n", address);

    //* as we have a write-no-allocate we don't bring the block in the cache, we only write to memory
    arch_state.memory[address / 4] = (uint32_t) write_data;
}

//* this write to the FULLY ASSOCIATIVE CACHE and to memory if there is a miss
//* COMPLERE -- common to the three types of caches
void write_associative_cache(int address, int write_data) {

    // * get necessary information from the address in order to write to cache
    int byte_offset_addr = get_piece_of_a_word(address, 0, num_bits_offet);
    int index_addr = get_piece_of_a_word(address, num_bits_offet, num_bits_index);
    int tag_addr = get_piece_of_a_word(address, num_bits_offet+num_bits_index, num_bits_tag);

    // * we first have to find which set we want to go to (find the address of the first row in cache of the corresponding set)
    // for fully associative, as index_addr is 0 it is simply the first row in the cache
    // for directly mapped cache, index_addr is the index of the single row
    // for set associative it is the first row in the correct set
    struct cache_row *first_row_set = cache_pt + (index_addr * num_row_per_set);


    // * now we try to find the tag in the set
    int row_num = 0; // remember which row you are at in order to be able to update the LRU policy

    // iterate through rows of set
    for (struct cache_row *curr_row = first_row_set; curr_row < first_row_set + num_row_per_set; curr_row++) {

        // check that valid bit it 1
        if (curr_row->valid == 1) {

            // check that the tag is the same
            if (curr_row->tag == tag_addr) {
                
                //* CACHE HIT on write
                printf("chache hit on write! \n");

                //* stats for memory
                arch_state.mem_stats.sw_cache_hits++;

                //* update LRU policy
                update_LRU(index_addr, row_num);

                //* write data in the cache and memory as we have a write through
                // have to convert as the struct for row is words not bytes
                int index_word_in_data_block = byte_offset_addr / 4;
                int *read_data_pt = curr_row->data_block_pt + index_word_in_data_block;

                // this is where you write in the cache
                *read_data_pt = (uint32_t) write_data; 

                // as we have write through, we also need to write to memory
                arch_state.memory[address / 4] = (uint32_t) write_data;

                return; // you want to exit so that if you normally exit the loop you know there was not cache hit

            }
        }

        row_num++;
    }

    //* at this point if this is executed then the cache did not deliver a hit
    printf("chache miss on write. address requested: %d \n", address);

    //* as we have a write-no-allocate we don't bring the block in the cache, we only write to memory
    arch_state.memory[address / 4] = (uint32_t) write_data;

}

//* this write to the SET ASSOCIATIVE CACHE and to memory if there is a miss
//* COMPLERE -- common to the three types of caches
void write_set_associative_cache(int address, int write_data) {

    // * get necessary information from the address in order to write to cache
    int byte_offset_addr = get_piece_of_a_word(address, 0, num_bits_offet);
    int index_addr = get_piece_of_a_word(address, num_bits_offet, num_bits_index);
    int tag_addr = get_piece_of_a_word(address, num_bits_offet+num_bits_index, num_bits_tag);

    // * we first have to find which set we want to go to (find the address of the first row in cache of the corresponding set)
    // for fully associative, as index_addr is 0 it is simply the first row in the cache
    // for directly mapped cache, index_addr is the index of the single row
    // for set associative it is the first row in the correct set
    struct cache_row *first_row_set = cache_pt + (index_addr * num_row_per_set);


    // * now we try to find the tag in the set
    int row_num = 0; // remember which row you are at in order to be able to update the LRU policy

    // iterate through rows of set
    for (struct cache_row *curr_row = first_row_set; curr_row < first_row_set + num_row_per_set; curr_row++) {

        // check that valid bit it 1
        if (curr_row->valid == 1) {

            // check that the tag is the same
            if (curr_row->tag == tag_addr) {
                
                //* CACHE HIT on write
                printf("chache hit on write! \n");

                //* stats for memory
                arch_state.mem_stats.sw_cache_hits++;

                //* update LRU policy
                update_LRU(index_addr, row_num);

                //* write data in the cache and memory as we have a write through
                // have to convert as the struct for row is words not bytes
                int index_word_in_data_block = byte_offset_addr / 4;
                int *read_data_pt = curr_row->data_block_pt + index_word_in_data_block;

                // this is where you write in the cache
                *read_data_pt = (uint32_t) write_data; 

                // as we have write through, we also need to write to memory
                arch_state.memory[address / 4] = (uint32_t) write_data;

                return; // you want to exit so that if you normally exit the loop you know there was not cache hit

            }
        }

        row_num++;
    }

    //* at this point if this is executed then the cache did not deliver a hit
    printf("chache miss on write. address requested: %d \n", address);

    //* as we have a write-no-allocate we don't bring the block in the cache, we only write to memory
    arch_state.memory[address / 4] = (uint32_t) write_data;

}


//* this is executed when the processor writes data on memory[address / 4] or cahce if possible
// * COMPLETED
void memory_write(int address, int write_data) {
    arch_state.mem_stats.sw_total++;
    check_address_is_word_aligned(address);

    // * cache disabled
    if (cache_size == 0) {
        // just write to memory
        arch_state.memory[address / 4] = (uint32_t) write_data;

    // * cache enabled
    } else {
        
        /// @students: your implementation must properly increment: arch_state_ptr->mem_stats.sw_cache_hits
        // * COMPLETED THE THING ABOVE
        
        switch(cache_type) {
            case CACHE_TYPE_DIRECT: // direct mapped
                write_direct_cache(address, write_data);
                break;

            case CACHE_TYPE_FULLY_ASSOC: // fully associative
                write_associative_cache(address, write_data);
                break;

            case CACHE_TYPE_2_WAY: // 2-way associative
                write_set_associative_cache(address, write_data);
                break;
            
            default:
                printf("trying to write with type of cache not implemented \n");
                assert(0);
        }
    }
}
