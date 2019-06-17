#ifndef BEATMODS2_UTIL_JSON_H
#define BEATMODS2_UTIL_JSON_H

#define RAPIDJSON_HAS_STDSTRING 1

#include <type_traits>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <rapidjson/reader.h>
#include <rapidjson/writer.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <string>
#include <string_view>
#include <cassert>

namespace BeatMods::json {
    
    enum class Endianness {
        Big, Little, Default, Network = Big
    };

    template<typename T> struct sfinae_fail {};

    template<typename, Endianness = Endianness::Default, bool BOM = false> struct encoding_for_char_t;
    template<typename Ch, Endianness E>
    struct encoding_for_char_t<Ch, E, false> {
        static_assert(std::is_integral_v<Ch>);
        static constexpr int size = sizeof(Ch);
        using type = 
            std::conditional_t<size == 1, rapidjson::UTF8<Ch>,
            std::conditional_t<size == 2, rapidjson::UTF16<Ch>,
            std::conditional_t<size == 4, rapidjson::UTF32<Ch>,
            sfinae_fail<Ch>>>>;
    };
    template<typename Ch>
    struct encoding_for_char_t<Ch, Endianness::Default, true> {
        static_assert(std::is_integral_v<Ch>);
        using type = typename encoding_for_char_t<Ch, Endianness::Default, false>::type;
    };
    template<typename Ch>
    struct encoding_for_char_t<Ch, Endianness::Big, true> {
        static_assert(std::is_integral_v<Ch>);
        static constexpr int size = sizeof(Ch);
        using type =
            std::conditional_t<size == 1, rapidjson::UTF8<Ch>,
            std::conditional_t<size == 2, rapidjson::UTF16BE<Ch>,
            std::conditional_t<size == 4, rapidjson::UTF32BE<Ch>,
            sfinae_fail<Ch>>>>;
    };
    template<typename Ch>
    struct encoding_for_char_t<Ch, Endianness::Little, true> {
        static_assert(std::is_integral_v<Ch>);
        static constexpr int size = sizeof(Ch);
        using type =
            std::conditional_t<size == 1, rapidjson::UTF8<Ch>,
            std::conditional_t<size == 2, rapidjson::UTF16LE<Ch>,
            std::conditional_t<size == 4, rapidjson::UTF32LE<Ch>,
            sfinae_fail<Ch>>>>;
    };

    template<typename Ch, Endianness E = Endianness::Default, bool BOM = false>
    using encoding_for_char = typename encoding_for_char_t<Ch, E, BOM>::type;

    template<typename Ch = char, typename Allocator = rapidjson::MemoryPoolAllocator<>>
    std::basic_string_view<Ch> get_string(rapidjson::GenericValue<encoding_for_char<Ch>, Allocator> const& value)
    {
        assert(value.IsString());
        return {value.GetString(), value.GetStringLength()};
    }

}

#endif