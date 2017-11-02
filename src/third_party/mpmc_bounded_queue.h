#pragma once

// Copyright Gavin Lambert 2012 - 2017.
// Based on code copyright Dmitry Vyukov 2010 - 2011.
// http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

#include <atomic>
#include <memory>
#include <type_traits>
#include <boost/assert.hpp>
#include <boost/detail/no_exceptions_support.hpp>

namespace lockfree_queue
{
    const size_t cache_line_size = 64;  // use hardware_destructive_interference_size in C++17
    
    using std::atomic;
    using std::memory_order;
    using std::memory_order_acquire;
    using std::memory_order_consume;
    using std::memory_order_relaxed;
    using std::memory_order_release;
    using std::memory_order_acq_rel;
    using std::memory_order_seq_cst;

    // lock-free, blocking, bounded, array-based allocation
    template< typename T, typename allocator = std::allocator<T> >
    class mpmc_bounded_value
    {
    private:
        using value_allocator_traits = typename std::allocator_traits<allocator>::template rebind_traits<T>;

    public:
        typedef allocator allocator_type;
        typedef typename value_allocator_traits::value_type value_type;
        typedef typename value_allocator_traits::size_type size_type;
        typedef typename value_allocator_traits::const_pointer const_pointer;
        typedef typename value_allocator_traits::pointer pointer;
        typedef value_type const& const_reference;
        typedef value_type& reference;

    public:
        // max_size must be a power of two
        explicit mpmc_bounded_value(size_t max_size)
            : m_buffer(nullptr), m_buffer_mask(max_size - 1), m_enqueue_pos(0), m_dequeue_pos(0), m_allocator()
        {
            BOOST_ASSERT_MSG((max_size >= 2) && ((max_size & m_buffer_mask) == 0), "size must be a power of two");
            init();
        }

        mpmc_bounded_value(size_t max_size, allocator_type const& a)
            : m_buffer(nullptr), m_buffer_mask(max_size - 1), m_enqueue_pos(0), m_dequeue_pos(0), m_allocator(a)
        {
            BOOST_ASSERT_MSG((max_size >= 2) && ((max_size & m_buffer_mask) == 0), "size must be a power of two");
            init();
        }

        ~mpmc_bounded_value()
        {
            while (dequeue_cell_prepare())
            {
                // values dequeued and destroyed by discarded unique_ptr return
            }
            for (size_t i = 0; i <= m_buffer_mask; ++i)
            {
                m_buffer[i].~cell();
            }
            cell_allocator_traits::deallocate(m_allocator, m_buffer, m_buffer_mask + 1);
        }
        
        // not copyable nor moveable.  not strictly needed as atomic members should block this too, but a little paranoia never hurts.
        mpmc_bounded_value(const mpmc_bounded_value&) = delete;
        mpmc_bounded_value& operator=(const mpmc_bounded_value&) = delete;

        BOOST_CONSTEXPR allocator_type get_allocator() const { return m_allocator; }
        
        bool is_lock_free() const BOOST_NOEXCEPT_OR_NOTHROW { return m_enqueue_pos.is_lock_free() && m_dequeue_pos.is_lock_free(); }

        template<typename... Args>
        bool emplace(Args&&... args) BOOST_NOEXCEPT_IF((std::is_nothrow_constructible<T, Args...>::value))
        {
            cell *c;
            size_t pos;
            if (enqueue_cell_reserve(c, pos))
            {
                enqueue_impl(c, pos, std::is_nothrow_constructible<T, Args...>(), std::forward<Args>(args)...);
                return true;
            }
            return false;
        }

        bool enqueue(const T& v) /*BOOST_NOEXCEPT_IF(BOOST_NOEXCEPT_EXPR(emplace(v)))*/ { return emplace(v); }
        bool enqueue(T&& v) BOOST_NOEXCEPT_IF(BOOST_NOEXCEPT_EXPR(emplace(std::move(v)))) { return emplace(std::move(v)); }

        bool dequeue(reference v) BOOST_NOEXCEPT_IF(std::is_nothrow_move_assignable<T>::value)
        {
            if (auto c = dequeue_cell_prepare())
            {
                v = std::move(c->value());    // exception-safe via unique_ptr cleanup
                return true;
            }
            return false;
        }

    private:
        struct cell
        {
            explicit cell(size_t s = 0) : q(s) {}
            struct node
            {
                node(size_t s = 0) : sequence(s), invalid(false) {}
                node(const node& n) : sequence(n.sequence.load(memory_order_relaxed)), invalid(false) {}
                atomic<size_t> sequence;
                bool invalid;
            } q;
            reference value() BOOST_NOEXCEPT_OR_NOTHROW { return reinterpret_cast<reference>(storage); }
        private:
            typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
        };

        using cell_allocator_traits = typename std::allocator_traits<allocator>::template rebind_traits<cell>;
        typedef char cacheline_pad[cache_line_size];
        typedef std::make_signed_t<size_t> diff_t;

        struct cell_dequeue
        {
            cell_dequeue() : q(nullptr) {}
            cell_dequeue(mpmc_bounded_value *qq, size_t p) : q(qq), pos(p) {}
            mpmc_bounded_value *q;
            size_t pos;
            void operator()(cell *c) const
            {
                if (q && c)
                {
                    cell_allocator_traits::destroy(q->m_allocator, std::addressof(c->value()));
                    q->dequeue_cell_release(c, pos);
                }
            }
        };

    private:
        void init()
        {
            m_buffer = cell_allocator_traits::allocate(m_allocator, m_buffer_mask + 1);
            for (size_t i = 0; i <= m_buffer_mask; ++i)
            {
                ::new(&m_buffer[i]) cell(i);
            }
        }
        
        bool enqueue_cell_reserve(cell*& c, size_t& pos) BOOST_NOEXCEPT_OR_NOTHROW
        {
            pos = m_enqueue_pos.load(memory_order_relaxed);
            for (;;)
            {
                c = &m_buffer[pos & m_buffer_mask];
                size_t seq = c->q.sequence.load(memory_order_acquire);
                diff_t diff = (diff_t)seq - (diff_t)pos;
                if (diff == 0)
                {
                    if (m_enqueue_pos.compare_exchange_weak(pos, pos + 1, memory_order_relaxed))
                    {
                        return true;
                    }
                }
                else if (diff < 0)
                {
                    return false;
                }
                else
                {
                    pos = m_enqueue_pos.load(memory_order_relaxed);
                }
            }
        }

        void enqueue_cell_commit(cell *c, size_t pos) BOOST_NOEXCEPT_OR_NOTHROW
        {
            c->q.sequence.store(pos + 1, memory_order_release);
        }

        template<typename... Args>
        void enqueue_impl(cell *c, size_t pos, std::true_type, Args&&... args) BOOST_NOEXCEPT_OR_NOTHROW
        {
            cell_allocator_traits::construct(m_allocator, std::addressof(c->value()), std::forward<Args>(args)...);
            enqueue_cell_commit(c, pos);
        }

        template<typename... Args>
        void enqueue_impl(cell *c, size_t pos, std::false_type, Args&&... args)
        {
            BOOST_TRY
            {
                cell_allocator_traits::construct(m_allocator, std::addressof(c->value()), std::forward<Args>(args)...);
            }
            BOOST_CATCH(...)
            {
                // we can't unreserve a cell, so mark it as invalid for dequeue to skip
                c->q.invalid = true;
                enqueue_cell_commit(c, pos);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
            enqueue_cell_commit(c, pos);
        }

        std::unique_ptr<cell, cell_dequeue> dequeue_cell_prepare() BOOST_NOEXCEPT_OR_NOTHROW
        {
            size_t pos = m_dequeue_pos.load(memory_order_relaxed);
            for (;;)
            {
                cell *c = &m_buffer[pos & m_buffer_mask];
                size_t seq = c->q.sequence.load(memory_order_acquire);
                diff_t diff = (diff_t)seq - (diff_t)(pos + 1);
                if (diff == 0)
                {
                    if (m_dequeue_pos.compare_exchange_weak(pos, pos + 1, memory_order_relaxed))
                    {
                        if (c->q.invalid)
                        {
                            c->q.invalid = false;
                            c->q.sequence.store(pos + m_buffer_mask + 1, memory_order_release);
                            continue;
                        }
                        return { c, cell_dequeue(this, pos) };
                    }
                }
                else if (diff < 0)
                {
                    return nullptr; // caught up to enqueue in progress or queue empty
                }
                else
                {
                    pos = m_dequeue_pos.load(memory_order_relaxed);
                }
            }
        }

        void dequeue_cell_release(cell *c, size_t pos) BOOST_NOEXCEPT_OR_NOTHROW
        {
            c->q.sequence.store(pos + m_buffer_mask + 1, memory_order_release);
        }

    private:
        cell* m_buffer;
        size_t const m_buffer_mask;
        cacheline_pad pad1_;
        atomic<size_t> m_enqueue_pos;
        cacheline_pad pad2_;
        atomic<size_t> m_dequeue_pos;
        cacheline_pad pad3_;
        typename cell_allocator_traits::allocator_type m_allocator;
    };

    // this is just like mpmc_bounded_value except that the max size is defined by the type rather than the instance (useful for arrays)
    template< typename T, size_t N, typename allocator = std::allocator<T> >
    class mpmc_fixed_bounded_value : public mpmc_bounded_value<T, allocator>
    {
    public:
        static_assert((N >= 2) && ((N & (N-1)) == 0), "N must be power of two");

        mpmc_fixed_bounded_value() : mpmc_bounded_value<T, allocator>(N) {}
        explicit mpmc_fixed_bounded_value(allocator const& a) : mpmc_bounded_value<T, allocator>(N, a) {}
    };

}
