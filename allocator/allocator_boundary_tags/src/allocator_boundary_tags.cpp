#include <not_implemented.h>
#include <new>
#include <cstring>
#include <stdexcept>
#include <cstddef>
#include "../include/allocator_boundary_tags.h"

// Хелперы
std::pmr::memory_resource*& allocator_boundary_tags::parent_allocator_ref(void* trusted) noexcept {
    return *reinterpret_cast<std::pmr::memory_resource**>(trusted);
}
const std::pmr::memory_resource* allocator_boundary_tags::parent_allocator_ref(const void* trusted) noexcept {
    return *reinterpret_cast<const std::pmr::memory_resource* const*>(trusted);
}

allocator_with_fit_mode::fit_mode& allocator_boundary_tags::fit_mode_ref(void* trusted) noexcept {
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(
        bytes_ref(trusted) + sizeof(std::pmr::memory_resource*));
}
const allocator_with_fit_mode::fit_mode& allocator_boundary_tags::fit_mode_ref(const void* trusted) noexcept {
    return *reinterpret_cast<const allocator_with_fit_mode::fit_mode*>(
        bytes_ref(trusted) + sizeof(std::pmr::memory_resource*));
}

size_t& allocator_boundary_tags::total_space_ref(void* trusted) noexcept {
    return *reinterpret_cast<size_t*>(
        bytes_ref(trusted) + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode));
}
const size_t& allocator_boundary_tags::total_space_ref(const void* trusted) noexcept {
    return *reinterpret_cast<const size_t*>(
        bytes_ref(trusted) + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode));
}

std::mutex& allocator_boundary_tags::mutex_ref(void* trusted) noexcept {
    return *reinterpret_cast<std::mutex*>(
        bytes_ref(trusted) + sizeof(std::pmr::memory_resource*)
        + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
}
const std::mutex& allocator_boundary_tags::mutex_ref(const void* trusted) noexcept {
    return *reinterpret_cast<const std::mutex*>(
        bytes_ref(trusted) + sizeof(std::pmr::memory_resource*)
        + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
}

void*& allocator_boundary_tags::first_occupied_ref(void* trusted) noexcept {
    return *reinterpret_cast<void**>(
        bytes_ref(trusted) + sizeof(std::pmr::memory_resource*)
        + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t) + sizeof(std::mutex));
}
void* const& allocator_boundary_tags::first_occupied_ref(const void* trusted) noexcept {
    return *reinterpret_cast<void* const*>(
        bytes_ref(trusted) + sizeof(std::pmr::memory_resource*)
        + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t) + sizeof(std::mutex));
}

size_t& allocator_boundary_tags::block_size_ref(void* block) noexcept {
    return *reinterpret_cast<size_t*>(block);
}
const size_t& allocator_boundary_tags::block_size_ref(const void* block) noexcept {
    return *reinterpret_cast<const size_t*>(block);
}

void*& allocator_boundary_tags::prev_occupied_ref(void* block) noexcept {
    return *reinterpret_cast<void**>(bytes_ref(block) + sizeof(size_t));
}
void* const& allocator_boundary_tags::prev_occupied_ref(const void* block) noexcept {
    return *reinterpret_cast<void* const*>(bytes_ref(block) + sizeof(size_t));
}

void*& allocator_boundary_tags::next_occupied_ref(void* block) noexcept {
    return *reinterpret_cast<void**>(bytes_ref(block) + sizeof(size_t) + sizeof(void*));
}
void* const& allocator_boundary_tags::next_occupied_ref(const void* block) noexcept {
    return *reinterpret_cast<void* const*>(bytes_ref(block) + sizeof(size_t) + sizeof(void*));
}

void*& allocator_boundary_tags::memory_begin_tag_ref(void* block) noexcept {
    return *reinterpret_cast<void**>(bytes_ref(block) + sizeof(size_t) + 2 * sizeof(void*));
}
void* const& allocator_boundary_tags::memory_begin_tag_ref(const void* block) noexcept {
    return *reinterpret_cast<void* const*>(bytes_ref(block) + sizeof(size_t) + 2 * sizeof(void*));
}

void* allocator_boundary_tags::user_memory_ref(void* block) noexcept {
    return bytes_ref(block) + sizeof(size_t) + 3 * sizeof(void*);
}
const void* allocator_boundary_tags::user_memory_ref(const void* block) noexcept {
    return bytes_ref(block) + sizeof(size_t) + 3 * sizeof(void*);
}

void* allocator_boundary_tags::next_physical_ref(void* block) noexcept {
    return bytes_ref(block) + sizeof(size_t) + 3 * sizeof(void*) + block_size_ref(block);
}
const void* allocator_boundary_tags::next_physical_ref(const void* block) noexcept {
    return bytes_ref(block) + sizeof(size_t) + 3 * sizeof(void*) + block_size_ref(block);
}

void* allocator_boundary_tags::memory_begin_ref(void* trusted) noexcept {
    return bytes_ref(trusted) + allocator_metadata_size;
}
const void* allocator_boundary_tags::memory_begin_ref(const void* trusted) noexcept {
    return bytes_ref(trusted) + allocator_metadata_size;
}

void* allocator_boundary_tags::memory_end_ref(void* trusted) noexcept {
    return bytes_ref(memory_begin_ref(trusted)) + total_space_ref(trusted);
}
const void* allocator_boundary_tags::memory_end_ref(const void* trusted) noexcept {
    return bytes_ref(memory_begin_ref(trusted)) + total_space_ref(trusted);
}

constexpr const size_t block_metadata_size = sizeof(size_t) + 3 * sizeof(void*);
constexpr const size_t allocator_metadata_size =
    sizeof(std::pmr::memory_resource*) +
    sizeof(allocator_with_fit_mode::fit_mode) +
    sizeof(size_t) +
    sizeof(std::mutex) +
    sizeof(void*);

allocator_boundary_tags::~allocator_boundary_tags()
{
    if (_trusted_memory == nullptr) return;
    mutex_ref(_trusted_memory).~mutex();
    auto* parent = parent_allocator_ref(_trusted_memory);
    size_t bytes_to_free = allocator_metadata_size + total_space_ref(_trusted_memory);
    parent->deallocate(_trusted_memory, bytes_to_free, alignof(std::max_align_t));
    _trusted_memory = nullptr;
}

allocator_boundary_tags::allocator_boundary_tags(
    allocator_boundary_tags &&other) noexcept
{
    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;
}

allocator_boundary_tags &allocator_boundary_tags::operator=(
    allocator_boundary_tags &&other) noexcept
{
    if (this == &other) return *this;

    this->~allocator_boundary_tags();
    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;
    return *this;
}


/** If parent_allocator* == nullptr you should use std::pmr::get_default_resource()
 */
allocator_boundary_tags::allocator_boundary_tags(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    auto* real_parent = (parent_allocator != nullptr) 
                      ? parent_allocator 
                      : std::pmr::get_default_resource();

    _trusted_memory = real_parent->allocate(
        allocator_metadata_size + space_size, 
        alignof(std::max_align_t));

    parent_allocator_ref(_trusted_memory) = real_parent;
    fit_mode_ref(_trusted_memory) = allocate_fit_mode;
    total_space_ref(_trusted_memory) = space_size;
    new (&mutex_ref(_trusted_memory)) std::mutex();
    first_occupied_ref(_trusted_memory) = nullptr;
}

[[nodiscard]] void *allocator_boundary_tags::do_allocate_sm(
    size_t bytes)
{
if (_trusted_memory == nullptr) throw std::bad_alloc();
    std::lock_guard<std::mutex> lock(mutex_ref(_trusted_memory));

    const size_t needed = block_metadata_size + bytes;
    const auto mode = fit_mode_ref(_trusted_memory);
    char* pool_start = bytes_ref(memory_begin_ref(_trusted_memory));
    char* pool_end = bytes_ref(memory_end_ref(_trusted_memory));

    void* cur_prev = nullptr;
    void* cur_next = first_occupied_ref(_trusted_memory);
    char* gap_start = pool_start;

    void* opt_start = nullptr;
    void* opt_prev = nullptr;
    void* opt_next = nullptr;
    size_t opt_len = 0;

    while (true) 
    {
        char* gap_end = (cur_next == nullptr) ? pool_end : bytes_ref(cur_next);
        const size_t gap_len = static_cast<size_t>(gap_end - gap_start);

        if (gap_len >= needed) 
        {
            bool is_better = (opt_start == nullptr);
            if (!is_better) 
            {
                if (mode == fit_mode::first_fit) 
                    is_better = false;
                else if (mode == fit_mode::the_best_fit) 
                    is_better = (gap_len < opt_len);
                else if (mode == fit_mode::the_worst_fit) 
                    is_better = (gap_len > opt_len);
            }

            if (is_better) 
            {
                opt_start = gap_start;
                opt_prev = cur_prev;
                opt_next = cur_next;
                opt_len = gap_len;
                
                if (mode == fit_mode::first_fit) break;
            }
        }

        if (cur_next == nullptr) break;
        gap_start = bytes_ref(next_physical_ref(cur_next));
        cur_prev  = cur_next;
        cur_next  = next_occupied_ref(cur_next);
    }

    if (opt_start == nullptr) throw std::bad_alloc();

    const size_t remainder = opt_len - needed;
    if (remainder >= block_metadata_size + 1) 
        block_size_ref(opt_start) = bytes;
    else 
        block_size_ref(opt_start) = opt_len - block_metadata_size;

    prev_occupied_ref(opt_start) = opt_prev;
    next_occupied_ref(opt_start) = opt_next;
    memory_begin_tag_ref(opt_start) = _trusted_memory;

    if (opt_prev == nullptr) 
        first_occupied_ref(_trusted_memory) = opt_start;
    else 
        next_occupied_ref(opt_prev) = opt_start;

    if (opt_next != nullptr) 
        prev_occupied_ref(opt_next) = opt_start;

    return user_memory_ref(opt_start);
}

void allocator_boundary_tags::do_deallocate_sm(
    void *at)
{
if (_trusted_memory == nullptr || at == nullptr) return;
    std::lock_guard<std::mutex> lock(mutex_ref(_trusted_memory));

    char* user_ptr = bytes_ref(at);
    char* pool_start = bytes_ref(memory_begin_ref(_trusted_memory));
    char* pool_end = bytes_ref(memory_end_ref(_trusted_memory));

    if (user_ptr < pool_start + block_metadata_size || user_ptr >= pool_end) 
        throw std::logic_error("pointer out of allocator bounds");

    void* block = user_ptr - block_metadata_size;

    if (memory_begin_tag_ref(block) != _trusted_memory) 
        throw std::logic_error("pointer does not belong to this allocator or is already free");

    void* prev = prev_occupied_ref(block);
    void* next = next_occupied_ref(block);


    if (prev == nullptr) 
        first_occupied_ref(_trusted_memory) = next;
    else 
        next_occupied_ref(prev) = next;

    if (next != nullptr) 
        prev_occupied_ref(next) = prev;

    memory_begin_tag_ref(block) = nullptr;
    prev_occupied_ref(block)    = nullptr;
    next_occupied_ref(block)    = nullptr;
}

inline void allocator_boundary_tags::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    std::lock_guard<std::mutex> lock(mutex_ref(_trusted_memory));
    fit_mode_ref(_trusted_memory) = mode;
}


std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const
{
    std::lock_guard<std::mutex> lock(mutex_ref(_trusted_memory));
    return get_blocks_info_inner();
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::begin() const noexcept
{
    return boundary_iterator(_trusted_memory);
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::end() const noexcept
{
    return boundary_iterator();
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info_inner() const
{
   std::vector<allocator_test_utils::block_info> result;
    for (auto it = begin(); it != end(); ++it)
        result.push_back({it.size(), it.occupied() });
    return result;
}

allocator_boundary_tags::allocator_boundary_tags(const allocator_boundary_tags &other)
: _trusted_memory(nullptr)
{
    if (other._trusted_memory == nullptr) return;

    auto* res = parent_allocator_ref(other._trusted_memory);
    size_t space = total_space_ref(other._trusted_memory);

    _trusted_memory = res->allocate(allocator_metadata_size + space, alignof(std::max_align_t));

    parent_allocator_ref(_trusted_memory) = res;
    fit_mode_ref(_trusted_memory) = fit_mode_ref(other._trusted_memory);
    total_space_ref(_trusted_memory) = space;
    new (&mutex_ref(_trusted_memory)) std::mutex();

    std::memcpy(memory_begin_ref(_trusted_memory), memory_begin_ref(other._trusted_memory), space);

    first_occupied_ref(_trusted_memory) = nullptr;
    void* src = first_occupied_ref(other._trusted_memory);
    void* last_dst = nullptr;

    while (src != nullptr)
    {
        ptrdiff_t offset = bytes_ref(src) - bytes_ref(other._trusted_memory);
        void* dst = bytes_ref(_trusted_memory) + offset;

        memory_begin_tag_ref(dst) = _trusted_memory;

        if (last_dst == nullptr) {
            first_occupied_ref(_trusted_memory) = dst;
            prev_occupied_ref(dst) = nullptr;
        } else {
            next_occupied_ref(last_dst) = dst;
            prev_occupied_ref(dst) = last_dst;
        }
        last_dst = dst;
        src = next_occupied_ref(src);
    }
    if (last_dst != nullptr) next_occupied_ref(last_dst) = nullptr;
}

allocator_boundary_tags &allocator_boundary_tags::operator=(const allocator_boundary_tags &other)
{
    if (this == &other) return *this;
    this->~allocator_boundary_tags();
    new (this) allocator_boundary_tags(other);
    return *this;
}

bool allocator_boundary_tags::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return this == dynamic_cast<const allocator_boundary_tags*>(&other);
}

bool allocator_boundary_tags::boundary_iterator::operator==(
        const allocator_boundary_tags::boundary_iterator &other) const noexcept
{
    return _occupied_ptr == other._occupied_ptr && _occupied == other._occupied;
}

bool allocator_boundary_tags::boundary_iterator::operator!=(
        const allocator_boundary_tags::boundary_iterator & other) const noexcept
{
    return !(*this == other);
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator++() & noexcept
{
    if (_occupied_ptr == nullptr && _occupied) return *this;

    if (_occupied_ptr == nullptr)
    {
        _occupied_ptr = first_occupied_ref(_trusted_memory);
        _occupied = true;
        return *this;
    }

    if (_occupied)
    {
        void* nxt = next_occupied_ref(_occupied_ptr);
        if (nxt != nullptr && nxt == next_physical_ref(_occupied_ptr))
            _occupied_ptr = nxt;
        else
            _occupied = false;
    }
    else
    {
        _occupied_ptr = next_occupied_ref(_occupied_ptr);
        _occupied = true;
    }
    return *this;
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator--() & noexcept
{
    if (_trusted_memory == nullptr) return *this;

    if (_occupied_ptr == nullptr)
    {
        void* cur = first_occupied_ref(_trusted_memory);
        if (cur == nullptr) return *this;

        while (next_occupied_ref(cur) != nullptr)
            cur = next_occupied_ref(cur);

        auto* phys_end = bytes_ref(next_physical_ref(cur));
        auto* pool_end = bytes_ref(memory_end_ref(_trusted_memory));

        if (phys_end != pool_end) {
            _occupied_ptr = cur; 
            _occupied = false;
        }
        else {
            _occupied_ptr = cur; 
            _occupied = true;
        }
        return *this;
    }

    if (_occupied)
    {
        void* prev_occ = prev_occupied_ref(_occupied_ptr);

        auto* expected_prev_end = (prev_occ != nullptr) 
            ? bytes_ref(next_physical_ref(prev_occ)) 
            : bytes_ref(memory_begin_ref(_trusted_memory));

        if (bytes_ref(_occupied_ptr) != expected_prev_end)
            _occupied = false;
        else
            _occupied_ptr = prev_occ;
            if (_occupied_ptr != nullptr) _occupied = true;
    }
    else
        _occupied = true;
    return *this;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator++(int n)
{
    auto copy = *this;
    ++(*this);
    return copy;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator--(int n)
{
    auto copy = *this;
    --(*this);
    return copy;
}

size_t allocator_boundary_tags::boundary_iterator::size() const noexcept
{
    if (_occupied_ptr == nullptr)
    {
        if (_occupied) return 0;
        void* first = first_occupied_ref(_trusted_memory);
        auto* start = static_cast<const char*>(memory_begin_ref(_trusted_memory));
        auto* limit = first ? static_cast<const char*>(first) : static_cast<const char*>(memory_end_ref(_trusted_memory));
        return static_cast<size_t>(limit - start);
    }

    if (_occupied)
        return occupied_block_metadata_size + block_size_ref(_occupied_ptr);

    void* nxt = next_occupied_ref(_occupied_ptr);
    auto* gap_start = static_cast<const char*>(next_physical_ref(_occupied_ptr));
    auto* gap_end   = nxt ? static_cast<const char*>(nxt) : static_cast<const char*>(memory_end_ref(_trusted_memory));
    return static_cast<size_t>(gap_end - gap_start);
}

bool allocator_boundary_tags::boundary_iterator::occupied() const noexcept
{
    return _occupied;
}

void* allocator_boundary_tags::boundary_iterator::operator*() const noexcept
{
    if (_trusted_memory == nullptr || _occupied_ptr == nullptr)
        return (_occupied || _trusted_memory == nullptr) ? nullptr : memory_begin_ref(_trusted_memory);

    if (_occupied) return _occupied_ptr;

    return next_physical_ref(_occupied_ptr);
}

allocator_boundary_tags::boundary_iterator::boundary_iterator()
    : _trusted_memory(nullptr), _occupied_ptr(nullptr), _occupied(true) {}

allocator_boundary_tags::boundary_iterator::boundary_iterator(void *trusted)
    : _trusted_memory(trusted), _occupied_ptr(nullptr), _occupied(true)
{
    if (trusted == nullptr) return;

    const void* first_occ = first_occupied_ref(trusted);
    const char* pool_start = bytes_ref(memory_begin_ref(trusted));

    if (first_occ != nullptr && bytes_ref(first_occ) == pool_start) 
    {
        _occupied_ptr = const_cast<void*>(first_occ);
        _occupied = true;
    }
}


void *allocator_boundary_tags::boundary_iterator::get_ptr() const noexcept
{
    return _occupied_ptr;
}
