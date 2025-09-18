// cache_sim.cpp
// Cache simulator (set-associative) with LRU/FIFO replacement.
// Also provides miss classification: compulsory / capacity / conflict.
// Usage (positional args):
//   ./cache_sim trace.txt [cache_size] [block_size] [assoc] [policy] [addr_bits] [-v]
// Example:
//   ./cache_sim traces/conflict.txt 32768 64 4 LRU 32 -v

#include <bits/stdc++.h>
using namespace std;
using u64 = unsigned long long;

struct FullyAssocLRU {
    int capacity;
    list<u64> order; // front = oldest, back = newest (MRU)
    unordered_map<u64, list<u64>::iterator> pos;

    FullyAssocLRU(int cap = 0) : capacity(cap) {
        order.clear();
        pos.clear();
    }

    // Access block_addr in fully-associative LRU, return true if HIT
    bool access(u64 block_addr) {
        auto it = pos.find(block_addr);
        if (it != pos.end()) {
            // move to MRU
            order.erase(it->second);
            order.push_back(block_addr);
            pos[block_addr] = prev(order.end());
            return true;
        }
        // MISS, insert (evict if necessary)
        if ((int)order.size() >= capacity && capacity > 0) {
            u64 ev = order.front();
            order.pop_front();
            pos.erase(ev);
        }
        order.push_back(block_addr);
        pos[block_addr] = prev(order.end());
        return false;
    }
};

struct Set {
    int assoc;
    bool isLRU;
    list<u64> order; // front=oldest, back=newest
    unordered_map<u64, list<u64>::iterator> pos; // used only for LRU
    unordered_set<u64> present; // used for FIFO membership

    Set(int a = 1, bool lru = true) : assoc(a), isLRU(lru) {
        order.clear();
        pos.clear();
        present.clear();
    }

    bool contains(u64 tag) const {
        if (isLRU) return pos.find(tag) != pos.end();
        return present.find(tag) != present.end();
    }

    void touch(u64 tag) {
        if (!isLRU) return;
        auto it = pos.find(tag);
        if (it == pos.end()) return;
        order.erase(it->second);
        order.push_back(tag);
        pos[tag] = prev(order.end());
    }

    void insert(u64 tag) {
        if (contains(tag)) {
            if (isLRU) touch(tag);
            return;
        }
        if ((int)order.size() < assoc) {
            order.push_back(tag);
            if (isLRU) pos[tag] = prev(order.end());
            else present.insert(tag);
        } else {
            // evict oldest
            u64 ev = order.front();
            order.pop_front();
            if (isLRU) pos.erase(ev);
            else present.erase(ev);
            // insert new
            order.push_back(tag);
            if (isLRU) pos[tag] = prev(order.end());
            else present.insert(tag);
        }
    }
};

struct Cache {
    u64 cache_size, block_size;
    int assoc;
    string policy;
    int num_sets;
    int offset_bits, index_bits, tag_bits;
    vector<Set> sets;

    FullyAssocLRU fa_sim; // fully-associative cache for miss classification

    // stats
    u64 accesses = 0, hits = 0, misses = 0;
    u64 miss_compulsory = 0, miss_capacity = 0, miss_conflict = 0;
    unordered_set<u64> seen_blocks; // block_addr (addr >> offset_bits)

    bool verbose = false;

    Cache(u64 cs, u64 bs, int a, const string &pol, int addr_bits = 32, bool verb = false)
        : cache_size(cs),
          block_size(bs),
          assoc(a),
          policy(pol),
          fa_sim( (int)(cs / bs) ),
          verbose(verb)
    {
        if (block_size == 0) throw runtime_error("block_size must be > 0");
        if (cache_size % (block_size * assoc) != 0)
            throw runtime_error("cache_size must be divisible by (block_size * assoc)");

        num_sets = (int)(cache_size / (block_size * assoc));
        offset_bits = (int)round(log2(block_size));
        index_bits = num_sets > 1 ? (int)round(log2(num_sets)) : 0;
        tag_bits = addr_bits - index_bits - offset_bits;

        bool isLRU = (pol == "LRU" || pol == "lru");
        sets.reserve(num_sets);
        for (int i = 0; i < num_sets; ++i) sets.emplace_back(assoc, isLRU);
    }

    pair<u64, int> decompose(u64 addr) {
        u64 block_addr = addr >> offset_bits; // unique block id (no offset)
        int index = num_sets > 1 ? (int)(block_addr & ((1ULL << index_bits) - 1)) : 0;
        u64 tag = block_addr >> index_bits;
        return {tag, index};
    }

    // Access address; returns tuple (hit, classification string)
    pair<bool, string> access(u64 addr) {
        accesses++;
        u64 block_addr = addr >> offset_bits;
        auto [tag, idx] = decompose(addr);

        bool first_time = seen_blocks.insert(block_addr).second; // true if inserted now => first reference

        // Check main cache
        if (sets[idx].contains(tag)) {
            hits++;
            if (policy == "LRU" || policy == "lru") sets[idx].touch(tag);
            // keep full-assoc sim updated
            fa_sim.access(block_addr);
            if (verbose) {
                printf("0x%08llx  set=%2d tag=%llu  => HIT\n", (unsigned long long)addr, idx, (unsigned long long)tag);
            }
            return {true, string("HIT")};
        } else {
            // main miss
            misses++;
            // we will use fa_sim.access to check whether this block would be present in a fully-associative LRU of same capacity
            bool fa_hit = fa_sim.access(block_addr); // this updates FA state as well
            if (first_time) {
                miss_compulsory++;
                sets[idx].insert(tag);
                if (verbose) {
                    printf("0x%08llx  set=%2d tag=%llu  => MISS (Compulsory)\n", (unsigned long long)addr, idx, (unsigned long long)tag);
                }
                return {false, string("MISS-Compulsory")};
            } else {
                if (fa_hit) {
                    miss_conflict++;
                    sets[idx].insert(tag);
                    if (verbose) {
                        printf("0x%08llx  set=%2d tag=%llu  => MISS (Conflict)\n", (unsigned long long)addr, idx, (unsigned long long)tag);
                    }
                    return {false, string("MISS-Conflict")};
                } else {
                    miss_capacity++;
                    sets[idx].insert(tag);
                    if (verbose) {
                        printf("0x%08llx  set=%2d tag=%llu  => MISS (Capacity)\n", (unsigned long long)addr, idx, (unsigned long long)tag);
                    }
                    return {false, string("MISS-Capacity")};
                }
            }
        }
    }

    void summary() {
        double hit_rate = accesses ? (100.0 * (double)hits / (double)accesses) : 0.0;
        printf("\n=== Simulation Summary ===\n");
        printf("Cache size: %llu bytes   Block size: %llu bytes   Associativity: %d-way   Num sets: %d\n",
               (unsigned long long)cache_size, (unsigned long long)block_size, assoc, num_sets);
        printf("Replacement policy: %s\n", policy.c_str());
        printf("Address decomposition: offset_bits=%d index_bits=%d tag_bits=%d\n", offset_bits, index_bits, tag_bits);
        printf("Accesses: %llu  Hits: %llu  Misses: %llu  Hit rate: %.2f%%\n",
               (unsigned long long)accesses, (unsigned long long)hits, (unsigned long long)misses, hit_rate);
        printf("Miss breakdown: Compulsory=%llu  Conflict=%llu  Capacity=%llu\n",
               (unsigned long long)miss_compulsory, (unsigned long long)miss_conflict, (unsigned long long)miss_capacity);
    }
};

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s trace.txt [cache_size] [block_size] [assoc] [policy] [addr_bits] [-v]\n", argv[0]);
        return 1;
    }

    string trace = argv[1];

    // gather optional args/flags (allow -v anywhere)
    bool verbose = false;
    vector<string> pos;
    for (int i = 2; i < argc; ++i) {
        string s(argv[i]);
        if (s == "-v" || s == "--verbose") { verbose = true; continue; }
        pos.push_back(s);
    }

    // defaults
    unsigned long long cache_size = 32768; // 32KB
    unsigned long long block_size = 64;
    int assoc = 4;
    string policy = "LRU";
    int addr_bits = 32;

    if (pos.size() > 0) cache_size = stoull(pos[0]);
    if (pos.size() > 1) block_size = stoull(pos[1]);
    if (pos.size() > 2) assoc = stoi(pos[2]);
    if (pos.size() > 3) policy = pos[3];
    if (pos.size() > 4) addr_bits = stoi(pos[4]);

    try {
        Cache sim(cache_size, block_size, assoc, policy, addr_bits, verbose);

        ifstream fin(trace);
        if (!fin.is_open()) {
            fprintf(stderr, "Error: could not open trace file '%s'\n", trace.c_str());
            return 2;
        }
        string line;
        u64 lineno = 0;
        while (getline(fin, line)) {
            lineno++;
            // strip comments and whitespace
            auto posc = line.find('#');
            if (posc != string::npos) line = line.substr(0, posc);
            // trim
            auto l = line.find_first_not_of(" \t\r\n");
            if (l == string::npos) continue;
            auto r = line.find_last_not_of(" \t\r\n");
            string token = line.substr(l, r - l + 1);
            if (token.empty()) continue;
            // parse address (hex 0x... or decimal)
            u64 addr = 0;
            try {
                if (token.rfind("0x", 0) == 0 || token.rfind("0X",0) == 0)
                    addr = stoull(token, nullptr, 16);
                else
                    addr = stoull(token, nullptr, 10);
            } catch (...) {
                if (verbose) fprintf(stderr, "Warning: skipping unparsable line %llu: '%s'\n", (unsigned long long)lineno, token.c_str());
                continue;
            }
            sim.access(addr);
        }
        sim.summary();
    } catch (const exception &ex) {
        fprintf(stderr, "Fatal error: %s\n", ex.what());
        return 3;
    }
    return 0;
}