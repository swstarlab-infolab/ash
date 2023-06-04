#ifndef ASH_LIST_BUILDER_H
#define ASH_LIST_BUILDER_H
#include <ash/detail/noncopyable.h>
#include <type_traits>
#include <functional>
#include <assert.h>

namespace ash {

// Note that list_builder_node can be initialized by zero
struct list_builder_node {
    struct {
        list_builder_node* prev;
        list_builder_node* next;
    } __list;

    list_builder_node* get_prev_node() const noexcept {
        return __list.prev;
    }

    list_builder_node* get_next_node() const noexcept {
        return __list.next;
    }

    void set_prev_node(list_builder_node* node) noexcept{
        __list.prev = node;
    }

    void set_next_node(list_builder_node* node) noexcept {
        __list.next = node;
    }

    void reset_links() noexcept {
        __list.prev = __list.next = nullptr;
    }

    [[nodiscard]] bool is_connected() const noexcept {
        return __list.prev != nullptr || __list.next != nullptr;
    }
};

template <typename NodeTy>
class list_builder: noncopyable {
public:
    using node_type = NodeTy;
    using callback_t = std::function<void(node_type*)>;
    static_assert(
        std::is_base_of_v<list_builder_node, node_type>,
        "Target type is not list_node type!"
    );

    class iterator {
    public:
        explicit iterator(list_builder_node* p_) {
            p = static_cast<node_type*>(p_);
        }

        explicit iterator(node_type* p_) {
            p = p_;
        }

        explicit iterator(std::nullptr_t) {
            p = nullptr;
        }

        iterator operator--() {
            assert(p != nullptr);
            p = p->get_prev_node();
            return *this;
        }

        iterator& operator--(int) {
            iterator tmp(*this);
            p = p->get_prev_node();
            return tmp;
        }

        iterator& operator++() {
            assert(p != nullptr);
            p = static_cast<node_type*>(p->get_next_node());
            return *this;
        }

        iterator& operator++(int) {
            iterator tmp(*this);
            p = static_cast<node_type*>(p->get_next_node());
            return tmp;
        }

        node_type& operator*() {
            return *static_cast<node_type*>(p);
        }

        bool operator==(iterator const& rhs) {
            return p == rhs.p;
        }

        bool operator!=(iterator const& rhs) {
            return p != rhs.p;
        }

        void invalidate() {
            p = nullptr;
        }

        [[nodiscard]] bool is_null() const {
            return p == nullptr;
        }

        node_type* get_target_pointer() const {
            return p;
        }

    private:
        node_type* p;
    };

    list_builder() noexcept {
        _head.set_prev_node(nullptr);
        _head.set_next_node(nullptr);
        _tail = &_head;
        _size = 0;
    }

    ~list_builder() noexcept = default;

    iterator insert_back_to(node_type* to, node_type* new_node_) {
        assert(to != nullptr); assert(new_node_ != nullptr);
        list_builder_node* next = to->get_next_node();
        list_builder_node* new_node = new_node_;
        assert(!new_node->is_connected());
        new_node->set_prev_node(to);
        new_node->set_next_node(next);
        to->set_next_node(new_node);
        if (next != nullptr)
            next->set_prev_node(new_node);
        else
            _tail = new_node;
        assert(_tail->get_next_node() == nullptr);
        _size += 1;
        return iterator{ new_node };
    }

    iterator insert_back_to(iterator const& iter, node_type* new_node_) {
        assert(!iter.is_null()); assert(new_node_ != nullptr);
        node_type* base = iter.get_target_pointer();
        return insert_back_to(base, new_node_);
    }

    iterator insert_front_of(node_type* to, node_type* new_node_) {
        assert(to != nullptr); assert(new_node_ != nullptr);
        list_builder_node* prev = to->get_prev_node();
        list_builder_node* new_node = new_node_;
        assert(!new_node->is_connected());
        new_node->set_prev_node(prev);
        new_node->set_next_node(to);
        to->set_prev_node(new_node);
        prev->set_next_node(new_node);
        _size += 1;
        return iterator{ new_node };
    }

    iterator insert_front_of(iterator const& iter, node_type* new_node_) {
        assert(!iter.is_null()); assert(new_node_ != nullptr);
        node_type* base = iter.get_target_pointer();
        return insert_front_of(base, new_node_);
    }

    iterator insert_front(node_type* new_node) {
        return insert_back_to(iterator(&_head), new_node);
    }

    iterator insert_back(node_type* new_node) {
        return insert_back_to(iterator(_tail), new_node);
    }

    iterator insert_back_contiguous_nodes(
        node_type*     to_node,
        node_type*     first_node,
        node_type*     last_node,
        uint64_t const n) {
        assert(first_node != nullptr);
        assert(last_node != nullptr);
        if (n == 0)
            return iterator{ nullptr };

        list_builder_node* next = to_node->get_next_node();
        first_node->set_prev_node(to_node);
        last_node->set_next_node(next);
        to_node->set_next_node(first_node);
        if (next != nullptr)
            next->set_prev_node(last_node);
        else
            _tail = last_node;
        assert(_tail->get_next_node() == nullptr);
        _size += n;
        return iterator{ last_node };
    }

    iterator insert_back_contiguous_nodes(
        iterator   to_node,
        node_type* first_node,
        node_type* last_node,
        uint64_t const n) {
        return insert_back_contiguous_nodes(to_node.get_target_pointer(), first_node, last_node, n);
    }

    iterator remove_node(iterator const& iter) {
        return remove_node(iter.get_target_pointer());
    }

    iterator remove_node(node_type* target) {
        list_builder_node* prev = target->get_prev_node();
        list_builder_node* next = target->get_next_node();
        target->set_prev_node(nullptr);
        target->set_next_node(nullptr);
        prev->set_next_node(next);
        if (next != nullptr)
            next->set_prev_node(prev);
        else
            _tail = prev;
        _size -= 1;
        return iterator{ next };
    }

    std::pair<uint64_t, node_type*> /* the number of actual list,  a pointer of the last node */
    remove_contiguous_nodes(node_type* const target_node, uint64_t const max_num, callback_t const& callback) {
        if (max_num == 0 || target_node == nullptr)
            return std::make_pair(0, nullptr);
        list_builder_node* prev_node_of_target_node = target_node->get_prev_node();
        uint64_t counter = 1;
        assert(target_node != nullptr);
        if (callback != nullptr)
            callback(target_node);

        // Find a tail node
        node_type* last_node = target_node;
        for (uint64_t i = 1; i < max_num; ++i) {
            node_type* next_node = static_cast<node_type*>(last_node->get_next_node());
            if (next_node == nullptr)
                break;
            last_node = next_node;
            counter += 1;
            if (callback != nullptr)
                callback(last_node);
        }

        list_builder_node* next_node_of_last_node = last_node->get_next_node();
        target_node->set_prev_node(nullptr);
        last_node->set_next_node(nullptr);
        prev_node_of_target_node->set_next_node(next_node_of_last_node);
        if (next_node_of_last_node != nullptr)
            next_node_of_last_node->set_prev_node(prev_node_of_target_node);
        else
            _tail = prev_node_of_target_node;
        _size -= max_num;
        return std::make_pair(counter, last_node);
    }

    iterator begin() const {
        return iterator{ _head.get_next_node() };
    }

    iterator tail() const {
        return iterator{ _tail };
    }

    iterator end() const {
        return iterator{ nullptr };
    }

    bool empty() const {
        return _head.get_next_node() == nullptr;
    }

    iterator head() const {
        return iterator{ _head };
    }

    uint64_t size() const {
        return _size;
    }

private:
    list_builder_node _head;
    list_builder_node* _tail;
    uint64_t _size;
};

template <typename AccessNodeTy, typename NodeTy, typename Translator>
class extended_list_builder {
public:
    using access_t = AccessNodeTy;
    using actual_node_t = NodeTy;
    using translator = Translator;
    using actual_list_t = list_builder<actual_node_t>;

    access_t* insert_back_to(access_t* to_node, access_t* new_node) {
        return translator::convert(list.insert_back_to(translator::convert(to_node), translator::convert(new_node)).get_target_pointer());
    }

    access_t* insert_front_of(access_t* to_node, access_t* new_node) {
        return translator::convert(list.insert_front_of(translator::convert(to_node), translator::convert(new_node)).get_target_pointer());
    }

    access_t* insert_back(access_t* new_node) {
        return translator::convert(list.insert_back(translator::convert(new_node)).get_target_pointer());
    }

    access_t* insert_front(access_t* new_node) {
         return translator::convert(list.insert_front(translator::convert(new_node)).get_target_pointer());
    }

    access_t* insert_back_contiguous_nodes(access_t* to_node, access_t* first_node, access_t* last_node, uint64_t const n) {
        return translator::convert(
            list.insert_back_contiguous_nodes(translator::convert(to_node), translator::convert(first_node), translator::convert(last_node), n).get_target_pointer());
    }

    access_t* remove_node(access_t* target_node) {
        return translator::convert(list.remove_node(translator::convert(target_node)).get_target_pointer());
    }

    using callback_t = std::function<void(access_t*)>;
    std::pair<uint64_t, access_t*> remove_contiguous_nodes(access_t* to_node, uint64_t const max_num, callback_t const& callback = nullptr) {
        if (callback != nullptr) {
            auto callback_wrapper = [&callback](actual_node_t* node) {
                callback(translator::convert(node));
            };
            std::pair<uint64_t, actual_node_t*> r = list.remove_contiguous_nodes(
                translator::convert(to_node),
                max_num, callback_wrapper);
            return std::make_pair(r.first, translator::convert(r.second));
        }
        std::pair<uint64_t, actual_node_t*> r = list.remove_contiguous_nodes(
            translator::convert(to_node),
            max_num, nullptr);
        return std::make_pair(r.first, translator::convert(r.second));
    }

    uint64_t size() const {
        return list.size();
    }

    bool empty() const {
        return list.empty();
    }

    access_t* front() const {
        return translator::convert(list.begin().get_target_pointer());
    }

    access_t* tail() const {
        return translator::convert(list.tail().get_target_pointer());
    }

private:
    actual_list_t list;
};

} // namespace ash

#endif // !ASH_LIST_BUILDER_H
