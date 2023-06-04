#ifndef _ASH_DETAIL_LIST_BASE_H_
#define _ASH_DETAIL_LIST_BASE_H_
#include <ash/config.h>
#include <utility>

namespace ash {

class list_impl {
public:
    template <typename T>
    struct list_node {
        using value_type = T;
        value_type value;
        list_node* prev;
        list_node* next;
    };

    template <typename T>
    class list_iterator {
    public:
        using node_type = list_node<T>;
        using value_type = T;
        explicit list_iterator(void* p = nullptr) {
            _p = static_cast<node_type*>(p);
        }

        ASH_FORCEINLINE list_iterator& operator++() {
            _p = _p->next;
            return *this;
        }

        ASH_FORCEINLINE list_iterator& operator++(int) {
            list_iterator tmp(*this);
            _p = _p->next;
            return tmp;
        }

        ASH_FORCEINLINE value_type& operator*() {
            return _p->value;
        }

        ASH_FORCEINLINE bool operator==(list_iterator const& rhs) {
            return _p == rhs._p;
        }

        ASH_FORCEINLINE bool operator!=(list_iterator const& rhs) {
            return _p != rhs._p;
        }

        ASH_FORCEINLINE node_type* node() const {
            return _p;
        }

        ASH_FORCEINLINE void invalidate() {
            _p = nullptr;
        }

    private:
        node_type* _p;
    };

    template <typename List, typename ... Args>
    static typename List::iterator emplace_back_to(List& list, typename List::iterator const& target, Args&&... args) {
        auto dest = target.node();
        auto next = dest->next;
        auto node = list._allocate_node();
        new (&node->value) typename List::value_type (std::forward <Args>(args)...);
        node->prev = dest; node->next = next;
        dest->next = node;
        if (next != nullptr)
            next->prev = node;
        else
            list._tail = node;
        list._size += 1;
        return typename List::iterator{ node };
    }

    template <typename List, typename ... Args>
    static typename List::iterator emplace_front_of(List& list, typename List::iterator const& target, Args&&... args) {
        auto dest = target.node();
        auto prev = dest->prev;
        auto node = list._allocate_node();
        new (&node->value) typename List::value_type (std::forward <Args>(args)...);
        node->prev = prev; node->next = dest;
        dest->prev = node;
        prev->next = node;
        list._size += 1;
        return typename List::iterator{ node };
    }

    template <typename List>
    static typename List::iterator remove_node(List& list, typename List::iterator const& target) {
        using value_type = typename List::value_type;
        auto dest = target.node();
        auto prev = dest->prev;
        auto next = dest->next;
        dest->value.~value_type();
        prev->next = next;
        if (next != nullptr)
            next->prev = prev;
        else
            list._tail = prev;
        list._deallocate_node(dest);
        list._size -= 1;
        return typename List::iterator{ next };
    }

    template <typename List>
    static typename List::node_pointer remove_node(List& list, typename List::node_pointer const& target) {
        using value_type = typename List::value_type;
        auto dest = target;
        auto prev = dest->prev;
        auto next = dest->next;
        dest->value.~value_type();
        prev->next = next;
        if (next != nullptr)
            next->prev = prev;
        else
            list._tail = prev;
        list._deallocate_node(dest);
        list._size -= 1;
        return next;
    }
};

} // !namespace ash
#endif // _ASH_DETAIL_LIST_BASE_H_
