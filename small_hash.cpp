#include <cassert>
#include <iostream>
#include <time.h>
#include <unordered_set>
#include <sys/time.h>
#include <limits>
#include <vector>
using namespace std;

// To Compile and Run:
// g++  -std=c++11  -march=native -flto -g -O3 -o small_hash small_hash.cpp
// ./smallhash

struct key {
  char a;
  char b;
  char c;
};

inline bool operator==(const struct key& a, const struct key& b) {
  return (a.a == b.a && a.b == b.b && a.c == b.c);
}

inline bool operator!=(const struct key& a, const struct key& b) {
  return (a.a != b.a || a.b != b.b || a.c != b.c);
}

class linear_probing {
public:

  linear_probing() {
    capacity = 1;
    size = 0;
    contains_zero = false;
    array = (struct key*)malloc(sizeof(struct key));
    memset(array, 0, sizeof(struct key));
    return;
  }

  bool contains(struct key x) {
    if (is_zero(x)) return contains_zero;
    uint32_t pos = hash(x);
    while (!is_zero(array[pos]) && array[pos] != x) {
      pos = pos + 1;
      while (pos >= capacity) pos = pos - capacity;
    }
    return (array[pos] == x); 
  }

  void insert(struct key x) {
    if (is_zero(x)) {
      contains_zero = true;
      return;
    }
    uint32_t pos = hash(x);
    while (!is_zero(array[pos]) && array[pos] != x) {
      pos = pos + 1;
      while (pos >= capacity) pos = pos - capacity;
    }
    size += (array[pos] != x);
    array[pos] = x;
    if (size > capacity * 0.9) grow();
  }

  ~linear_probing() {
    free(array);
  }
  
private:

  inline bool is_zero(struct key x) {
    return (x.a == 0 && x.b == 0 && x.c == 0);
  }
  
  inline uint32_t hash(struct key x) {
    // An old 32 bit hash I used to use
    uint64_t h = x.a + x.b * 256 + x.c * 256 * 256;
    h = (~h) + (h << 21); // h = (h << 21) - h - 1;
    h = h ^ (h >> 24);
    h = (h + (h << 3)) + (h << 8); // h * 265
    h = h ^ (h >> 14);
    h = (h + (h << 2)) + (h << 4); // h * 21
    h = h ^ (h >> 28);
    h = h & (((uint64_t)1 << 32) - 1);
    // a neat trick for getting a number in range [capacity] without any divisions.
    return (h * (uint64_t)capacity) >> 32;
  }

  void grow() {
    uint64_t new_capacity = (uint64_t) ceil((float)size / 0.8);
    assert(new_capacity < ((uint64_t)1 << 32));    
    struct key* array2 = (struct key*)malloc(sizeof(struct key) * new_capacity);
    memset(array2, 0, sizeof(struct key) * new_capacity);
    struct key* array3 = array;
    array = array2;
    size = 0;
    uint32_t old_cap = capacity;
    capacity = new_capacity;
    for (uint32_t i = 0; i < old_cap; i++) {
      if (!is_zero(array3[i])) insert(array3[i]);
    }
    free(array3);
  }

  bool contains_zero;
  uint32_t capacity;
  uint32_t size;
  struct key* array;
  
};

class crazy_hash {
public:
  inline void insert(uint32_t x) {
    struct key k = *((struct key*)&x);
    uint8_t byte = x % 256;
    tables[byte].insert(k);
  }
  inline bool contains(uint32_t x) {
    struct key k = *((struct key*)&x);
    uint8_t byte = x % 256;
    return tables[byte].contains(k);
  }
  crazy_hash() {
    tables.resize(256);
  }
  ~crazy_hash() {
  }
private:
  vector <linear_probing> tables;
};

uint32_t hash_int(uint32_t x) {
    // An old 32 bit hash I used to use
  uint64_t h = x;
  h = (~h) + (h << 21); // h = (h << 21) - h - 1;
  h = h ^ (h >> 24);
  h = (h + (h << 3)) + (h << 8); // h * 265
  h = h ^ (h >> 14);
  h = (h + (h << 2)) + (h << 4); // h * 21
  h = h ^ (h >> 28);
  return h & (((uint64_t)1 << 32) - 1);
}

struct Hash {
   inline size_t operator()(const uint32_t &x) const {
     uint64_t h = x;
     h = (~h) + (h << 21); // h = (h << 21) - h - 1;
     h = h ^ (h >> 24);
     h = (h + (h << 3)) + (h << 8); // h * 265
     h = h ^ (h >> 14);
     h = (h + (h << 2)) + (h << 4); // h * 21
     h = h ^ (h >> 28);
     return (size_t)h;
   }
};

typedef unsigned long long timestamp_t;

// copied from http://stackoverflow.com/questions/1861294/how-to-calculate-execution-time-of-a-code-snippet-in-c
static timestamp_t
    get_timestamp ()
{
  struct timeval now;
  gettimeofday (&now, NULL);
  return  now.tv_usec + (timestamp_t)now.tv_sec * 1000000;
}

int main () {
  timestamp_t start_time = get_timestamp();
  class crazy_hash table;
  for (uint32_t i = 0; i < 10000000; i++) {
    table.insert(2 * i);
  }
  for (uint32_t i = 0; i < 10000000; i++) {
    assert(!table.contains(2 * i + 1));
    assert(table.contains(2 * i));
  }
  timestamp_t end_time = get_timestamp();
  cout<<"Time with our hash table (s): "<<(end_time - start_time)/1000000.0L<<endl;
  
  // In order to be fair, I force the c++ hash table to use our hash function
  start_time = get_timestamp();
  unordered_set<uint32_t, Hash> H;
  for (uint32_t i = 0; i < 10000000; i++) {
    H.emplace(2 * i);
  }
  for (uint32_t i = 0; i < 10000000; i++) {
    assert(H.find(2 * i + 1) == H.end());
    assert(H.find(2 * i) != H.end());
  }
  end_time = get_timestamp();
  cout<<"Time with std hash table (s): "<<(end_time - start_time)/1000000.0L<<endl;

  return 0;
}
