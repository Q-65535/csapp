#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

int set_index_bit_count;
int set_count;
int line_count;
int offset_bit_count;
char file_path[1000];
int help;
int verbose;
int hit_count;
int miss_count;
int eviction_count;

typedef unsigned long uint64_t;
typedef struct cache_line {
    int valid;
    int tag;
} cache_line;

typedef struct LRU_queue_node {
    struct LRU_queue_node *pre;
    struct LRU_queue_node *next;
    int tag;
} LRU_queue_node;

// each set has its own LRU queue. Each queue is identified by the dummy head and tail of the queue.
typedef struct LRU_queue {
    LRU_queue_node *dummy_head;
    LRU_queue_node *dummy_tail;
} LRU_queue;

// cache is just a 2D arry of cache lines
cache_line** cache = NULL;
// an array of queues, each set has its own LRU queue.
LRU_queue* queues;

// function declarations
void init_cache();
void init_LRU_queue();
void update(uint64_t addr);
void LRU_queue_hit(uint64_t addr);
void LRU_queue_evict(int set_index);
void LRU_queue_enque(uint64_t addr);
void LRU_queue_remove(uint64_t addr);
int get_LRU_tag(int set_index);
int get_tag(uint64_t addr);
int get_set_index(uint64_t addr);
void parse_trace();

int main(int args, char** argv) {
    int opt;
    while (-1 != (opt = getopt(args, argv, "hvs:E:b:t:"))) {
        switch (opt) {
            case 's':
                set_index_bit_count = atoi(optarg);
                break;
            case 'E':
                line_count = atoi(optarg);
                break;
            case 'b':
                offset_bit_count = atoi(optarg);
                break;
            case 't':
                strcpy(file_path, optarg);
                break;
            case 'h':
                help = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                printf("wrong arguments\n");
                break;
        }
    }
    set_count = 1 << set_index_bit_count;
    init_cache();
    init_LRU_queue();

    parse_trace();
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}

void init_cache() {
    cache = malloc(sizeof(cache_line*) * set_count);
    for (int i = 0; i < set_count; i++) {
        cache[i] = malloc(sizeof(cache_line) * line_count);
        for (int j = 0; j < line_count; j++) {
            cache[i][j].valid = 0;
            cache[i][j].tag = 0;
        }
    }
    init_LRU_queue();
}

void init_LRU_queue() {
    queues = malloc(sizeof(LRU_queue) * set_count);
    for (int i = 0; i < set_count; i++) {
        // For each queue, generate dummy head and tail pointers
        queues[i].dummy_head = malloc(sizeof(LRU_queue_node));
        queues[i].dummy_tail = malloc(sizeof(LRU_queue_node));
        queues[i].dummy_head->next = queues[i].dummy_tail;
        queues[i].dummy_tail->pre = queues[i].dummy_head;
    }
}

void parse_trace() {
    FILE* fp = fopen(file_path, "r");
    if (fp == NULL) {
        printf("open error \n");
        exit(-1);
    }

    char operation;
    uint64_t addr;
    int size;
    while (fscanf(fp, "%c %lx,%d\n", &operation, &addr, &size) > 0) {
        // ignore instuction fetch
        if (operation == 'I') {
            continue;
        }
        if (operation == 'L') {
            update(addr);
        }
        // modify requires load and store operation, so it needs to update twice
        if (operation == 'M') {
            update(addr);
            update(addr);
        }
        if (operation == 'S') {
            update(addr);
        }
    }
}

void update(uint64_t addr) {
    int set_index = get_set_index(addr);
    int tag = get_tag(addr);
    // cache hit
    for (int i = 0; i < line_count; i++) {
        if (cache[set_index][i].tag == tag && cache[set_index][i].valid == 1) {
            LRU_queue_hit(addr);
            hit_count++;
            return;
        }
    }
    // cache miss
    miss_count++;
    // cold start miss: if there is empty slot, occupy it
    for (int i = 0; i < line_count; i++) {
        if (cache[set_index][i].valid == 0) {
            cache[set_index][i].valid = 1;
            cache[set_index][i].tag = tag;
            LRU_queue_enque(addr);
            return;
        }
    }
    // capacity miss: evict the LRU slot in this set and occupy it
    eviction_count++;
    int LRU_tag = get_LRU_tag(set_index);
    for (int i = 0; i < line_count; i++) {
        if (cache[set_index][i].tag == LRU_tag) {
            LRU_queue_evict(set_index);
            // change the tag of LRU cache line to the given tag
            cache[set_index][i].tag = tag;
            LRU_queue_enque(addr);
            return;
        }
    }
}

void LRU_queue_hit(uint64_t addr) {
    LRU_queue_remove(addr);
    LRU_queue_enque(addr);
}

void LRU_queue_remove(uint64_t addr) {
    int set_index = get_set_index(addr);
    int tag = get_tag(addr);
    LRU_queue selected_queue = queues[set_index];
    // @Care: be really care about how to find a target node in a linked list!
    // The while condition and the initial current node should be carefully set.
    // Also, when to update the current node in the while loop requires careful thinking: at
    // the beginning of the while or at the end of the while?
    LRU_queue_node* cur = selected_queue.dummy_head;
    while (cur->next != NULL && cur->next->next != NULL) {
        cur = cur->next;
        if (cur->tag == tag) {
            LRU_queue_node *pre = cur->pre;
            pre->next = cur->next;
            cur->next->pre = pre;
            // remember to free the node
            free(cur);
            return;
        }
    }
    printf("error: the tag %d is not found in the queue!", tag);
    exit(0);
}

/*
 * create a new node and enqueue it into the queue indicated by the given address.
 */
void LRU_queue_enque(uint64_t addr) {
    int set_index = get_set_index(addr);
    int tag = get_tag(addr);
    // create the new node to be enqueed
    LRU_queue_node *new_node = malloc(sizeof(LRU_queue_node));
    new_node->tag = tag;
    // enque operation
    LRU_queue selected_queue = queues[set_index];
    LRU_queue_node *next = selected_queue.dummy_head->next;
    new_node->pre = selected_queue.dummy_head;
    selected_queue.dummy_head->next = new_node;
    new_node->next = next;
    next->pre = new_node;
}

// @Incomplete: we assume that the #bits required to represent tag, set index and block offset is less
// than 32, so we can use int to store it. We can pass the test because the test cases conform this assumption.
/*
 * Given an 64-bit address return its set index value. address layout: ||---tag---|---set index---|---block offset---||
 */
int get_set_index(uint64_t addr) {
    int mask = (1 << set_index_bit_count) - 1;
    return (addr >> offset_bit_count) & mask;
}

/*
 * Given an 64-bit address return its the tag value
 */
int get_tag(uint64_t addr) {
    return addr >> (set_index_bit_count + offset_bit_count);
}

/*
 * Evict the LRU node in the queue indexed by the given set index
 */
void LRU_queue_evict(int set_index) {
    LRU_queue selected_queue = queues[set_index];
    LRU_queue_node *LRU_node = selected_queue.dummy_tail->pre;
    LRU_queue_node *pre = LRU_node->pre;
    pre->next = LRU_node->next;
    LRU_node->next->pre = pre;
    // remember to free the node
    free(LRU_node);
}

// @Incomplete: we assume that there is at least one record node in the queue, so the returned tag value has its meaning
int get_LRU_tag(int set_index) {
    LRU_queue selected_queue = queues[set_index];
    LRU_queue_node *last_record_node = selected_queue.dummy_tail->pre;
    return last_record_node->tag;
}
