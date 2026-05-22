#include <exception>
#include <new>
#include <memory_resource>
#include <mutex>
#include <cstddef>
#include <algorithm>
#include "../include/allocator_sorted_list.h"

// Хелперы
void allocator_sorted_list::swap(allocator_sorted_list& other) noexcept
{
    std::swap(_trusted_memory, other._trusted_memory);
}

size_t allocator_sorted_list::get_space_size() const noexcept {
    return *reinterpret_cast<const size_t*>(
        reinterpret_cast<const char*>(_trusted_memory) + sizeof(std::pmr::memory_resource*) + 
        sizeof(allocator_with_fit_mode::fit_mode));
}

std::pmr::memory_resource* allocator_sorted_list::get_parent() const noexcept {
    return *reinterpret_cast<std::pmr::memory_resource* const*>(
        reinterpret_cast<const char*>(_trusted_memory));
}

allocator_with_fit_mode::fit_mode allocator_sorted_list::get_fit_mode() const noexcept {
    return *reinterpret_cast<const fit_mode*>(
        reinterpret_cast<const char*>(_trusted_memory) + sizeof(std::pmr::memory_resource*));
}

std::mutex& allocator_sorted_list::get_mutex() const noexcept {
    return *reinterpret_cast<std::mutex*>(
        reinterpret_cast<char*>(_trusted_memory) + sizeof(std::pmr::memory_resource*) +
        sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
}

void* allocator_sorted_list::get_blocks_start(const allocator_sorted_list* alloc) {
    return reinterpret_cast<char*>(alloc->_trusted_memory) + allocator_sorted_list::allocator_metadata_size;
}

static void* read_block_next(void* block) {
    return *reinterpret_cast<void**>(block);
}

static void set_block_next(void* block, void* next) {
    *reinterpret_cast<void**>(block) = next;
}

static size_t read_block_size(void* block) {
    return *reinterpret_cast<size_t*>(reinterpret_cast<char*>(block) + sizeof(void*));
}

static void set_block_size(void* block, size_t size) {
    *reinterpret_cast<size_t*>(reinterpret_cast<char*>(block) + sizeof(void*)) = size;
}

void* allocator_sorted_list::get_first_free() const noexcept
{
    return *reinterpret_cast<void**>(
        reinterpret_cast<char*>(_trusted_memory)
        + sizeof(std::pmr::memory_resource*)
        + sizeof(allocator_with_fit_mode::fit_mode)
        + sizeof(size_t)
        + sizeof(std::mutex));
}

void allocator_sorted_list::set_first_free(void* ptr) noexcept
{
    *reinterpret_cast<void**>(
        reinterpret_cast<char*>(_trusted_memory)
        + sizeof(std::pmr::memory_resource*)
        + sizeof(allocator_with_fit_mode::fit_mode)
        + sizeof(size_t)
        + sizeof(std::mutex)) = ptr;
}

allocator_sorted_list::~allocator_sorted_list()
{
    if (!_trusted_memory) return;
    
    get_mutex().~mutex(); 
    auto parent = get_parent();
    size_t total_bytes = allocator_metadata_size + get_space_size();
    parent->deallocate(_trusted_memory, total_bytes, alignof(std::max_align_t));
}

allocator_sorted_list::allocator_sorted_list(
    allocator_sorted_list &&other) noexcept
    : _trusted_memory(std::exchange(other._trusted_memory, nullptr)) {}

allocator_sorted_list &allocator_sorted_list::operator=(
    allocator_sorted_list &&other) noexcept
{
    if (*this == other) return *this;

    get_mutex().~mutex();
    get_parent()->deallocate(_trusted_memory, allocator_metadata_size +
                            block_metadata_size +
                            get_space_size());

    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;

    return *this;
}

allocator_sorted_list::allocator_sorted_list(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (!parent_allocator)
        parent_allocator = std::pmr::get_default_resource();
    
    size_t total_bytes = allocator_metadata_size + space_size;
    _trusted_memory = parent_allocator->allocate(total_bytes, alignof(std::max_align_t));
    if (!_trusted_memory) throw std::bad_alloc();
    
    char* ptr = static_cast<char*>(_trusted_memory);
    
    *reinterpret_cast<std::pmr::memory_resource**>(ptr) = parent_allocator;
    ptr += sizeof(std::pmr::memory_resource*);
    
    *reinterpret_cast<fit_mode*>(ptr) = allocate_fit_mode;
    ptr += sizeof(fit_mode);
    
    *reinterpret_cast<size_t*>(ptr) = space_size;
    ptr += sizeof(size_t);
    
    new (ptr) std::mutex;
    ptr += sizeof(std::mutex);
    
    *reinterpret_cast<void**>(ptr) = nullptr;
    ptr += sizeof(void*);
    
    void* free_block_start = ptr;
    *reinterpret_cast<void**>(free_block_start) = nullptr;
    *reinterpret_cast<size_t*>(static_cast<char*>(free_block_start) + sizeof(void*)) = space_size;
    
    set_first_free(free_block_start);
}

[[nodiscard]] void *allocator_sorted_list::do_allocate_sm(size_t size)
{
    std::lock_guard<std::mutex> lock(get_mutex());

    if (size == 0) return nullptr;

    const size_t alignment = alignof(std::max_align_t);
    size = (size + alignment - 1) & ~(alignment - 1);

    void* prev = nullptr;
    void* best_block = nullptr;
    void* best_prev = nullptr;
    size_t best_size = 0;

    void* curr = get_first_free();
    while (curr) {
        size_t curr_size = read_block_size(curr);
        if (curr_size >= size) {
            auto mode = get_fit_mode();
            switch (mode) {
                case fit_mode::first_fit:
                    best_block = curr;
                    best_prev = prev;
                    best_size = curr_size;
                    curr = nullptr;
                    break;
                case fit_mode::the_best_fit:
                    if (!best_block || curr_size < best_size) {
                        best_block = curr;
                        best_prev = prev;
                        best_size = curr_size;
                    }
                    break;
                case fit_mode::the_worst_fit:
                    if (!best_block || curr_size > best_size) {
                        best_block = curr;
                        best_prev = prev;
                        best_size = curr_size;
                    }
                    break;
            }
        }
        if (curr) {
            prev = curr;
            curr = read_block_next(curr);
        }
    }

    if (!best_block) throw std::bad_alloc();

    const size_t min_remainder = block_metadata_size + alignment;
    void* next_block = read_block_next(best_block);
    if (best_size >= size + min_remainder) {

        char* new_block = reinterpret_cast<char*>(best_block) + block_metadata_size + size;
        set_block_next(new_block, next_block);
        set_block_size(new_block, best_size - size - block_metadata_size);
        next_block = new_block;
        set_block_size(best_block, size);
    }

    if (best_prev) {
        set_block_next(best_prev, next_block);
    } else {
        set_first_free(next_block);
    }

    return reinterpret_cast<char*>(best_block) + block_metadata_size;
}

allocator_sorted_list::allocator_sorted_list(const allocator_sorted_list &other)
    : allocator_sorted_list(other.get_space_size(), 
                            other.get_parent(), 
                            other.get_fit_mode())
{}

allocator_sorted_list &allocator_sorted_list::operator=(const allocator_sorted_list &other)
{
    if (this != &other) {
        allocator_sorted_list temp(other);
        swap(temp);
    }
    return *this;
}

bool allocator_sorted_list::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    const auto* o = dynamic_cast<const allocator_sorted_list*>(&other);
    return o != nullptr && o->_trusted_memory == _trusted_memory;}

void allocator_sorted_list::do_deallocate_sm(void *at)
{
    if (!at) return;

    std::lock_guard<std::mutex> lock(get_mutex());

    void* block = reinterpret_cast<char*>(at) - block_metadata_size;

    void* blocks_start = get_blocks_start(this);
    void* blocks_end = reinterpret_cast<char*>(blocks_start) + get_space_size();
    if (block < blocks_start || block >= blocks_end)
        std::terminate();

    void* prev = nullptr;
    void* curr = get_first_free();
    while (curr && curr < block) {
        prev = curr;
        curr = read_block_next(curr);
    }

    set_block_next(block, curr);
    if (prev) {
        set_block_next(prev, block);
    } else {
        set_first_free(block);
    }

    if (curr && reinterpret_cast<char*>(block) + block_metadata_size + read_block_size(block) == curr) {
        set_block_size(block, read_block_size(block) + block_metadata_size + read_block_size(curr));
        set_block_next(block, read_block_next(curr));
    }

    if (prev && reinterpret_cast<char*>(prev) + block_metadata_size + read_block_size(prev) == block) {
        set_block_size(prev, read_block_size(prev) + block_metadata_size + read_block_size(block));
        set_block_next(prev, read_block_next(block));
    }
}

inline void allocator_sorted_list::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    std::lock_guard<std::mutex> lock(get_mutex());
    char* ptr = reinterpret_cast<char*>(_trusted_memory) + sizeof(std::pmr::memory_resource*);
    *reinterpret_cast<fit_mode*>(ptr) = mode;
}

std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info() const noexcept
{
    std::lock_guard<std::mutex> lock(get_mutex());
    return get_blocks_info_inner();
}


std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info_inner() const
{
    std::vector<allocator_test_utils::block_info> result;
    for (auto it = begin(); it != end(); ++it) {
        result.push_back({it.size(), it.occupied()});
    }
    return result;
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::free_begin() const noexcept
{
    return sorted_free_iterator(_trusted_memory);
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::free_end() const noexcept
{
    return sorted_free_iterator();
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::begin() const noexcept
{
    return sorted_iterator(_trusted_memory);
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::end() const noexcept
{
    return sorted_iterator();
}


bool allocator_sorted_list::sorted_free_iterator::operator==(
        const allocator_sorted_list::sorted_free_iterator & other) const noexcept
{
    return _free_ptr == other._free_ptr;
}

bool allocator_sorted_list::sorted_free_iterator::operator!=(
        const allocator_sorted_list::sorted_free_iterator &other) const noexcept
{
    return !(*this == other);
}

allocator_sorted_list::sorted_free_iterator &allocator_sorted_list::sorted_free_iterator::operator++() & noexcept
{
    _free_ptr = *reinterpret_cast<void**>(_free_ptr);
    return *this;
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::sorted_free_iterator::operator++(int n)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

size_t allocator_sorted_list::sorted_free_iterator::size() const noexcept
{
    return *reinterpret_cast<size_t*>(reinterpret_cast<char*>(_free_ptr) + sizeof(void*));
}

void *allocator_sorted_list::sorted_free_iterator::operator*() const noexcept
{
    return _free_ptr;
}

allocator_sorted_list::sorted_free_iterator::sorted_free_iterator()
    : _free_ptr(nullptr) {}

allocator_sorted_list::sorted_free_iterator::sorted_free_iterator(void *trusted)
{
    _free_ptr = *reinterpret_cast<void**>(reinterpret_cast<char*>(trusted) + 
        sizeof(std::pmr::memory_resource*) + sizeof(fit_mode) + 
        sizeof(size_t) + sizeof(std::mutex));
}

bool allocator_sorted_list::sorted_iterator::operator==(const allocator_sorted_list::sorted_iterator & other) const noexcept
{
    return _current_ptr == other._current_ptr;
}

bool allocator_sorted_list::sorted_iterator::operator!=(const allocator_sorted_list::sorted_iterator &other) const noexcept
{
    return !(*this == other);
}

allocator_sorted_list::sorted_iterator &allocator_sorted_list::sorted_iterator::operator++() & noexcept
{
    if (_current_ptr == _free_ptr && _free_ptr != nullptr)
        _free_ptr = *reinterpret_cast<void**>(_free_ptr);

    size_t cur_sz = *reinterpret_cast<size_t*>(reinterpret_cast<char*>(_current_ptr) + sizeof(void*));
    _current_ptr = reinterpret_cast<char*>(_current_ptr) + block_metadata_size + cur_sz;

    void* mem_end = reinterpret_cast<char*>(_trusted_memory) + 
        allocator_sorted_list::allocator_metadata_size + 
        *reinterpret_cast<size_t*>(reinterpret_cast<char*>(_trusted_memory) + sizeof(std::pmr::memory_resource*) + sizeof(fit_mode));
    if (_current_ptr >= mem_end) _current_ptr = nullptr;
    return *this;
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::sorted_iterator::operator++(int n)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

size_t allocator_sorted_list::sorted_iterator::size() const noexcept
{
    return *reinterpret_cast<size_t*>(reinterpret_cast<char*>(_current_ptr) + sizeof(void*));
}

void *allocator_sorted_list::sorted_iterator::operator*() const noexcept
{
    return _current_ptr;
}

allocator_sorted_list::sorted_iterator::sorted_iterator()
    : _free_ptr(nullptr), _current_ptr(nullptr), _trusted_memory(nullptr) {}

allocator_sorted_list::sorted_iterator::sorted_iterator(void *trusted)
    : _trusted_memory(trusted) 
{
    _current_ptr = reinterpret_cast<char*>(trusted) + allocator_sorted_list::allocator_metadata_size;
    _free_ptr = *reinterpret_cast<void**>(reinterpret_cast<char*>(trusted) + 
        sizeof(std::pmr::memory_resource*) + sizeof(fit_mode) + 
        sizeof(size_t) + sizeof(std::mutex));
}

bool allocator_sorted_list::sorted_iterator::occupied() const noexcept
{
    return _current_ptr != _free_ptr;
}
