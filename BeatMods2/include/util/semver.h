#ifndef BEATMODS2_UTIL_SEMVER_H
#define BEATMODS2_UTIL_SEMVER_H

#include <vector>
#include <algorithm>
#include <variant>
#include <stdexcept>
#include "util/algorithm.h"

namespace BeatMods::semver {

    class Version {
        static constexpr size_t bufsize = 32;

        unsigned int major_ = 0;
        unsigned int minor_ = 0;
        unsigned int patch_ = 0;

        size_t pre_len = 0;
        char prerelease_[bufsize] = {0};
        size_t build_len = 0;
        char build_[bufsize] = {0};

        static constexpr void _copy_to(std::string_view dat, char(&target)[bufsize]) noexcept {
            BeatMods::algorithm::copy_n(dat.data(), std::min(dat.size(), bufsize-1), target);
        }
        static constexpr void _copy(char(&src)[bufsize], char(&dest)[bufsize]) noexcept {
            BeatMods::algorithm::copy_n(src, bufsize, dest);
        }
        constexpr void _set_pre(std::string_view dat) noexcept {
            _copy_to(dat, prerelease_);
            pre_len = dat.size();
        }
        constexpr void _set_build(std::string_view dat) noexcept {
            _copy_to(dat, build_);
            build_len = dat.size();
        }

    public:
        constexpr Version() noexcept = default;
        constexpr Version(std::string_view view) {
            if (view.size() == 0)
                throw std::length_error("semver version length zero");
            
            char const* left = view.data();
            size_t len = 0;
            int part = 0;
            bool reading_pre = false;
            bool reading_build = false;

            auto read_num_part = [&]() {
                switch (part++) {
                    case 0:
                        major_ = BeatMods::algorithm::parse_int<decltype(major_)>({left, static_cast<size_t>(len)}); break;
                    case 1:
                        minor_ = BeatMods::algorithm::parse_int<decltype(minor_)>({left, static_cast<size_t>(len)}); break;
                    case 2:
                        patch_ = BeatMods::algorithm::parse_int<decltype(patch_)>({left, static_cast<size_t>(len)}); break;
                    default:
                        throw std::length_error("too many version components");
                }
            };

            for (auto i = 0; i < view.size(); ++i, ++len) {
                if (reading_build) {
                    // do nothing
                } else if (reading_pre) {
                    if (view[i] == '+') {
                        _set_pre({left, len});
                        reading_build = true;
                        left = view.data() + i + 1;
                        len = -1;
                    } 
                } else 
                switch (view[i]) {
                    case '+': reading_build = true;
                    case '-': reading_pre = true;
                    case '.':
                        read_num_part();
                        left = view.data() + i + 1;
                        len = -1;
                        break;
                    default: break;
                }
            }

            if (reading_build) {
                _set_build({left, len});
            } else if (reading_pre) {
                _set_pre({left, len});
            } else
                read_num_part();

            if (part < 3) throw std::length_error("too few version components");
        }
        constexpr Version(unsigned int maj, unsigned int min, unsigned int pat, std::string_view pre, std::string_view build) noexcept
            : major_(maj), minor_(min), patch_(pat) {
                _set_pre(pre);
                _set_build(build);
            }

        [[nodiscard]] constexpr auto major() const noexcept { return major_; }
        [[nodiscard]] constexpr auto minor() const noexcept { return minor_; }
        [[nodiscard]] constexpr auto patch() const noexcept { return patch_; }
        constexpr Version& major(unsigned int n) noexcept { major_ = n; return *this; }
        constexpr Version& minor(unsigned int n) noexcept { major_ = n; return *this; }
        constexpr Version& patch(unsigned int n) noexcept { major_ = n; return *this; }

        [[nodiscard]] constexpr std::string_view prerelease_v() const noexcept { return {prerelease_, pre_len}; }
        [[nodiscard]] constexpr std::string_view build_v() const noexcept { return {build_, build_len}; }
        constexpr Version& prerelease(std::string_view v) noexcept { _set_pre(v); return *this; }
        constexpr Version& build(std::string_view v) noexcept { _set_build(v); return *this; }
        [[nodiscard]] std::string prerelease() const noexcept { return std::string{prerelease_v()}; }
        [[nodiscard]] std::string build() const noexcept { return std::string{build_v()}; }
        
        constexpr Version(Version const& other) noexcept
            : major_(other.major()), minor_(other.minor()), patch_(other.patch()) {
                _set_pre(other.prerelease_v());
                _set_build(other.build_v());
            }

        [[nodiscard]] constexpr Version copy() const noexcept { return Version(*this); }

        [[nodiscard]] constexpr Version& reset_major(unsigned int n) noexcept {
            return major(n).minor(0).patch(0).prerelease("").build("");
        }
        [[nodiscard]] constexpr Version& reset_minor(unsigned int n) noexcept {
            return minor(n).patch(0).prerelease("").build("");
        }
        [[nodiscard]] constexpr Version& reset_patch(unsigned int n) noexcept {
            return patch(n).prerelease("").build("");
        }
        [[nodiscard]] constexpr Version& reset_prerelease(std::string_view n) noexcept {
            return prerelease(n).build("");
        }

        [[nodiscard]] constexpr Version& inc_major() noexcept { return reset_major(major()+1); }
        [[nodiscard]] constexpr Version& inc_minor() noexcept { return reset_minor(minor()+1); }
        [[nodiscard]] constexpr Version& inc_patch() noexcept { return reset_patch(patch()+1); }

        [[nodiscard]] constexpr int compare(Version const& b) const noexcept {
            constexpr int const LESS = -1;
            constexpr int const GREATER = 1;
            constexpr int const EQUAL = 0;

            if (major() < b.major() ||
                minor() < b.minor() ||
                patch() < b.patch()) return LESS;
            if (major() > b.major() ||
                minor() > b.minor() ||
                patch() > b.patch()) return GREATER;
            
            if (prerelease_v() == b.prerelease_v()) return EQUAL;
            
            if (pre_len == 0 && b.pre_len != 0) return GREATER;
            if (pre_len != 0 && b.pre_len == 0) return LESS;
            // both have a prerelease, lex compare it for the most part

            auto begin1 = prerelease_v().begin();
            auto end1 = prerelease_v().end();
            auto rstart1 = begin1;
            auto lag1 = rstart1;
            auto begin2 = b.prerelease_v().begin();
            auto end2 = b.prerelease_v().end();
            auto rstart2 = begin2;
            auto lag2 = rstart2;

            for (; lag1 != end1 && lag2 != end2; rstart1 = ++begin1, rstart2 = ++begin2) { // TODO: maybe get rid of lag somehow?
                while (begin1 != end1 && *begin1 != '.') ++begin1;
                while (begin2 != end2 && *begin2 != '.') ++begin2;

                {
                    std::string_view sv1 {rstart1, static_cast<size_t>(begin1 - rstart1)};
                    std::string_view sv2 {rstart2, static_cast<size_t>(begin2 - rstart2)};

                    // first, check if the section is numeric
                    bool a_num = BeatMods::algorithm::is_numeric(sv1);
                    bool b_num = BeatMods::algorithm::is_numeric(sv2);

                    if (a_num && !b_num) return LESS;
                    if (!a_num && b_num) return GREATER;

                    if (a_num && b_num) {
                        auto an = BeatMods::algorithm::parse_int<unsigned int>(sv1);
                        auto bn = BeatMods::algorithm::parse_int<unsigned int>(sv2);

                        if (an < bn) return LESS;
                        if (an > bn) return GREATER;
                    } else {
                        if (sv1 < sv2) return LESS;
                        if (sv1 > sv2) return GREATER;
                    }
                }

                lag1 = begin1;
                lag2 = begin2;
            }

            // if 1 finished first, 1 is less
            if (lag1 == end1 && lag2 != end2) return LESS;
            if (lag1 != end1 && lag2 == end2) return GREATER;

            return EQUAL;
        }

        template<typename OStream>
        friend OStream& operator<<(OStream& os, Version const& v) {
            os << v.major() << "."
            << v.minor() << "."
            << v.patch();
            if (v.pre_len > 0)
                os << "-" << v.prerelease_v();
            if (v.build_len > 0)
                os << "+" << v.build_v();
            return os;
        }

        friend void swap(Version& a, Version& b) {
            std::swap(a.major_, b.major_);
            std::swap(a.minor_, b.minor_);
            std::swap(a.patch_, b.patch_);
            std::swap(a.pre_len, b.pre_len);
            std::swap(a.build_len, b.build_len);

            char prec[Version::bufsize] = {0};
            char buildc[Version::bufsize] = {0};

            Version::_copy(a.prerelease_, prec);
            Version::_copy(a.build_, buildc);
            Version::_copy(b.prerelease_, a.prerelease_);
            Version::_copy(b.build_, a.build_);
            Version::_copy(prec, a.prerelease_);
            Version::_copy(buildc, b.prerelease_);
        }

        friend constexpr bool operator==(Version const& a, Version const& b) {
            return a.compare(b) == 0;
        }
        friend constexpr bool operator<(Version const& a, Version const& b) {
            return a.compare(b) < 0;
        }
        friend constexpr bool operator>(Version const& a, Version const& b) {
            return a.compare(b) > 0;
        }
        friend constexpr bool operator<=(Version const& a, Version const& b) {
            return !(a > b);
        }
        friend constexpr bool operator>=(Version const& a, Version const& b) {
            return !(a < b);
        }
    };
    
    // static tests (these are pulled straight from section 11 of the semver spec)
    static_assert(Version{"1.0.0-alpha"} < Version{"1.0.0-alpha.1"});
    static_assert(Version{"1.0.0-alpha.1"} < Version{"1.0.0-alpha.beta"});
    static_assert(Version{"1.0.0-alpha.beta"} < Version{"1.0.0-beta"});
    static_assert(Version{"1.0.0-beta"} < Version{"1.0.0-beta.2"});
    static_assert(Version{"1.0.0-beta.2"} < Version{"1.0.0-beta.11"});
    static_assert(Version{"1.0.0-beta.11"} < Version{"1.0.0-rc.1"});
    static_assert(Version{"1.0.0-rc.1"} < Version{"1.0.0"});

    static_assert(Version{"1.0.0-alpha.1"} > Version{"1.0.0-alpha"});
    static_assert(Version{"1.0.0-alpha.beta"} > Version{"1.0.0-alpha.1"});
    static_assert(Version{"1.0.0-beta"} > Version{"1.0.0-alpha.beta"});
    static_assert(Version{"1.0.0-beta.2"} > Version{"1.0.0-beta"});
    static_assert(Version{"1.0.0-beta.11"} > Version{"1.0.0-beta.2"});
    static_assert(Version{"1.0.0-rc.1"} > Version{"1.0.0-beta.11"});
    static_assert(Version{"1.0.0"} > Version{"1.0.0-rc.1"});

    class Range { // TODO: really need to bench this to see if parsing can be improved...
        enum class op {
            greater, less, greater_equal, less_equal, exact
        };

        struct part {
            Version version;
            op comparator;
        };

        struct bounds {
            part lower = {{}, op::exact};
            part upper = {{}, op::exact};
        };

        // because of this, this cannot be constexpr unfortunately
        std::vector<bounds> parts;

    public:
        inline Range() = default;
        explicit Range(std::string_view toParse);

        [[nodiscard]] bool in_range(Version const&) const noexcept;
    };

}

#endif