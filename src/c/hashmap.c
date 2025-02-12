/** @file hashmap.c

  @author  Melwin Svensson.
  @date    9-2-2025.
 
 */
#include "../include/c_proto.h"

#define INITIAL_CAP  16
#define LOAD_FACTOR  0.7f

struct HashNode {
  char *key;
  void *value;
  HashNode *next;
};

struct HashMap {
  HashNode **buckets;  /* This is where the data is stored, in `buckets` where each bucket has a linked list, when there are collisions. */
  int        cap;      /* The currently allocated size of `buckets`, as a whole. */
  int        size;     /* Current number of buckets with data in them. */
  
  /* A callback used to free the value of a `HashNode`, so that deallocation can happen when needed. */
  HashNodeValueFreeCb free_value;

  mutex_t globmutex;  /* This is locked when we resize the hashmap, so that we ensure singular thread resizeing. */
};

/* `INTERNAL`  Create a `djb2` hash from `str`. */
static Ulong hash_djb2(const char *str) {
  ASSERT(str);
  Ulong hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = (((hash << 5) + hash) + c);
  }
  return hash;
}

/* `INTERNAL`  Get the current `cap` or `size` of `map`, or both, NULL can be passed to one, but not both at the same time. */
static void hashmap_get_data(HashMap *map, int *cap, int *size) {
  ASSERT(map);
  /* Ensure at least one of the params are valid. */
  ASSERT(cap || size);
  under_mutex(&map->globmutex,
    ASSIGN_IF_VALID(cap, map->cap);
    ASSIGN_IF_VALID(size, map->size);
  );
}

/* `INTERNAL`  Free a `HashNode` structure. */
static inline void hashmap_free_node(HashMap *const map, HashNode *const node) {
  ASSERT(map);
  /* Ensure map is in a valid state. */
  ASSERT(map->cap);
  ASSERT(map->buckets);
  ASSERT(node);
  /* If a callback has been set to free the value of node, then call it. */
  CALL_IF_VALID(map->free_value, node->value);
  free(node->key);
  free(node);
}

/* Create a `new hashmap`. */
HashMap *hashmap_create(void) {
  HashMap *map    = xmalloc(sizeof(*map));
  map->cap        = INITIAL_CAP;
  map->size       = 0;
  map->buckets    = xcalloc(map->cap, sizeof(HashNode *));
  map->free_value = NULL;
  init_mutex(&map->globmutex, NULL);
  return map;
}

/* Free a hashmap structure. */
void hashmap_free(HashMap *const map) {
  ASSERT(map);
  HashNode *node, *next;
  /* Ensure thread safe operation, even tough this should not ever be needed. */
  under_mutex(&map->globmutex,
    ASSERT(map->cap);
    ASSERT(map->buckets);
    for (int i=0; i<map->cap; ++i) {
      node = map->buckets[i];
      while (node) {
        next = node->next;
        hashmap_free_node(map, node);
        node = next;
      }
    }
  );
  destroy_mutex(&map->globmutex);
  free(map->buckets);
  free(map);
}

/* Set the function that should be used to free HashNode's value.  Signature should ba `void foo(void *)`. */
void hashmap_set_free_value_callback(HashMap *const map, HashNodeValueFreeCb callback) {
  ASSERT(map);
  /* Assign 'callback' to the 'free_value' function ptr of map in a thread-safe manner. */
  under_mutex(&map->globmutex,
    /* Ensure the map is in a valid state. */
    ASSERT(map->cap);
    ASSERT(map->buckets);
    map->free_value = callback;
  );
}

/* `INTERNAL`  Resize `map`, this is called when `map->cap` goes above the set `LOAD_FACTOR`. */
static void hashmap_resize(HashMap *map) {
  /* Ensure the ptr to the map is valid. */
  ASSERT(map);
  /* And that the map is in a valid state. */
  ASSERT(map->cap);
  ASSERT(map->buckets);
  int   newcap;
  Ulong index;
  HashNode **newbuckets, *node, *next;
  newcap = (map->cap * 2);
  newbuckets = xcalloc(newcap, sizeof(HashNode *));
  for (int i=0; i<map->cap; ++i) {
    node = map->buckets[i];
    while (node) {
      next              = node->next;
      index             = (hash_djb2(node->key) % newcap);
      node->next        = newbuckets[index];
      newbuckets[index] = node;
      node              = next;
    }
  }
  free(map->buckets);
  map->buckets = newbuckets;
  map->cap     = newcap;
}

/* Insert a entry into `map` with `key` and `value`. */
void hashmap_insert(HashMap *map, const char *key, void *value) {
  ASSERT(map);
  ASSERT(key);
  ASSERT(value);
  Ulong     index;
  HashNode *node;
  /* Ensure thread-safe insertion. */
  under_mutex(&map->globmutex,
    /* Ensure the map is in a valid state. */
    ASSERT(map->cap);
    ASSERT(map->buckets);
    /* Check if the map needs resizing. */
    if (((float)(map->size + 1) / map->cap) > LOAD_FACTOR) {
      hashmap_resize(map);
    }
    index = (hash_djb2(key) % map->cap);
    node  = map->buckets[index];
    while (node) {
      if (strcmp(node->key, key) == 0) {
        node->value = value;
        unlock_mutex(&map->globmutex);
        return;
      }
      node = node->next;
    }
    node                = xmalloc(sizeof(HashNode));
    node->key           = xstr_copy(key);
    node->value         = value;
    node->next          = map->buckets[index];
    map->buckets[index] = node;
    ++map->size;
  );
}

/* Retrieve the `value` of a entry using the key of that entry, if any.  Otherwise, returns `NULL`. */
void *hashmap_get(HashMap *map, const char *key) {
  ASSERT(map);
  ASSERT(key);
  Ulong     index;
  HashNode *node;
  /* Ensure thread-safe retrieval of the value accosiated with key. */
  lock_mutex(&map->globmutex);
  {
    index = (hash_djb2(key) % map->cap);
    node = map->buckets[index];
    while (node) {
      if (strcmp(node->key, key) == 0) {
        unlock_mutex(&map->globmutex);
        return node->value;
      }
      node = node->next;
    }
  }
  unlock_mutex(&map->globmutex);
  return NULL;
}

/* Remove one entry from the hash map. */
void hashmap_remove(HashMap *map, const char *key) {
  ASSERT(map);
  ASSERT(key);
  Ulong     index;
  HashNode *node;
  HashNode *prev = NULL;
  /* Ensure thread-safe removal. */
  under_mutex(&map->globmutex,
    index = (hash_djb2(key) % map->cap);
    node = map->buckets[index];
    while (node) {
      /* Found the entry. */
      if (strcmp(node->key, key) == 0) {
        /* If the entry to erase is the not the first entry. */
        if (prev) {
          prev->next = node->next;
        }
        /* Otherwise, when its the first entry. */
        else {
          map->buckets[index] = node->next;
        }
        hashmap_free_node(map, node);
        --map->size;
        break;
      }
      prev = node;
      node = node->next;
    }
  );
}

/* Get the `size` of a `HashMap` structure in a `thread-safe` manner. */
int hashmap_size(HashMap *const map) {
  ASSERT(map);
  int size;
  hashmap_get_data(map, NULL, &size);
  return size;
}

/* Get the `capacity` of a `HashMap` structure in a `thread-safe` manner. */
int hashmap_cap(HashMap *const map) {
  ASSERT(map);
  int cap;
  hashmap_get_data(map, &cap, NULL);
  return cap;
}


/* ------------ Tests ------------ */


/* The concurency test will be ran by doing 1000 requsts from 100 threads concurently. */
#define OPS_PER_THREAD  1000
#define NUM_THREADS     100

/* The task for a single thread when running hashmap thread test. */
static void* hashmap_thread_test_task(void* arg) {
  TIMER_START(start);
  HashMap* map = arg;
  ASSERT(map);
  ASSERT(map->cap);
  char *key, *value;
  int op, i, insert_count=0, get_count=0, remove_count=0;
  for(i=0; i<OPS_PER_THREAD; ++i) {
    /* Generate random operation: 0=put, 1=get, 2=remove. */
    op = (rand() % 3);
    key   = fmtstr("key_%d_%d",   (int)pthread_self(), i);
    value = fmtstr("value_%d_%d", (int)pthread_self(), i);
    switch(op) {
      case 0: {
        ++insert_count;
        hashmap_insert(map, key, value);
        break;
      }
      case 1: {
        ++get_count;
        hashmap_get(map, key);
        break;
      }
      case 2: {
        ++remove_count;
        hashmap_remove(map, key);
        break;
      }
    }
    free(key);
    free(value);
  }
  TIMER_END(start, elapsed_ms);
  printf(
    "Thread %lu finished hashmap concurrent test.  Total time %.5f ms: Result: (I:%d G:%d R:%d)\n",
    pthread_self(), (double)elapsed_ms, insert_count, get_count, remove_count
  );
  return NULL;
}

/* Perform the concurency test. */
void hashmap_thread_test(void) {
  TIMER_START(start);
  int i;
  HashMap *map = hashmap_create();
  thread   threads[NUM_THREADS];
  printf("Running hashmap concurrent test.\n");
  /* Create threads. */
  for (i=0; i<NUM_THREADS; ++i) {
    ALWAYS_ASSERT(pthread_create(&threads[i], NULL, hashmap_thread_test_task, map) == 0);
  }
  /* Wait for all threads to complete. */
  for (i=0; i<NUM_THREADS; ++i) {
    pthread_join(threads[i], NULL);
  }
  hashmap_free(map);
  TIMER_END(start, elapsed_ms);
  printf("Finished hashmap concurrent test.  Total time %.5f ms\n", (double)elapsed_ms);
}

#undef OPS_PER_THREAD
#undef NUM_THREADS
