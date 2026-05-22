#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BOUNDARY_TAGS_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BOUNDARY_TAGS_H

#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <pp_allocator.h>
#include <iterator>
#include <mutex>

class allocator_boundary_tags final :
    public smart_mem_resource,
    public allocator_test_utils,
    public allocator_with_fit_mode
{

private:

    static constexpr const size_t allocator_metadata_size = sizeof(memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) +
                                                            sizeof(size_t) + sizeof(std::mutex) + sizeof(void*);

    static constexpr const size_t occupied_block_metadata_size = sizeof(size_t) + sizeof(void*) + sizeof(void*) + sizeof(void*);

    static constexpr const size_t free_block_metadata_size = 0;

    void *_trusted_memory;

    static inline char* bytes_ref(void* p) noexcept { return static_cast<char*>(p); }
    static inline const char* bytes_ref(const void* p) noexcept { return static_cast<const char*>(p); }

    static std::pmr::memory_resource*& parent_allocator_ref(void* trusted) noexcept;
    static const std::pmr::memory_resource* parent_allocator_ref(const void* trusted) noexcept;

    static allocator_with_fit_mode::fit_mode& fit_mode_ref(void* trusted) noexcept;
    static const allocator_with_fit_mode::fit_mode& fit_mode_ref(const void* trusted) noexcept;

    static size_t& total_space_ref(void* trusted) noexcept;
    static const size_t& total_space_ref(const void* trusted) noexcept;

    static std::mutex& mutex_ref(void* trusted) noexcept;
    static const std::mutex& mutex_ref(const void* trusted) noexcept;

    static void*& first_occupied_ref(void* trusted) noexcept;
    static void* const& first_occupied_ref(const void* trusted) noexcept;

    static size_t& block_size_ref(void* block) noexcept;
    static const size_t& block_size_ref(const void* block) noexcept;

    static void*& prev_occupied_ref(void* block) noexcept;
    static void* const& prev_occupied_ref(const void* block) noexcept;

    static void*& next_occupied_ref(void* block) noexcept;
    static void* const& next_occupied_ref(const void* block) noexcept;

    static void*& memory_begin_tag_ref(void* block) noexcept;
    static void* const& memory_begin_tag_ref(const void* block) noexcept;

    static void* user_memory_ref(void* block) noexcept;
    static const void* user_memory_ref(const void* block) noexcept;

    static void* next_physical_ref(void* block) noexcept;
    static const void* next_physical_ref(const void* block) noexcept;

    static void* memory_begin_ref(void* trusted) noexcept;
    static const void* memory_begin_ref(const void* trusted) noexcept;

    static void* memory_end_ref(void* trusted) noexcept;
    static const void* memory_end_ref(const void* trusted) noexcept;    

public:
    
    ~allocator_boundary_tags() override;
    
    allocator_boundary_tags(allocator_boundary_tags const &other);
    
    allocator_boundary_tags &operator=(allocator_boundary_tags const &other);
    
    allocator_boundary_tags(
        allocator_boundary_tags &&other) noexcept;
    
    allocator_boundary_tags &operator=(
        allocator_boundary_tags &&other) noexcept;

public:
    
    explicit allocator_boundary_tags(
            size_t space_size,
            std::pmr::memory_resource *parent_allocator = nullptr,
            allocator_with_fit_mode::fit_mode allocate_fit_mode = allocator_with_fit_mode::fit_mode::first_fit);

private:
    
    [[nodiscard]] void *do_allocate_sm(
        size_t bytes) override;
    
    void do_deallocate_sm(
        void *at) override;

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

public:
    
    inline void set_fit_mode(
        allocator_with_fit_mode::fit_mode mode) override;

public:
    
    std::vector<allocator_test_utils::block_info> get_blocks_info() const override;

private:

    std::vector<allocator_test_utils::block_info> get_blocks_info_inner() const override;

/** TODO: Highly recommended for helper functions to return references */

    class boundary_iterator
    {
        void* _occupied_ptr;
        bool _occupied;
        void* _trusted_memory;

    public:

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = void*;
        using reference = void*&;
        using pointer = void**;
        using difference_type = ptrdiff_t;

        bool operator==(const boundary_iterator&) const noexcept;

        bool operator!=(const boundary_iterator&) const noexcept;

        boundary_iterator& operator++() & noexcept;

        boundary_iterator& operator--() & noexcept;

        boundary_iterator operator++(int n);

        boundary_iterator operator--(int n);

        size_t size() const noexcept;

        bool occupied() const noexcept;

        void* operator*() const noexcept;

        void* get_ptr() const noexcept;

        boundary_iterator();

        boundary_iterator(void* trusted);
    };

    friend class boundary_iterator;

    boundary_iterator begin() const noexcept;

    boundary_iterator end() const noexcept;
};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BOUNDARY_TAGS_H