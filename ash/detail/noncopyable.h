#ifndef ASH_DETAIL_NONCOPYABLE_H
#define ASH_DETAIL_NONCOPYABLE_H

namespace ash {

namespace _noncopyable { 

class noncopyable {
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
protected:
    /*constexpr*/ noncopyable() = default;
    ~noncopyable() = default;
};

} // !namespace _noncopyable

using _noncopyable::noncopyable;

} // !namespace mixx

#endif // !ASH_DETAIL_NONCOPYABLE_H