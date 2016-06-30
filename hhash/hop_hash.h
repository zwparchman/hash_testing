#ifndef  hop_hash_INC
#define  hop_hash_INC

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

typedef int h_Key_Type;
typedef int h_Value_Type;
typedef uint8_t h_bitmap_type;



typedef struct {
    size_t slot_count;
    size_t in_use;
    h_Value_Type * data;
    h_Key_Type * keys;
    uint64_t * hashes;
    int neighbood_size;
    uint64_t (*hash)(h_Key_Type *key);
    int (*compare)(h_Key_Type *a, h_Key_Type *b);
} hash_data_t;

static const uint64_t bit64 = 1ull<<63;

/* returns 0 or the index of the most significant bit set */
int find_highest_set( uint64_t val ){
    int where = 0;
    while( val ){
        val >>= 1;
        where ++;
    }
    return where;
}

static inline int equal_fun(h_Key_Type *a, h_Key_Type *b){
    return *a == *b;
}

static uint64_t hash_fun(h_Key_Type *k){
    return *k;
}

hash_data_t * hop_hash_init( size_t slots,
                             uint64_t (*hash)(h_Key_Type* key),
                             int (*compare)(h_Key_Type *a, h_Key_Type *b)){
    assert(hash);
    assert(compare);


    slots = (1 << find_highest_set(slots));

    hash_data_t *ret = (hash_data_t*)malloc( sizeof(hash_data_t));
    if( ! ret ) return NULL;

    ret->data = (h_Value_Type*)calloc(sizeof(h_Value_Type), slots);
    ret->keys = (h_Key_Type*) calloc(sizeof(h_Key_Type), slots);
    ret->hashes = (uint64_t*) calloc(sizeof(uint64_t), slots);

    if( ! (ret->keys && ret->data) ){
        free(ret->data);
        free(ret->keys);
        return NULL;
    }

    ret->neighbood_size = 8;
    ret->slot_count=slots;
    ret->in_use=0;
    ret->hash = hash;
    ret->compare = compare;

    return ret;
}



size_t mod( size_t first, size_t second){
    //assumes second is a power of 2 > 0
    return first & (second-1);
}

uint64_t hash_in_use( uint64_t combo ){
    return combo & bit64;
}

uint64_t extract_hash( uint64_t combo){
    return combo & (~bit64);
}

size_t _distance( hash_data_t *dat, uint64_t first, uint64_t second){
    if( first <= second ){
        return second - first;
    } else {
        return dat->slot_count - first + second;
    }
}

size_t distance( hash_data_t *dat, uint64_t first, uint64_t second){
    int ret = _distance(dat,first,second);
    //printf("%i: %lu\n",ret, dat->slot_count);
    return ret;
}

void hhash_move(hash_data_t *dat, size_t a, size_t b){
    dat->hashes[a] = dat->hashes[b];
    dat->keys[a] = dat->keys[b];
    dat->data[a] = dat->data[b];

    dat->hashes[b] = 0;
}


size_t make_room(hash_data_t*  dat, size_t where, int retries){
    /* this function is called when the `where` bucket dosen't have any free
     * slots in its range so make room. The room will come from our far end*/
    if( retries < 1){
        return dat->slot_count;
    }

    while( retries -- ){

        /* find free slot */
        size_t free_slot = dat->slot_count;
        for( size_t offset=0;
             offset < (uint64_t)dat->neighbood_size * retries;
             offset++){
            size_t modded = mod(where+offset, dat->slot_count);
            if( ! hash_in_use( dat->hashes[modded] )){
                /* TODO check disassembly */
                free_slot = modded ;
                break;
            }
        }

        if( free_slot == dat->slot_count){
            return free_slot;
        }

        /* find a hash that can go in the free slot */
        size_t new_free_slot=dat->slot_count;
        assert(free_slot < dat->slot_count);
        for( size_t offset=dat->neighbood_size*retries;
             offset != where;
             offset--){
            size_t modded_offset = mod(offset+where, dat->slot_count);
            size_t hash = dat->hashes[modded_offset];

            if( ! hash_in_use(hash)) continue;

            size_t hash_loc = mod(extract_hash(hash), dat->slot_count);
            size_t foward_distance = distance(dat, hash_loc, free_slot);
            if( (size_t)dat->neighbood_size < foward_distance ){
                hhash_move(dat, free_slot, modded_offset);
                new_free_slot = hash_loc;
                break;
            }
        }
        if( distance(dat, where, new_free_slot) < (size_t)dat->neighbood_size){
            return new_free_slot;
        }
    }

    return dat->slot_count;
}

int rehash(hash_data_t *dat){
    const size_t new_slots = dat->slot_count *2;

    size_t new_neighborhood = 1;

    h_Key_Type * const keys = (h_Key_Type*)calloc( sizeof(h_Key_Type), new_slots);
    h_Value_Type * const values = (h_Value_Type*)calloc( sizeof(h_Value_Type), new_slots);
    uint64_t * const hashes = (uint64_t*)calloc( sizeof(uint64_t), new_slots);

    if( ! (keys && values && hashes )){
        free(keys);
        free(values);
        free(hashes);
        return 1;
    }

    size_t found=0;
    for( size_t i=0; i < dat->slot_count; i++){
        const uint64_t hash = dat->hashes[i];
        if( ! hash_in_use( hash )){
            continue;
        }

        const size_t modded = mod( extract_hash(hash), new_slots);
        size_t j;
        for( j=0; j< new_slots; j++){
            const size_t remodded = mod( modded + j, new_slots);
            if( ! hash_in_use(hashes[remodded]) ){
                hashes[remodded] = hash;
                keys[remodded] = dat->keys[i];
                values[remodded] = dat->data[i];
                found++;
                break;
            }
        }
        if( j > new_neighborhood ){
            new_neighborhood = j;
        }
    }

    assert(found == dat->in_use);

    memset(dat->keys, 0, sizeof(dat->keys[0]));
    free(dat->keys);
    memset(dat->data, 0, sizeof(dat->data[0]));
    free(dat->data);
    memset(dat->hashes, 0, sizeof(dat->hashes[0]));
    free(dat->hashes);

    dat->keys = keys;
    dat->data = values;
    dat->hashes = hashes;

    dat->slot_count = new_slots;
    dat->neighbood_size = new_neighborhood + 3;

    return 0;
}


h_Value_Type * add_location(hash_data_t *dat, h_Key_Type *key){
    uint64_t hash = extract_hash(hash_fun(key));
    size_t where = mod( hash, dat->slot_count);

    while(1){
        for( size_t offset = 0; offset < (uint64_t)dat->neighbood_size; offset++){
            size_t modded = mod(where+offset, dat->slot_count);
            //ignore the case where hashes match for now
            if( ! hash_in_use(dat->hashes[modded]) ){
                //if we get here we have an address to return
                dat->hashes[modded] = bit64 | hash;
                dat->keys[modded] = *key;
                ++dat->in_use;
                return dat->data + modded;
            }
        }

        size_t new_slot = make_room(dat,where, 2);
        if( dat->slot_count != new_slot){
            dat->hashes[new_slot] = hash;
            dat->keys[new_slot] = *key;
            dat->in_use++;
            return dat->data + new_slot;
        }

        //faild to find slot so rehash
        rehash(dat);
        where = mod(hash, dat->slot_count);
    }
}

void mark_unused(hash_data_t *dat, size_t slot){
    dat->hashes[slot] = 0;
}

int hop_hash_remove(hash_data_t *dat, h_Key_Type *key){
    uint64_t hash = hash_fun(key) & (~bit64);
    size_t where = mod( hash, dat->slot_count);

    for( size_t offset = 0; offset < (uint64_t)dat->neighbood_size; offset++){
        size_t modded = mod(where+offset, dat->slot_count);
        //ignore the case where hashes match for now
        if( ((hash ^ dat->hashes[modded] ) << 1) == 0 &&
            equal_fun(&dat->keys[modded], key)){
            mark_unused(dat, modded);
            return 0;
        }
    }
    return 1;
}

h_Value_Type *retrieve_value(hash_data_t *dat, h_Key_Type *key){
    uint64_t hash = hash_fun(key) & (~bit64);
    size_t where = mod( hash, dat->slot_count);

    for( size_t offset = 0; offset < (uint64_t)dat->neighbood_size; offset++){
        size_t modded = mod(where+offset, dat->slot_count);
        //ignore the case where hashes match for now
        if( ((hash ^ dat->hashes[modded] ) << 1) == 0 &&
            equal_fun(&dat->keys[modded], key)){
            return dat->data + modded;
        }
    }

    return NULL;
}

void hhash_destroy( hash_data_t *dat){
    free(dat->hashes);
    free(dat->data);
    free(dat->keys);
}

#endif   /* ----- #ifndef hop_hash_INC  ----- */
