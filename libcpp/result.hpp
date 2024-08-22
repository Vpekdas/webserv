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

template<typename T, typename E>
Result<T, E> Ok(T value)
{
    return Result<T, E>(value);
}

template<typename T, typename E>
Result<T, E> Err(E err)
{
    return Result<T, E>(err);
}

/*
    Similar to Rust's `?` operator.
 */
#define EXPECT_OK(T, E, RESULT) \
    if (RESULT .is_err()) return Err<T, E>(RESULT .unwrap_err());

#define EXPECT_OK_AND(T, E, COND, RESULT) \
    if (RESULT .is_err() && (COND)) return Err<T, E>(RESULT .unwrap_err());
