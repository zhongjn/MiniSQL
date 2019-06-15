#pragma once
#include <stdexcept>
#include <type_traits>

using namespace std;

class Null {};

template <typename T> class Nullable {
    bool has_v = false;
    union { T v; };

public:
    Nullable() {}

    Nullable(Null n) {}

    Nullable(const T& t) : has_v(true), v(t) {}

    Nullable(T&& t) : has_v(true), v(move(t)) {}


    Nullable& operator=(Nullable&& n) noexcept {
        if (&n != this) {
            if (has_v) {
                v.~T();
            }
            has_v = n.has_v;
            if (has_v) {
                new(&v) T(move(n.v));
                n.has_v = false;
            }
        }
        return *this;
    }

    Nullable(Nullable&& n) noexcept {
        *this = n;
    }

    Nullable& operator=(const Nullable& n) {
        if (&n != this) {
            if (has_v) {
                v.~T();
            }
            has_v = n.has_v;
            if (has_v) {
                new(&v) T(n.v);
            }
        }
        return *this;
    }

    Nullable(const Nullable& n) {
        *this = n;
    }

    //template<typename ...Args>
    //Nullable(Args&& ... args) :
    //    v(forward<Args>(args)...), has_v(true)
    //{
    //}

    bool null() const { return !has_v; }

    const T& value() const {
        if (!has_v)
            throw logic_error("Null value");
        return v;
    }

    T& value() {
        if (!has_v)
            throw logic_error("Null value");
        return v;
    }

    const T* operator->() const {
        return &value();
    }

    T* operator->() {
        return &value();
    }

    operator bool() const {
        return has_v;
    }

    ~Nullable() {
        if (has_v) {
            has_v = false;
            v.~T();
        }
    }
};
