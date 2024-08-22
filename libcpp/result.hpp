#pragma once

#include <exception>
#include "maybe_uninit.hpp"

/*
    Implementation of Rust's `Result` type.
 */

template<typename T, typename E>
class Result
{
public:
    Result(T value) : m_value(MaybeUninit<T>(value))
    {
    }

    Result(E err) : m_err(MaybeUninit<E>(err))
    {
    }

    T unwrap()
    {
        if (is_err()) throw std::exception();
        T value = m_value.get();
        return value;
    }

    E unwrap_err()
    {
        if (is_ok()) throw std::exception();
        E err = m_err.get();
        return err;
    }

    inline bool is_ok()
    {
        return m_value.is_init();
    }

    inline bool is_err()
    {
        return !is_err();
    }

private:
    MaybeUninit<T> m_value;
    MaybeUninit<E> m_err;
};
