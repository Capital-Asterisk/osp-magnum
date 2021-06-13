/**
 * Open Space Program
 * Copyright © 2019-2020 Open Space Program Project
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef INCLUDED_OSP_SHARED_STRING_H_56F463BF_C8A5_4633_B906_D7250C06E2DB
#define INCLUDED_OSP_SHARED_STRING_H_56F463BF_C8A5_4633_B906_D7250C06E2DB
#pragma once

#include <memory>
#include <string_view>
#include <type_traits>

namespace osp
{

/**
 * A class representing a read-only string with shared ownership of the underlying storage for the string.
 * The interface is a std::string_view, and the lifetime management is conducted through move construction / assignment
 * of the template variable LIFETIME_T.
 *
 * The intended usage of this class is for strings that are read-only, but not compile-time constants.
 * For example, data read out of a configuration file, or from the network, that needs to have a long lifetime
 * but which is never modified after initial creation.
 *
 * In either of those situations, using basic_shared_string provides a single allocation for the lifetime of the
 * string data, while still providing the full interface of string_view, as well as relatively cheap copy operations
 * (depending on the specific LIFETIME_T parameter, of course).
 *
 * A very nieve implementation is to provide std::any as the lifetime management. This is undesirable because of the additional
 * pointer indirection that std::any incurs because of it's type erasure.
 *
 * The next most obvious implementation is to use std::shared_ptr<const char[]> for lifetime, which results in basic_shared_string
 * having a size of 4*sizeof(void*) (2 for the std::string_view, 2 for the shared_ptr). That's the implementation provided by the
 * using shared_string = ...; line below the template class declaration.
 *
 * A more sophisticated implementation could use intrusive reference counting for the string data, instead of a seperately
 * allocated and tracked management block like std::shared_ptr uses, and this interface also would allow for a choice of
 * atomic reference counting, or non-atomic, depending on the template parameters.
 *
 * Note also that the create_***() functions at the end of this file provide a mechanism to create a shared_string that
 * does not attempt lifetime management (e.g. default constructs m_lifetime). This is provided for two purposes.
 * 1. compile-time string literals being passed to a function that take "ownership" of the data passed in,
 *    but the string literal requires no ownership because it's lifetime is a superset of the lifetime of the program
 *    in such case, the extra cost of copying / moving the m_lifetime variable can be omitted.
 * 2. Situations where the use of shared_string is required due to type system constraints, but the caller of the code
 *    knows for certain that the ownership of the passed in shared_string will never outlive the lifetime of the function call
 *    (e.g. the function never copies the shared_string, only references it). Situations with this usage are rare, but do happen
 *    so the create_reference_shared_string() function provides a way to do that.
 */
template<typename CHAR_T, typename LIFETIME_T>
class basic_shared_string : public std::basic_string_view<CHAR_T>
{
    using ThisType_t = basic_shared_string;
    using ViewBase_t = std::basic_string_view<CHAR_T>;
    static_assert(std::is_nothrow_default_constructible_v<ViewBase_t>);
    static_assert(std::is_nothrow_move_constructible_v<ViewBase_t>);
    static_assert(std::is_nothrow_copy_constructible_v<ViewBase_t>);
    static_assert(std::is_nothrow_move_assignable_v<ViewBase_t>);
    static_assert(std::is_nothrow_copy_assignable_v<ViewBase_t>);
    static_assert(std::is_nothrow_destructible_v<ViewBase_t>);

public:
    using traits_type            = typename ViewBase_t::traits_type;
    using value_type             = typename ViewBase_t::value_type;
    using pointer                = typename ViewBase_t::pointer;
    using const_pointer          = typename ViewBase_t::const_pointer;
    using reference              = typename ViewBase_t::reference;
    using const_reference        = typename ViewBase_t::const_reference;
    using const_iterator         = typename ViewBase_t::const_iterator;
    using iterator               = typename ViewBase_t::iterator;
    using const_reverse_iterator = typename ViewBase_t::const_reverse_iterator;
    using reverse_iterator       = typename ViewBase_t::reverse_iterator;
    using size_type              = typename ViewBase_t::size_type;
    using difference_type        = typename ViewBase_t::difference_type;

    constexpr basic_shared_string(void) noexcept( std::is_nothrow_default_constructible_v<LIFETIME_T> ) = default;
    basic_shared_string(basic_shared_string &&) noexcept( std::is_nothrow_move_constructible_v<LIFETIME_T> )  = default;
    basic_shared_string(basic_shared_string const&) noexcept( std::is_nothrow_copy_constructible_v<LIFETIME_T> )  = default;
    basic_shared_string& operator=(basic_shared_string &&) noexcept( std::is_nothrow_move_assignable_v<LIFETIME_T> )  = default;
    basic_shared_string& operator=(basic_shared_string const&) noexcept( std::is_nothrow_copy_assignable_v<LIFETIME_T> )  = default;
    ~basic_shared_string() noexcept( std::is_nothrow_destructible_v<LIFETIME_T> )  = default;

    /**
     * @brief substr -- see std::string_view::substr
     * @param offset
     * @param length
     * @return a new basic_shared_string that has a copy of the m_lifetime variable to preserve the lifetime semantics of the object.
     */
    basic_shared_string substr(size_type offset, size_type length = ViewBase_t::npos) const& noexcept(false) // std::string_view::substr can throw.
    {
        return { ViewBase_t::substr(offset, length), m_lifetime };
    }

    /**
     * @brief substr -- see std::string_view::substr
     * @param offset
     * @param length
     * @return a new basic_shared_string that has the m_lifetime variable of this basic_shared_string std::moved() into it.
     * This allows for a basic_shared_string that is an rvalue reference to have substr called on it, and avoid copying the lifetime.
     */
    basic_shared_string substr(size_type offset, size_type length = ViewBase_t::npos) && noexcept(false) // std::string_view::substr can throw.
    {
        return { ViewBase_t::substr(offset, length), std::move(m_lifetime) };
    }

    /**
     * @brief operator std::basic_string<CHAR_T>
     * Convenience function for getting an std::basic_string<CHAR_T> from this basic_shared_string
     */
    explicit operator std::basic_string<CHAR_T>() noexcept( noexcept( std::basic_string<CHAR_T>(ViewBase_t::data(), ViewBase_t::size()) ) )
    {
        return { ViewBase_t::data(), ViewBase_t::size() };
    }

protected:
    friend basic_shared_string create_shared_string(ViewBase_t, LIFETIME_T) noexcept( std::is_nothrow_move_constructible_v<LIFETIME_T> );

    template<typename IT_T>
    friend basic_shared_string create_shared_string(IT_T&&, IT_T&&) noexcept(false); // allocates
    friend basic_shared_string create_shared_string(const char * data, size_t len) noexcept(false);
    friend basic_shared_string create_shared_string(ViewBase_t) noexcept(false); // allocates

    friend basic_shared_string create_reference_shared_string(ViewBase_t) noexcept( std::is_nothrow_default_constructible_v<LIFETIME_T> );

    explicit constexpr basic_shared_string(ViewBase_t view) noexcept( std::is_nothrow_default_constructible_v<LIFETIME_T> )
     : ViewBase_t{ view }
    { }

    basic_shared_string(ViewBase_t view, LIFETIME_T lifetime) noexcept( std::is_nothrow_move_constructible_v<LIFETIME_T> )
     : ViewBase_t{ view }
     , m_lifetime{ std::move(lifetime) }
    { }

private:
    LIFETIME_T m_lifetime;
}; // class basic_shared_string

/**
 * The concrete implementation.
 */
using shared_string = basic_shared_string<char, std::shared_ptr<const char[]>>;


shared_string create_shared_string(std::string_view view, std::shared_ptr<const char[]> buf) noexcept
{
    return shared_string{ view, std::move(buf) };
}

template<typename IT_T>
shared_string create_shared_string(IT_T && begin, IT_T && end) noexcept(false) // allocates
{
    // Determine size
    auto len = static_cast<std::size_t>(std::distance(begin, end));

    // Make space to copy the string
    auto buf = std::make_shared<char[]>(len);

    // Do the copy
    std::copy(std::forward<IT_T>(begin), std::forward<IT_T>(end), buf.get());

    // Construct prior to shared_string constructor to avoid
    // ABI specific parameter passing order problems.
    auto view = std::string_view{ buf.get(), len };

    // Make the shared_string
    return create_shared_string(view, std::move(buf));
}

shared_string create_shared_string(const char * data, size_t len) noexcept(false) // allocates
{
    // Make space to copy the string
    auto buf = std::make_shared<char[]>(len);

    // Do the copy
    std::copy(data, data+len, buf.get());

    // Construct prior to shared_string constructor to avoid
    // ABI specific parameter passing order problems.
    auto view = std::string_view{ buf.get(), len };

    // Make the shared_string
    return create_shared_string(view, std::move(buf));
}

shared_string create_shared_string(std::string_view view) noexcept(false) // allocates
{
    return create_shared_string(view.begin(), view.end());
}

/**
 * @brief create_reference_shared_string
 * @param view
 * @return a shared_string that does not attempt lifetime management at all.
 */
shared_string create_reference_shared_string(std::string_view view) noexcept
{
    return shared_string{ view };
}

} // namespace osp

#endif // defined INCLUDED_OSP_SHARED_STRING_H_56F463BF_C8A5_4633_B906_D7250C06E2DB
