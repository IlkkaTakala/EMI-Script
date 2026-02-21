#pragma once

#include <vector>
#include <mutex>
#include <cstddef>

// Simple node pool for Parser::ConstructAST. Thread-local pool to avoid locking
// between parser threads. Pool hands out raw memory blocks sized for Node.

class NodePool
{
public:
    NodePool() {
        All.reserve(4);
        Free.reserve(1024);
    }

    void* allocate()
    {
        if (Free.empty()) {
            // allocate a block of nodes to amortize new calls
            expand();
        }
        void* ptr = Free.back();
        Free.pop_back();
        return ptr;
    }

    void deallocate(void* p)
    {
        Free.push_back(p);
    }

    void clear()
    {
        for (void* p : All) ::operator delete(p);
        All.clear();
        Free.clear();
    }

    static NodePool& Instance()
    {
        thread_local static NodePool pool;
        return pool;
    }

private:
    void expand()
    {
        // allocate a chunk of raw memory holding N nodes
        constexpr size_t chunk = 1024;
        size_t nodeSize = nodeAllocSize * chunk;
        void* p = ::operator new(nodeSize);
        All.push_back(p);

        for (size_t i = 0; i < chunk; ++i) {
            Free.push_back((Node*)p + i);
        }
    }

public:
    static size_t nodeAllocSize;

private:
    std::vector<void*> All;
    std::vector<void*> Free;
};

