#include <not_implemented.h>
#include "../include/allocator_global_heap.h"
#include <mutex>

static std::mutex global_heap_mutex;

allocator_global_heap::allocator_global_heap() {}

[[nodiscard]] void *allocator_global_heap::do_allocate_sm(
    size_t size)
{
    std::lock_guard<std::mutex> lock(global_heap_mutex);
    return ::operator new(size);
}

void allocator_global_heap::do_deallocate_sm(
    void *at)
{
    std::lock_guard<std::mutex> lock(global_heap_mutex);
    ::operator delete(at);
}

allocator_global_heap::~allocator_global_heap() {}

allocator_global_heap::allocator_global_heap(const allocator_global_heap &other) {}

allocator_global_heap &allocator_global_heap::operator=(const allocator_global_heap &other)
{
    return *this;
}

bool allocator_global_heap::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return dynamic_cast<const allocator_global_heap*>(&other) != nullptr;
}

allocator_global_heap::allocator_global_heap(allocator_global_heap &&other) noexcept {}

allocator_global_heap &allocator_global_heap::operator=(allocator_global_heap &&other) noexcept
{
    return *this;
}
