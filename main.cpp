#include <string>
#include <numeric>
#include <iostream>
#include <vector>
#include "Timer.hpp"
#include <random>
#include <unordered_map>
#include <algorithm>
#include <iomanip>
#include <assert.h>
#include <fstream>


#include <malloc.h>

void * count_malloc(size_t size);
void count_free(void *ptr);
void count_free_2( void *ptr, size_t ){
    count_free(ptr);
}


#define PROFILE_MEMORY 0
size_t count_malloc_size=0;
std::unordered_map<void*, size_t> count_sizes;

void * count_malloc(size_t size){
    count_malloc_size+= size;
    void * next = malloc(size);

#if PROFILE_MEMORY
    assert( count_sizes.find(next) == count_sizes.end());
    count_sizes[next]=size;
#endif

    return next;
}

void count_free(void *ptr){
#if PROFILE_MEMORY
    assert( !ptr || count_sizes.find( ptr ) != count_sizes.end() );
    count_malloc_size -= count_sizes[ptr];
    count_sizes.erase( count_sizes.find(ptr));
#endif
    free(ptr);
}

#include <glib.h>
#include "uthash.h"
#include "sglib.h"
#include "khash.h"

#if 0
#undef uthash_malloc
#undef uthash_free

#define uthash_malloc count_malloc
#define uthash_free count_free_2
#endif


using namespace std;

struct value_struct{
    int value;
    char waste[80];
    int operator=(int x){
        value=x;
        return value;
    }

    operator decltype(value)(){
        return value;
    }

    template <typename T>
    value_struct(T v){
        value=v;
    }

    value_struct(){
        //uninitilized
    }
};

constexpr decltype(value_struct::value)
    value_min = std::numeric_limits<decltype(value_struct::value)>::min();

constexpr decltype(value_struct::value)
    value_max = std::numeric_limits<decltype(value_struct::value)>::max();


typedef int KeyType;
typedef int ValueType;



//returns the number of bytes the program is using that came from malloc
size_t program_memory(){
    size_t ret=0;
    struct mallinfo info = mallinfo();

    ret = info.arena;
    ret+= info.hblkhd;

    ret-= info.fordblks;
    return ret;
}


struct Times{
    double insert_time;
    double random_access;
    double sequential_access;
    size_t memory_in_use;
    size_t malloc_memory;
    bool success;
};

struct Time_Hash{
    std::vector<KeyType> keys;
    std::vector<ValueType> values;
    std::vector<KeyType> scrambled;
    Time_Hash(size_t count){
        std::mt19937_64 mt;
        KeyType low_key = std::numeric_limits<KeyType>::min();
        KeyType high_key = std::numeric_limits<KeyType>::max();
        std::uniform_int_distribution<KeyType> key_distro(low_key, high_key);

        int64_t low_value = value_min;
        int64_t high_value = value_max;
        std::uniform_int_distribution<decltype(value_struct::value)>
            value_distro(low_value, high_value);

        std::unordered_map<KeyType, ValueType> umap;
        umap.reserve(count);

        while( umap.size() < count ){
            KeyType key = key_distro(mt);
            ValueType value = value_distro(mt);

            umap[key] = value;
        }

        keys.reserve(count);
        values.reserve(count);
        for( auto pair: umap){
            keys.push_back(pair.first);
            values.push_back(pair.second);
        }

        std::shuffle(keys.begin(), keys.end(), mt);
        scrambled = keys;
        std::shuffle(values.begin(), values.end(), mt);
        std::shuffle(scrambled.begin(), scrambled.end(), mt);
    }

    template <typename Hasher>
    Times run(){
        Timer insert, sequential_access, random_access;

        Times ret;
        ret.success=true;
        size_t begin_usage = count_malloc_size;
        size_t begin_malloc_usage = program_memory();

        Hasher h;

        insert.start();
        ret.success |= h.insert(keys, values);
        insert.stop();
        ret.insert_time = insert.getTime();


        sequential_access.start();
        ret.success |= h.sequential(keys, values);
        sequential_access.stop();
        ret.sequential_access = sequential_access.getTime();

        random_access.start();
        ret.success |= h.random(keys);
        random_access.stop();
        ret.random_access = random_access.getTime();

        ret.malloc_memory = program_memory() - begin_malloc_usage;
        ret.memory_in_use = ret.malloc_memory - (count_malloc_size - begin_usage);

        return ret;
    }
};

struct STD{
    std::unordered_map<KeyType, ValueType> umap;

    bool insert(std::vector<KeyType> &keys, std::vector<ValueType> &values){
        umap.reserve(keys.size());
        for( size_t i=0; i<keys.size(); i++){
            umap[keys[i]] = values[i];
        }
        return umap.size() == keys.size();
    }

    bool sequential(std::vector<KeyType> &keys, std::vector<ValueType> &values){
        size_t found=0;
        for( size_t i=0; i<keys.size(); i++){
            auto where = umap.find(keys[i]);
            if( where != umap.end() && where->second == values[i] ){
                found++;
            }
        }
        return found == keys.size();
    }


    bool random(std::vector<KeyType> &keys){
        size_t found=0;
        for( size_t i=0; i<keys.size(); i++){
            auto where = umap.find(keys[i]);
            if( where != umap.end() ){
                found++;
            }
        }
        return found == keys.size();
    }
};

struct UT{
    struct UT_Struct{
        KeyType key;
        ValueType value;
        UT_hash_handle hh;
    };

    UT_Struct *head;

    UT(){
        head=NULL;
    }

    bool insert(std::vector<KeyType> &keys, std::vector<ValueType> &values){
        static_assert( sizeof( KeyType ) == sizeof( int ), "KeyType must be an int");

        for( size_t i=0; i<keys.size(); i++){
            UT_Struct * elem = (UT_Struct*)count_malloc(sizeof(*elem));
            elem->key = keys[i];
            elem->value = values[i];

            HASH_ADD_INT(head, key, elem);
        }
        return true;
    }

    bool sequential(std::vector<KeyType> &keys, std::vector<ValueType> &values){
        size_t found=0;
        for( size_t i=0; i<keys.size(); i++){
            UT_Struct elm;
            elm.key = keys[i];

            UT_Struct *where;
            HASH_FIND_INT(head, &elm.key, where);

            if( where != nullptr && where->value == values[i] ){
                found++;
            }
        }
        return found == keys.size();
    }


    bool random(std::vector<KeyType> &keys){
        size_t found=0;
        for( size_t i=0; i<keys.size(); i++){
            UT_Struct elm;
            elm.key = keys[i];

            UT_Struct *where;
            HASH_FIND_INT(head, &elm.key, where);

            if( where != nullptr ){
                found++;
            }
        }
        return found == keys.size();
    }

    ~UT(){
        UT_Struct * where=NULL;
        UT_Struct * tmp=NULL;
        HASH_ITER( hh, head, where, tmp){
            HASH_DEL(head, where);
            count_free((void*)where);
        }
    }
};


struct GL{
    GHashTable *table;
    GL(){
        table = g_hash_table_new_full( GL::hash,
                                       GL::equal,
                                       count_free,
                                       count_free);
    }

    static guint hash( gconstpointer keyp){
        KeyType key = *(KeyType*)keyp;
        return key;
    }

    static gint equal( gconstpointer a, gconstpointer b){
        KeyType aa = *(KeyType*)a;
        KeyType bb = *(KeyType*)b;

        return aa == bb;
    }

    bool insert(std::vector<KeyType> &keys, std::vector<ValueType> &values){
        static_assert( sizeof( KeyType ) == sizeof( int ), "KeyType must be an int");

        for( size_t i=0; i<keys.size(); i++){
            KeyType *key = (KeyType*)count_malloc(sizeof(*key));
            ValueType *value = (ValueType*)count_malloc(sizeof(*value));

            *key = keys[i];
            *value = values[i];

            g_hash_table_insert(table, key, value);
        }
        return true;
    }

    bool sequential(std::vector<KeyType> &keys, std::vector<ValueType> &values){
        size_t found=0;
        for( size_t i=0; i<keys.size(); i++){
            ValueType * where = (ValueType*) g_hash_table_lookup(table, &keys[i]);
            if( where && *where == values[i] ){
                ++found;
            }
        }
        return found == keys.size();
    }


    bool random(std::vector<KeyType> &keys){
        size_t found=0;
        for( size_t i=0; i<keys.size(); i++){
            ValueType * where = (ValueType*) g_hash_table_lookup(table, &keys[i]);
            if( where ){
                ++found;
            }
        }
        return found == keys.size();
    }

    ~GL(){
    }
};




struct slist{
    KeyType key;
    ValueType value;
    slist * next;
};

#define COMPARITOR(la, lb) (la->key-lb->key)
unsigned int slist_hash_function(slist * lst){
    return lst->key;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused"
#pragma GCC diagnostic ignored "-Wunused-variable"
const static int sg_hash_size=100000;
SGLIB_DEFINE_LIST_PROTOTYPES( slist, COMPARITOR, next);
SGLIB_DEFINE_LIST_FUNCTIONS( slist, COMPARITOR, next);
SGLIB_DEFINE_HASHED_CONTAINER_PROTOTYPES(slist, sg_hash_size, slist_hash_function);
SGLIB_DEFINE_HASHED_CONTAINER_FUNCTIONS(slist, sg_hash_size, slist_hash_function);
#pragma GCC diagnostic pop

struct SG{
    slist **head;

    SG(){
        head=(slist**)count_malloc(sizeof(slist*)*sg_hash_size);
        assert(head);
        sglib_hashed_slist_init( head );
    }

    bool insert(std::vector<KeyType> &keys, std::vector<ValueType> &values){
        static_assert( sizeof( KeyType ) == sizeof( int ), "KeyType must be an int");

        for( size_t i=0; i<keys.size(); i++){
            slist * elem = nullptr;
            elem = (slist*)count_malloc(sizeof(*elem));
            elem->key = keys[i];
            elem->value = values[i];

            sglib_hashed_slist_add(head, elem);
        }
        return true;
    }

    bool sequential(std::vector<KeyType> &keys, std::vector<ValueType> &values){
        size_t found=0;
        for( size_t i=0; i<keys.size(); i++){
            slist elm;
            elm.key = keys[i];

            slist *where;
            where = sglib_hashed_slist_find_member(head, &elm);

            if( where && where->value == values[i] ){
                found++;
            }
        }
        return found == keys.size();
    }


    bool random(std::vector<KeyType> &keys){
        size_t found=0;
        for( size_t i=0; i<keys.size(); i++){
            slist elm;
            elm.key = keys[i];

            slist *where;
            where = sglib_hashed_slist_find_member(head, &elm);

            if( where ){
                found++;
            }
        }
        return found == keys.size();
    }

    ~SG(){
        slist * where=NULL;

        sglib_hashed_slist_iterator current;
        for( where = sglib_hashed_slist_it_init(&current, head);
             where != NULL;
             where=sglib_hashed_slist_it_next(&current)){

            count_free((void*)where);
        }

        count_free( (void *) head );
    }
};



struct KH {

    static uint64_t hash_fun(KeyType k){
        return k;
    }

    static uint32_t equal_fun(KeyType a, KeyType b){
        return a == b;
    }

    KHASH_INIT(m32, KeyType, ValueType, 1, hash_fun, equal_fun);

    khash_t(m32) *hash;

    KH(){
        hash = kh_init(m32);
    }

    bool insert(std::vector<KeyType> &keys, std::vector<ValueType> &values){
        for( size_t i=0; i<keys.size(); i++){
            int absent;
            khint_t k = kh_put(m32, hash, keys[i], &absent);
            kh_value(hash, k) = values[i];
        }
        return true;
    }

    bool sequential(std::vector<KeyType> &keys, std::vector<ValueType> &values){
        size_t found=0;
        for( size_t i=0; i<keys.size(); i++){
            khint_t k;
            k = kh_get(m32, hash, keys[i]);
            if( ! ( k == kh_end(hash)) && kh_value(hash, k) == values[i]){
                ++found;
            }
        }
        return found == keys.size();
    }


    bool random(std::vector<KeyType> &keys){
        size_t found=0;
        for( size_t i=0; i<keys.size(); i++){
            khint_t k;
            k = kh_get(m32, hash, keys[i]);
            if( ! ( k == kh_end(hash)) ){
                ++found;
            }
        }
        return found == keys.size();
    }

    ~KH(){
        kh_destroy(m32, hash);
    }
};



int main(int argc, char * argv[]){
    size_t count = 10;
    string fname = "";
    if( argc >= 3 ){
        fname = argv[2];
    }
    if( argc >= 2 ){
        count = atoll(argv[1]);
    }

    cerr<<"Generating the arrays ... ";
    cout.flush();
    Timer t;
    t.start();
    Time_Hash tester(count);
    t.stop();

    cerr<<"Generating the arrays took: "<<t.getTime()<<endl;

    Times std_time = tester.run<STD>();
    Times ut_time = tester.run<UT>();
    Times sg_time = tester.run<SG>();
    Times glib_time = tester.run<GL>();
    Times khash_time = tester.run<KH>();

    auto test_out = [count](std::ostream &os, string name, Times tme, int buffer=1){
        os<<setw(15*buffer)<<name<<","<<
            setw(12*buffer)<<(tme.success?"Success": "Failure")<<(buffer?" ":"")<<","<<
            setw(12*buffer)<<tme.insert_time<<","<<
            setw(12*buffer)<<tme.sequential_access<<","<<
            //setw(12*buffer)<<tme.random_access<<","<<
            setw(12*buffer)<<tme.memory_in_use<<","<<
            setw(12*buffer)<<tme.malloc_memory<<","<<
            setw(10*buffer)<<count<<endl;
    };

    std::ofstream file(fname);

#define output(os,buff) \
    test_out(os,"unordered_map", std_time, buff);\
    test_out(os,"ut_hash", ut_time, buff);\
    test_out(os,"sglib", sg_time, buff);\
    test_out(os,"glib", glib_time, buff);\
    test_out(os,"khash", khash_time, buff);

    output(cout, 1);
    if( fname != ""){
        output(file, 0);
    }
        
    return 0;
}
