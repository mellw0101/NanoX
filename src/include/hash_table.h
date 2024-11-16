/* #include <list>
#include <vector>

template <typename K, typename V>
class HashTable
{
private:
    std::vector<std::list<std::pair<K, V>>> table;
    size_t                                  numBuckets;
    std::hash<K>                            hashFn;  // You can replace this with your custom hash function.

public:
    HashTable(size_t size) : numBuckets(size) {
      table.resize(numBuckets);
    }

    void
    insert(const K &key, const V &value)
    {
        size_t hashValue = hashFn(key) % numBuckets;
        table[hashValue].emplace_back(key, value);
    }

    V *
    find(const K &key)
    {
        size_t hashValue = hashFn(key) % numBuckets;
        for (auto &pair : table[hashValue])
        {
            if (pair.first == key)
            {
                return &pair.second;
            }
        }
        return nullptr;
    }

    void
    remove(const K &key)
    {
        size_t hashValue = hashFn(key) % numBuckets;
        table[hashValue].remove_if(
            [&](const std::pair<K, V> &pair)
            {
                return pair.first == key;
            });
    }
};
 */
