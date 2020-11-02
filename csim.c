#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/* A cache line has a valid bit, a tag, and a block (not used in this program).
   A queue count is also implemented, in order to determine which line is to be evicted.*/
struct Line {
    int validbit;
    int tag;
};

/* A cache set is an array of E lines. */
struct Set {
    struct Line *lines;
};

/* A cache memory is an array of 2^s sets. */
struct Cache {
    struct Set *sets;
};

struct Line_node{
    struct Line *current_line;
    struct Line_node *next_line;
};

int main(int argc, char** argv)
{
    /* initialize set bits, associativity and file-name of trace-file */
    int s, E, b, opt;
    char *t;

    /* loop through arguments and attribute them */
    while(-1 != (opt = getopt(argc, argv, "s:E:b:t:")))
    {
        switch(opt)
        {
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                t = optarg;
                break;
            default:
                printf("wrong arg\n");
                break;
        }
    }

    /* calculate number of sets*/
    int set_num = pow(2, s);

    /* initialize memory for cache */
    struct Cache *cache;
    cache = initialize_cache(cache, set_num, E);

    /* create array of results - hits, misses, evictions */
    int results[3] = {0, 0, 0};
    int *results_ptr = results;

    /* initialize line queue */
    struct Line_node queue = malloc(sizeof(struct Line_node));
    queue->current_line = NULL;
    queue->next_line = NULL;

    struct Line_node *queue_ptr = queue;

    /* open file */
    FILE * pfile;
    pfile = fopen(t, "r");

    /* initialize elements of file contents
       for each line, there is:
        an identifier - I (instruction load, ignored in this exercise)
                        L (load from memory), S (store in memory), M (load and store from/in memory)
        an address - referenced address (that accesses the cache)
        a size - how many bytes are manipulated */
    char identifier;
    unsigned address;
    int size;

    /* for each instruction that doesn't have an I identifier, operate on the cache */
    while(fscanf(pfile, "%c %x, %d", &identifier, &address)>0)
    {
        if(identifier != "I")
        {
            operate_cache(cache, identifier, address, set_num, b, results_ptr, queue_ptr);
        }
    }

    fclose();
    /* free cache, print final results (hits, misses, evictions) and return */
    printSummary(results[0], results[1], results[2]);
    return 0;
}

/* initialize cache, set all values to 0 */
struct Cache initialize_cache(struct Cache *cache, int sets, int lines)
{
    cache->sets = struct Set[sets];
    int i, j;
    for(i = 0; i < sets; i++)
    {
        set = cache->sets[i];
        set->lines = struct Line[lines];

        for (j = 0; j < lines; j++)
        {
            set->lines[j]->validBit = 0;
            set->lines[j]->tag = 0;
            set->lines[j]->queue_count = 0;
        }
    }

    return cache;
}

void operate_cache(struct Cache *cache, char identifier, unsigned address, int sets, int size, int *results_ptr, struct Line_node *queue)
{
    int size_bits, set_bits, block_offset, set_index, tag_offset, tag, addr_size;
    addr_size = pow(2, strlen(itoa(address)));

    size_bits = pow(2, size);
    set_bits = sets << size_bits;

    tag_offset = sets + size_bits;

    /* get corresponding bits (offset, set index and tag) via bitwise operations */
    block_offset = address & size_bits;
    set_index = address & set_bits;
    tag = get_tag(address, addr_size, tag_offset);

    cache_set = cache->sets[set_index];

    bool found = false;
    bool *found_ptr = found;

    /* check each line for matching tag and valid bit */
    for(int i = 0; i < associativity; i++)
    {
        cache_line = cache_set->line[i];
        if (cache_line->validbit == 1) && (cache_line->tag == tag)
        {
            found_ptr = true;
            results_ptr[0]++;
            add_to_queue(queue, cache_line);
        }
    }

    /* if line still hasn't been found, report miss and evict line */
    if(found == false)
    {
        results_ptr[1]++;
        results_ptr[2]++;

        evicted_line = pop_from_queue(queue);
        evicted_line->validbit = 1;
        evicted_line->tag = tag;

        add_to_queue(queue, evicted_line);

        /* if it's an M instruction, there's an extra operation that hits */
        if(identifier == "M")
        {
            results_ptr[0]++;
            add_to_queue(queue, evicted_line);
        }
    }
}

int get_tag(unsigned address, int addr_size, int tag_offset)
{
    if(tag_offset == addr_size)
    {
        return 0;
    }
    else
    {
        return address >> tag_offset << tag_offset;
    }
}

/* add element to queue (traverse through it, then add line at the end */
void add_to_queue(struct Line_node *queue, struct Line *line)
{
    while(queue != NULL)
    {
        queue = queue->next_line;
    }

    queue->current_line = line;
}

/* return first line from queue and remove it */
void pop_from_queue(struct Line_node *queue)
{
    return_line = queue->current_line;
    queue = queue->next_line;
    return return_line;
}
