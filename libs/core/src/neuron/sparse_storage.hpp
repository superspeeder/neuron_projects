//
// Created by andy on 11/15/25.
//

#pragma once

#include <bit>
#include <cassert>
#include <concepts>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

namespace neuron {
    namespace detail {
        // require trivial copies so that we can do std::memcpy on the data
        template <typename T>
        concept sparse_storage_value = std::is_trivially_copyable_v<T> && std::default_initializable<T>;

        template <typename P>
        concept sparse_storage_policies_internal = requires(void *value) {
            { P::dynamic_allocation } -> std::common_reference_with<bool>;
            { P::write_use_element_ref } -> std::common_reference_with<bool>;
            { P::always_defragment } -> std::common_reference_with<bool>;
            { P::explicit_allocations } -> std::common_reference_with<bool>;
            { P::zero_new_allocations } -> std::common_reference_with<bool>;
            { P::call_destructor_on_free } -> std::common_reference_with<bool>;
            { P::destroy_element(value) };
        };

        template <typename P>
        concept sparse_storage_policies = std::same_as<P, void> || sparse_storage_policies_internal<P>;

        template <typename P>
        struct sparse_storage_policies_for;

        template <sparse_storage_policies_internal P>
        struct sparse_storage_policies_for<P> {
            using type = P;
        };

        struct sparse_storage_policies_default {
            static constexpr bool dynamic_allocation      = false;
            static constexpr bool write_use_element_ref   = true;
            static constexpr bool always_defragment       = true;
            static constexpr bool explicit_allocations    = false;
            static constexpr bool zero_new_allocations    = true;
            static constexpr bool call_destructor_on_free = false;
            static void           destroy_element(void *value) {}
        };

        static_assert(sparse_storage_policies_internal<sparse_storage_policies_default>);

        template <>
        struct sparse_storage_policies_for<void> {
            using type = sparse_storage_policies_default;
        };

        template <std::unsigned_integral Index>
        constexpr bool sparse_storage_valid_fixed_block_size_count(Index BlockSize, Index BlockCount) {
            return BlockSize * BlockCount <= std::numeric_limits<Index>::max() && BlockCount > 0 && BlockCount <= std::numeric_limits<std::uint32_t>::max();
        }

        template <std::size_t n>
        struct smallest_fitting_uint;

        template <std::size_t n>
            requires(n <= std::numeric_limits<std::uint8_t>::max())
        struct smallest_fitting_uint<n> {
            using type = uint8_t;
        };

        template <std::size_t n>
            requires(n > std::numeric_limits<std::uint8_t>::max() && n <= std::numeric_limits<std::uint16_t>::max())
        struct smallest_fitting_uint<n> {
            using type = uint16_t;
        };

        template <std::size_t n>
            requires(n > std::numeric_limits<std::uint16_t>::max() && n <= std::numeric_limits<std::uint32_t>::max())
        struct smallest_fitting_uint<n> {
            using type = uint32_t;
        };

        template <std::size_t n>
            requires(n > std::numeric_limits<std::uint32_t>::max() && n <= std::numeric_limits<std::uint64_t>::max())
        struct smallest_fitting_uint<n> {
            using type = uint64_t;
        };

        template <std::size_t n>
        using smallest_fitting_uint_t = smallest_fitting_uint<n>::type;

        // DONT USE THIS FOR ANYTHING USEFUL
        template <std::size_t n, std::size_t S, std::unsigned_integral D>
        struct smallest_fitting_uint_specialization;

        template <std::size_t n, std::size_t S, std::unsigned_integral D>
            requires(n == 0)
        struct smallest_fitting_uint_specialization<n, S, D> {
            // Find the smallest fitting integer type which can fit the remaining values for the index
            using type = smallest_fitting_uint_t<(1 << (sizeof(D) * 8 - std::bit_width(S))) - 1>;
        };

        template <std::size_t n, std::size_t S, std::unsigned_integral D>
            requires(n != 0)
        struct smallest_fitting_uint_specialization<n, S, D> {
            using type = smallest_fitting_uint_t<n>;
        };

        template <std::unsigned_integral Index, Index BlockSize, Index BlockCount>
        struct sparse_storage_index_pair {
            using local_index_t = smallest_fitting_uint_t<BlockSize - 1>;
            using block_index_t = smallest_fitting_uint_specialization<BlockCount - 1, BlockSize - 1, Index>::type;
            block_index_t block_index;
            local_index_t local_index;
        };
    } // namespace detail

    /**
     * **THREADSAFETY:**
     *  To use this properly in a multithreaded environment without synchronizing the whole structure:
     *      Enable the policy `explicit_allocations`
     *          This policy separates the allocation step from the write step. This will completely disable the functionality of the element_ref struct (no matter what the value of
     *          the `write_use_element_ref` policy is). This will also change the `set` function to return a `bool`. If the returned bool from `set` is `false`, then the write
     *          operation was unsuccessful due to requiring an allocation.
     *
     *          The correct way to use this setup in a thread pool (i.e. for an asset loading system where assets are stored in this storage) is to have a `std::atomic_flag`
     *              that all threads test before continuing work (if the flag is `false`, continue work, otherwise call `wait(true)` to wait for the controlling thread to perform
     *              allocations). When a thread needs to make an allocation, have a mutex locked queue of indices to allocate blocks for that is added to before signaling that an
     *              allocation is pending on the flag and starting to wait. There should also be a `std::shared_mutex` that all thread workers hold shared ownership over while it
     *              is active, and unlock before entering the flag wait stage. The controller thread should then wait for exclusive ownership over the mutex before making
     *              allocations. This parade of sync primitives should allow any thread to signal all threads that it needs to do something in a way that allows them to all finish
     *              their work without deadlocks, then the controller thread can do the allocation management.
     *
     *              My suggestion is that the controller thread should simply wait for the atomic flag to change from `false` (via `flag.wait(false)`) after finishing its work so
     *                  that it doesn't use a bunch of cpu power to do nothing. The added time cost of this should be negligible in the long run given proper tuning of this
     *                  structure's parameters.
     *
     *
     *
     * @tparam T Stored value type
     * @tparam Index Index type
     * @tparam BlockSize Size of allocated blocks
     * @tparam BlockCount Number of allocated blocks (must not be > 2^32 due to how contiguous blocks are stored)
     * @tparam Policies A struct containing policy settings which can be used to configure how this storage works.
     */
    template <detail::sparse_storage_value T, std::unsigned_integral Index, Index BlockSize, Index BlockCount, detail::sparse_storage_policies Policies = void>
    class sparse_storage {
        using policies                                = detail::sparse_storage_policies_for<Policies>::type;
        static constexpr bool dynamic_allocation      = policies::dynamic_allocation;
        static constexpr bool write_use_element_ref   = policies::write_use_element_ref;
        static constexpr bool always_defragment       = policies::always_defragment;
        static constexpr bool explicit_allocations    = policies::explicit_allocations;
        static constexpr bool zero_new_allocations    = policies::zero_new_allocations;
        static constexpr bool call_destructor_on_free = policies::call_destructor_on_free;

        static_assert((dynamic_allocation && BlockCount == 0) || detail::sparse_storage_valid_fixed_block_size_count(BlockSize, BlockCount));

        using index_pair_t = detail::sparse_storage_index_pair<Index, BlockSize, BlockCount>;

        struct contiguous_block_info {
            uint32_t block_start;
            uint32_t block_end;
        };

        using block_t = T *;
        std::vector<block_t>               _blocks;
        std::vector<contiguous_block_info> _contiguous_blocks;
        bool                               _possibly_fragmented = false;

      public:
        // The element ref type is used for allocate-on-write semantics for write-indexing
        struct element_ref {
            T              *ref;
            sparse_storage *self;
            Index           index;

            constexpr operator const T *() const { return ref; }
            constexpr operator T *() { return ref; }

            constexpr const T &operator*() const { return *ref; }
            constexpr T       &operator*() { return *ref; }

            constexpr const T *operator->() const { return ref; }
            constexpr T       *operator->() { return ref; }

            element_ref &operator=(const T &value) {
                if (!ref)
                    *ref = value;
                else {
                    ref = self->_get_alloc_for(index);
                }
                return *this;
            }

            element_ref &operator=(const element_ref &value) {
                if (this == &value)
                    return *this;
                if (value.ref) {
                    if (!ref) {
                        ref = self->_get_alloc_for(value.index);
                    }
                    *ref = *value.ref;
                }
                return *this;
            }
        };

        sparse_storage() {
            if constexpr (!dynamic_allocation) {
                _blocks.resize(BlockCount, nullptr);
                _contiguous_blocks.resize(BlockCount, contiguous_block_info{0, 0});
            }
        }

        ~sparse_storage() {
            for (std::size_t i = 0; i < _blocks.size(); ++i) {
                if (_blocks[i]) {
                    // call element destructors on free if that is enabled
                    if constexpr (call_destructor_on_free) {
                        const std::size_t block_count = _contiguous_blocks[i].block_end - _contiguous_blocks[i].block_start;
                        const std::size_t elem_count  = block_count * BlockSize;
                        block_t           block       = _blocks[i];
                        for (std::size_t j = 0; j < elem_count; ++j) {
                            policies::destructor(&block[j]);
                        }
                    }
                    delete[] _blocks[i];
                    i = _contiguous_blocks[i].block_end - 1; // jump over the contiguous block. This would be the same as `i = i` if there are no adjoined blocks
                }
            }
        }

        sparse_storage(sparse_storage const &)            = delete;
        sparse_storage &operator=(sparse_storage const &) = delete;
        sparse_storage(sparse_storage &&)                 = delete;
        sparse_storage &operator=(sparse_storage &&)      = delete;

        void set(const Index index, const T& value) requires (!explicit_allocations)
        {
            allocate_and_write(index, value);
        }

        /**
         * Explicit allocation form of the standard set function
         *
         * @param index
         * @param value
         * @return True if the set operation succeeded, false if an allocation must be made in order to complete this operation.
         */
        bool set(const Index index, const T& value) requires (explicit_allocations)
        {
            const auto [local_index, block_index] = _index_pair(index);
            auto block = _blocks[block_index];
            if (block) {
                block[local_index] = value;
                return true;
            }
            return false;
        }

        void allocate_block_containing(const Index index) {
            _get_alloc_for(index);
        }

        void allocate_and_write(const Index index, const T& value) {
            *_get_alloc_for(index) = value;
        }

        const T& get(const Index index) const
        {
            const auto [local_index, block_index] = _index_pair(index);
            return _blocks[block_index][local_index];
        }

      private:
        constexpr static index_pair_t _index_pair(Index index) {
            index_pair_t pair;
            if constexpr (std::bit_width(BlockSize) != std::bit_width(BlockSize - 1)) {
                // block size is a power of 2, we can do bit shifting to make our index
                static constexpr auto local_mask  = BlockSize - 1;
                static constexpr auto block_shift = std::bit_width(BlockSize - 1);
                pair.local_index                  = index & local_mask;
                pair.block_index                  = index >> block_shift;
            } else {
                // block size is not a power of 2, use normal math to get the indices instead.
                pair.block_index = index / BlockSize;
                pair.local_index = index % BlockSize;
            }
            return pair;
        }

        T *_get_alloc_for(Index index) {
            index_pair_t pair = _index_pair(index);
            block_t      block;
            if constexpr (dynamic_allocation) {
                // if the _blocks array is not long enough, grow it to fit the new block
                if (pair.block_index >= _blocks.size()) {
                    _dynalloc_grow_to_fit_block(pair.block_index);
                }
            }

            block = _blocks[pair.block_index];
            if (!block)
                block = _alloc_block_i(pair.block_index);

            return &block[pair.local_index];
        }

        // allocate blocks [block_start, block_end)
        // note: block_end is exclusive
        // note: we also use a larger int type so that we make sure we never try to allocate [block_start, 0) if we butt up against the end of the space.
        void _alloc_contiguous_blocks(const std::size_t block_start, const std::size_t block_end) {
            assert(block_start < block_end);
            assert(block_end <= _blocks.size());

            block_t               block = _alloc(block_end - block_start);
            contiguous_block_info cbi   = {static_cast<std::uint32_t>(block_start), static_cast<std::uint32_t>(block_end)};
            for (std::size_t i = 0; i < block_end - block_start; ++i) {
                std::size_t idx       = i + block_start;
                block_t    &block_ref = _blocks[idx];
                if (block_ref) {
                    block_t this_block = block + i * BlockSize;
                    std::memcpy(this_block, _blocks[idx], sizeof(T) * BlockSize);
                }

                block_ref = block;

                _blocks[idx]            = block + i * BlockSize;
                _contiguous_blocks[idx] = cbi;
            }
        }

        void _alloc_single_block(const std::size_t block_index) {
            assert(block_index < _blocks.size());
            _blocks[block_index]            = _alloc(1);
            _contiguous_blocks[block_index] = {block_index, block_index + 1};
        }

        // helper for allocating multiple blocks at a time
        static block_t _alloc(const std::size_t block_count) {
            T *block = new T[BlockSize * block_count];

            if constexpr (zero_new_allocations) {
                std::memset(block, 0, sizeof(T) * BlockSize * block_count);
            }

            return block;
        }

        // repair any fragmentation. This should be used to improve data locality.
        void _defragment() {
            if (_blocks.size() <= 1)
                return;

            bool        in_fragment    = false;
            std::size_t fragment_start = 0;
            for (std::size_t block_index = 0; block_index < _blocks.size(); ++block_index) {
                if (_blocks[block_index] != nullptr) {
                    if (!in_fragment) {
                        fragment_start = block_index;
                        in_fragment    = true;
                    }
                } else {
                    if (in_fragment) {
                        _alloc_contiguous_blocks(fragment_start, block_index);
                    }
                }
            }

            if (in_fragment) {
                _alloc_contiguous_blocks(fragment_start, _blocks.size());
            }
        }

        void _dynalloc_grow_to_fit_block(const index_pair_t::block_index_t block_index)
            requires(dynamic_allocation)
        {
            _blocks.resize(block_index + 1, nullptr); // we also need to make sure that we zero new block pointers
            _contiguous_blocks.resize(block_index + 1, contiguous_block_info{0, 0});
        }

        struct defragment_info {
            std::size_t start_block;
            // similar to how numbers are normally done in computer land, this is exclusive
            std::size_t end_block;
        };

        // Check if a defragmented block can be formed by filling in a hole (the passed block index).
        // Note: if the neighbors are also fragmented this will not be all that useful
        defragment_info _check_frag(index_pair_t::block_index_t block_index) {
            defragment_info info{.start_block = block_index, .end_block = static_cast<std::size_t>(block_index) + 1};
            if (block_index > 0 && _blocks[block_index - 1]) {
                info.start_block = _contiguous_blocks[block_index - 1].block_start;
            }

            if (block_index < _blocks.size() - 1 && _blocks[block_index + 1]) {
                info.end_block = _contiguous_blocks[block_index + 1].block_end;
            }

            return info;
        }

        // this function allocates a block with the precondition that the memory exists in the _blocks array for it already (does no dynalloc)
        block_t _alloc_block_i(const index_pair_t::block_index_t block_index) {
            if constexpr (always_defragment) {
                defragment_info dfi = _check_frag(block_index);
                _alloc_contiguous_blocks(dfi.start_block, dfi.end_block);
            } else {
                _alloc_single_block(block_index);
            }
            return _blocks[block_index];
        }

        // TODO: figure out how best to free blocks (due to contiguous blocks, manual frees may unintentionally break things since we don't want to waste time checking if all
        //       blocks in a contiguous block are up to be freed).
    };
} // namespace neuron::core
