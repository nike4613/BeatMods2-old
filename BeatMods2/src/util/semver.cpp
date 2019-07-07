#include "util/semver.h"
#include "util/algorithm.h"
#include "util/macro.h"
#include <algorithm>

using namespace BeatMods::semver;
using namespace BeatMods::algorithm;

Range::Range(std::string_view toParse) {
    auto last = &parts.emplace_back();
    split_equals(toParse.begin(), toParse.end(), ' ', 
        [&](char const* begin, char const* end) { // takes iterator types of string_view
            if (begin == end) return; // ignore empty segments
            std::string_view sv {begin, static_cast<size_t>(end-begin)};

            if (sv == "||") { // for or operators
                last = &parts.emplace_back();
                return;
            }

            sassert(sv.size() >= 5);

            part cpart;
            int len = 0;

            auto get_ver = [&](size_t len) {
                return Version{remove_prefix(sv, len)};
            };

            switch (sv[0]) {
                case '>':
                    if (sv[1] == '=') {
                        cpart.comparator = op::greater_equal;
                        len = 2;
                    } else {
                        cpart.comparator = op::greater;
                        len = 1;
                    }
                    cpart.version = get_ver(len);
                    if ((last->lower.comparator != op::greater_equal && last->lower.comparator != op::greater) ||
                        last->lower.version > cpart.version ||
                        (last->lower.version == cpart.version && cpart.comparator == op::greater_equal)) 
                        last->lower = cpart;
                    break;
                case '<': 
                    if (sv[1] == '=') {
                        cpart.comparator = op::less_equal;
                        len = 2;
                    } else {
                        cpart.comparator = op::less;
                        len = 1;
                    }
                    cpart.version = get_ver(len);
                    if ((last->upper.comparator != op::less_equal && last->upper.comparator != op::less) ||
                        last->upper.version < cpart.version ||
                        (last->upper.version == cpart.version && cpart.comparator == op::less_equal)) 
                        last->upper = cpart;
                    break;
                case '^': // equivalent to >=(a).(b).(c) <(a+1).0.0
                    len = 1;
                    cpart.comparator = op::greater_equal;
                    cpart.version = get_ver(len);
                    last->lower = cpart; // force state

                    cpart.comparator = op::less;
                    cpart.version = cpart.version.major() == 0 ? 
                                        cpart.version.inc_minor() :
                                        cpart.version.inc_major();
                    last->upper = cpart;
                    break;
                case '=': len = 1;
                default: 
                    cpart.comparator = op::exact; 
                    cpart.version = get_ver(len);
                    last->lower = cpart; // force state
                    last->upper = cpart; // force state
                    break;
            }
        });
}

bool Range::in_range(Version const& ver) const noexcept {
    auto matches_part = [](part const& part, Version const& version) {
        switch (part.comparator) {
            case op::exact:
                return part.version == version;
            case op::greater:
                return version > part.version;
            case op::greater_equal:
                return version >= part.version;
            case op::less:
                return version < part.version;
            case op::less_equal:
                return version <= part.version;
        }
        return false;
    };

    for (auto const& bound : parts) {
        if (matches_part(bound.lower, ver) && matches_part(bound.upper, ver))
            return true;
    }
    return false;
}