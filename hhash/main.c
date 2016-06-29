#include <stdio.h>
#include <stdlib.h>

#include "hop_hash.h"

typedef int keytype;
typedef int valuetype;

uint64_t _hash_fun ( keytype * key){
    return *key;
}


int _compare(keytype *a, keytype *b){
    return *a == *b;
}

int main(int argc, char * argv[]){
    size_t test_size=12000;
    if( argc > 1 ){
        test_size = atoll(argv[1]);
    }
    hash_data_t *hash = hop_hash_init( 127, _hash_fun, _compare);

    printf("Test size: %lu\n",test_size);

    const size_t keys_size = test_size;

    int *keys = (int*)malloc(keys_size* sizeof(int));
    int *values = (int*)malloc(keys_size* sizeof(int));

    int pos=1;
    for( size_t count=0; count<keys_size; count++){
        keys[count] = count;
    }
    /* mix up the keys */
    for( size_t count = 0; count <keys_size ; count ++){
        size_t where = rand()%keys_size;
        keytype temp = keys[where];
        keys[where] = keys[0];
        keys[0]=temp;
    }
    for( size_t count=0; count<keys_size; count++){
        values[count]=count*2;

        keys[count] *= pos;
        pos = ((pos + 1) * -1 ) + 1;/* switch between -1 and 1 */
    }

    

    for( size_t i=0; i<keys_size; i++){
        valuetype * where = add_location(hash, &keys[i]);
        *where = values[i];

        where = retrieve_value(hash, &keys[i]);
        if( *where != values[i] ){
            printf("Error on index %lu, expected %i, recieved %i\n",
                   i,
                   values[i],
                   *where);
        }
    }


    return 0;
}

