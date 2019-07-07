#ifndef BEATMODS2_UTIL_ALGORITHM_H
#define BEATMODS2_UTIL_ALGORITHM_H

#include <algorithm>
#include <utility>
#include <stdexcept>
#include <type_traits>

namespace BeatMods::algorithm {

    // nice generalized sequence split function
    template<typename RandomIterator, class PredFunctor, class ActionFunctor>
    constexpr void split(RandomIterator begin, RandomIterator end, PredFunctor&& pred, ActionFunctor&& action) {
        for (auto rstart = begin, lag = begin; lag != end; rstart = ++begin) { // TODO: maybe get rid of lag somehow?
            while (begin != end && !pred(begin)) ++begin;
            if constexpr (std::is_same_v<bool, decltype(action({},{}))>) {
                if (!action(rstart, begin)) break;
            } else {
                action(rstart, begin);
            }
            lag = begin;
        }
    }

    // specialization for splitting on a value based on ==
    template<typename RandomIterator, class ActionFunctor>
    constexpr void split_equals(RandomIterator begin, RandomIterator end, decltype(*std::declval<RandomIterator>()) const& val, ActionFunctor&& action) {
        split(begin, end, [&](auto v) { return *v == val; }, action);
    }

    template<typename ForwardIterator, class PredFunctor>
    constexpr bool all(ForwardIterator begin, ForwardIterator end, PredFunctor&& pred) {
        for (; begin != end; ++begin)
            if (!pred(*begin)) return false;
        return true;
    }

    template<typename T>
    constexpr void debug_use(T const&) noexcept {}

    template<typename Ch>
    [[nodiscard]] constexpr std::basic_string_view<Ch> remove_prefix(std::basic_string_view<Ch> sv, typename std::basic_string_view<Ch>::size_type size) noexcept {
        sv.remove_prefix(size);
        return sv;
    }

    template<typename Ch>
    [[nodiscard]] constexpr std::basic_string_view<Ch> remove_suffix(std::basic_string_view<Ch> sv, typename std::basic_string_view<Ch>::size_type size) noexcept {
        sv.remove_suffix(size);
        return sv;
    }

    [[nodiscard]] constexpr bool is_digit(char c) noexcept {
        return c >= '0' && c <= '9';
    }

    [[nodiscard]] constexpr bool is_numeric(std::string_view sv) noexcept {
        return all(sv.begin(), sv.end(), is_digit);
    }

    namespace detail {
        template<typename I>
        constexpr I parse_int_impl(std::string_view s, I val) {
            return s.size() == 0 ? val 
                    : is_digit(s[0]) ? 
                        parse_int_impl<I>(remove_prefix(s, 1), (val*10) + (s[0] - '0'))
                        : throw std::domain_error("number not a valid integer");
        }
    }

    template<typename I = int>
    [[nodiscard]] constexpr I parse_int(std::string_view s) { // only support ascii numbers because meh
        static_assert(std::is_integral_v<I>);
        if constexpr (std::is_signed_v<I>) {
            if (s[0] == '-') // only compile this check if parsing signed type
                return -detail::parse_int_impl<I>(remove_prefix(s, 1), 0);
            else 
                return detail::parse_int_impl<I>(s, 0);
        } else {
            return detail::parse_int_impl<I>(s, 0);
        }
    }

    // the only difference between this and std::copy_n is this is contexpr
    template<class InputIt, class Size, class OutputIt>
    constexpr OutputIt copy_n(InputIt first, Size count, OutputIt result) {
        if (count > 0) {
            *result++ = *first;
            for (Size i = 1; i < count; ++i) {
                *result++ = *++first;
            }
        }
        return result;
    }

}

#endif