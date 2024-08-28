#pragma once

#include <exception>

/*
    Implementation of Rust's `Result` type.
 */

template<typename T, typename E>
class Result
{
public:
    Result(T value) : m_value(value), m_ok(true)
    {
    }

    Result(E err) : m_err(err), m_ok(false)
    {
    }

    Result() : m_ok(false)
    {
    }

    T unwrap()
    {
        if (is_err()) throw std::exception();
        return m_value;
    }

    E unwrap_err()
    {
        if (is_ok()) throw std::exception();
        return m_err;
    }

    inline bool is_ok()
    {
        return m_ok;
    }

    inline bool is_err()
    {
        return !m_ok;
    }

private:
    T m_value;
    E m_err;
    bool m_ok;
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
