#include <getopt.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cachelab.h"

/* A cache line has a valid bit, a tag, and a block (not used in this program).
   A queue count is also implemented, in order to determine which line is to be evicted.*/
struct Line{
    int validbit;
    int tag;
};

/* A cache set is an array of E lines. */
struct Set{
    struct Line *lines;
};

/* A cache memory is an array of 2^s sets. */
struct Cache{
    struct Set *sets;
};

struct Line_node{
    struct Line *current_line;
    struct Line_node *next_line;
};

/* function prototypes - to avoid implicit function declaration */
void initialize_cache(struct Cache *cache, int sets, int lines);
void free_cache(struct Cache *cache, int sets);
void operate_cache(struct Cache *cache, char identifier, int E, unsigned address, int sets, int size, int *results_ptr, struct Line_node *queue);
int get_tag(unsigned address, int addr_size, int tag_offset);
void add_to_queue(struct Line_node *queue, struct Line *line);
void pop_from_queue(struct Line_node *queue);
void free_queue(struct Line_node *queue);

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
    initialize_cache(cache, set_num, E);

    /* create array of results - hits, misses, evictions */
    int results[3] = {0, 0, 0};
    int *results_ptr = results;

    /* initialize line queue */
    struct Line_node *queue = (struct Line_node *) malloc(sizeof(struct Line_node));
    queue->current_line = NULL;
    queue->next_line = NULL;

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
    while(fscanf(pfile, "%c %x, %d", &identifier, &address, &size)>0)
    {
        if(identifier != 'I')
        {
            operate_cache(cache, identifier, E, address, set_num, b, results_ptr, queue);
        }
    }

    fclose(pfile);
    /* free cache and queue, print final results (hits, misses, evictions) and return */
    free_cache(cache, set_num);
    free_queue(queue);
    printSummary(results[0], results[1], results[2]);
    return 0;
}

/* initialize cache, set all values to 0 */
void initialize_cache(struct Cache *cache, int sets, int lines)
{
    cache =(struct Cache *) malloc(sizeof(struct Cache));

    struct Set *set_array =(struct Set *) malloc(sizeof(struct Set) * sets);
    cache->sets = set_array;

    int i, j;
    for(i = 0; i < sets; i++)
    {
        struct Set set = cache->sets[i];
        struct Line *set_lines = (struct Line *) malloc(sizeof(struct Line) * lines);
        set.lines = set_lines;

        for (j = 0; j < lines; j++)
        {
            set.lines[j].validbit = 0;
            set.lines[j].tag = 0;
        }
    }
}

/* free cache sets and lines */
void free_cache(struct Cache *cache, int sets)
{
    int i;
    for (i=0; i<sets; i++)
    {
        free(cache->sets[i].lines);
    }
    free(cache->sets);
    free(cache);
}

void operate_cache(struct Cache *cache, char identifier, int E, unsigned address, int sets, int size, int *results_ptr, struct Line_node *queue)
{
    int size_bits, set_bits, block_offset, set_index, tag_offset, tag, addr_size, addr_len;

    addr_len = snprintf( NULL, 0, "%d", address);
    addr_size = pow(2, addr_len);

    size_bits = pow(2, size);
    set_bits = sets << size_bits;

    tag_offset = sets + size_bits;

    /* get corresponding bits (offset, set index and tag) via bitwise operations */
    block_offset = address & size_bits;
    set_index = address & set_bits;
    tag = get_tag(address, addr_size, tag_offset);

    struct Set cache_set = cache->sets[set_index];

    bool found = false;
    bool *found_ptr = &found;

    /* check each line for matching tag and valid bit */
    for(int i = 0; i < E; i++)
    {
        struct Line cache_line = cache_set.lines[i];
        if((cache_line.validbit == 1) && (cache_line.tag == tag))
        {
            *found_ptr = true;
            results_ptr[0]++;
            add_to_queue(queue, &cache_line);
        }
    }

    /* if line still hasn't been found, report miss and evict line */
    if(found == false)
    {
        results_ptr[1]++;
        results_ptr[2]++;

        struct Line *evicted_line = queue->current_line;
        pop_from_queue(queue);
        evicted_line->validbit = 1;
        evicted_line->tag = tag;

        add_to_queue(queue, evicted_line);

        /* if it's an M instruction, there's an extra operation that hits */
        if(identifier == 'M')
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
    struct Line_node *last = queue;
    while(last != NULL)
    {
        last = last->next_line;
    }

    struct Line_node *new_node = (struct Line_node *) malloc(sizeof(struct Line_node));
    new_node->current_line = line;
    new_node->next_line = NULL;

    last->next_line = new_node;
}

/* remove first line from queue */
void pop_from_queue(struct Line_node *queue)
{
    queue = queue->next_line;
}

void free_queue(struct Line_node *queue)
{
    struct Line_node *tmp;

    while (queue != NULL)
    {
        tmp = queue;
        queue = queue->next_line;
        free(tmp);
    }
}
