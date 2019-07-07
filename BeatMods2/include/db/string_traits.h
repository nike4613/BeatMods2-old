#ifndef BEATMODS2_DB_STRING_TRAITS_H
#define BEATMODS2_DB_STRING_TRAITS_H

#include "db/engine.h"
#include "util/json.h"
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

// TODO: split this into a source file with definitions here, but impls in the source file
// like engine.h/engine.cpp

namespace pqxx {
    template<> struct string_traits<BeatMods::db::TimeStamp>
    {
        static constexpr const char* date_format() noexcept { return "%Y-%m-%d %T%z"; }
        static constexpr const char *name() noexcept { return "BeatMods::db::TimeStamp"; }
        static constexpr bool has_null() noexcept { return false; }
        static bool is_null(BeatMods::db::TimeStamp) { return false; }
        [[noreturn]] static BeatMods::db::TimeStamp null()
        { internal::throw_null_conversion(name()); }
        static void from_string(const char Str[], BeatMods::db::TimeStamp& Obj) 
        { 
            std::istringstream stream {Str};
            stream >> date::parse(date_format(), Obj);
        }
        static std::string to_string(BeatMods::db::TimeStamp Obj) 
        { return date::format(date_format(), Obj); }
    };

    template<typename TR, typename TU> 
    struct string_traits<BeatMods::db::foreign_key<TR, TU>>
    {
        static constexpr const char *name() noexcept 
        { return "BeatMods::db::foreign_key"; }
        static constexpr bool has_null() noexcept { return false; }
        static bool is_null(BeatMods::db::foreign_key<TR, TU>) { return false; }
        [[noreturn]] static BeatMods::db::foreign_key<TR, TU> null()
        { internal::throw_null_conversion(name()); }
        static void from_string(const char Str[], BeatMods::db::foreign_key<TR, TU>& Obj) 
        { TU val; pqxx::string_traits<TU>::from_string(Str, val); Obj = val; }
        static std::string to_string(BeatMods::db::foreign_key<TR, TU> const& Obj) 
        { return pqxx::to_string(BeatMods::db::foreign_key_id(Obj)); }
    };

    template<typename T> 
    struct string_traits<std::vector<T>>
    {
        static constexpr const char *name() noexcept 
        { return "std::vector"; }
        static constexpr bool has_null() noexcept { return false; }
        static bool is_null(std::vector<T> const&) { return false; }
        [[noreturn]] static std::vector<T> null()
        { internal::throw_null_conversion(name()); }
        static void from_string(const char Str[], std::vector<T>& Obj) 
        { 
            Obj.clear();
            pqxx::array_parser parser {Str};

            auto value = parser.get_next();
            int depth = 0;
            while (value.first != pqxx::array_parser::juncture::done)
            {
                switch (value.first) 
                {
                case pqxx::array_parser::juncture::row_start: ++depth; break;
                case pqxx::array_parser::juncture::row_end: --depth; break;
                    // TODO: do something about nested arrays
                case pqxx::array_parser::juncture::null_value:
                    Obj.push_back(pqxx::string_traits<T>::null());
                    break;
                case pqxx::array_parser::juncture::string_value:
                    T& val = Obj.emplace_back();
                    pqxx::string_traits<T>::from_string(value.second.c_str(), val);
                    break;
                }
                value = parser.get_next();
            }
        }
        static std::string to_string(std::vector<T> const& Obj) 
        {
            std::ostringstream str{"{"};
            bool first = true;
            for (auto const& val : Obj) {
                if (!first) str << ",";
                first = false;
                auto sval = pqxx::string_traits<T>::to_string(val);
                // TODO: just use connection_base::quote()
                //       but that means figuring out how to get a connection
                //         object in here....
                str << '\'' << sval << '\'';
            }
            str << "}";
            return str.str();
        }
    };

    template<> struct string_traits<BeatMods::db::Version>
    {
        static constexpr const char *name() noexcept { return "BeatMods::db::Version"; }
        static constexpr bool has_null() noexcept { return false; }
        static bool is_null(BeatMods::db::Version const&) { return false; }
        [[noreturn]] static BeatMods::db::Version null()
        { internal::throw_null_conversion(name()); }
        static void from_string(const char Str[], BeatMods::db::Version& Obj) 
        { 
            auto const len = std::strlen(Str);
            char copy[len+1]; // TODO: see if i actually need heap allocations because of stack size limitations
            std::memcpy(copy, Str, len);
            // copy now contains the string data from Str, and is on the stack
            // this lets me modify it, to have a series of c-strings to pass to 
            //   from_string

            struct {
                unsigned int major; 
                unsigned int minor; 
                unsigned int patch; 
                std::string prerelease;
                std::string build;
            } vers;

            int str_idx = 0;
            int str_front = -1;
            for (size_t i = 0; i < len; ++i) {
                switch (copy[i]) {
                    case '(':
                    case ')':
                    case ',':
                        copy[i] = 0;

                        if (str_front >= 0)
                        switch (str_idx++) {
                            case 0: // major
                                pqxx::from_string(&copy[str_front+1], vers.major);
                                break;
                            case 1: // minor
                                pqxx::from_string(&copy[str_front+1], vers.minor);
                                break;
                            case 2: // patch
                                pqxx::from_string(&copy[str_front+1], vers.patch);
                                break;
                            case 3:
                                pqxx::from_string(&copy[str_front+1], vers.prerelease);
                                break;
                            case 4:
                                pqxx::from_string(&copy[str_front+1], vers.build);
                                break;
                        }

                        str_front = i; // str_front will be the null before
                        continue;
                    default: break;
                }
            }

            Obj = BeatMods::db::Version{vers.major, vers.minor, vers.patch, vers.prerelease, vers.build};
        }
        static std::string to_string(BeatMods::db::Version const& Obj) 
        {
            std::ostringstream strm{"("};
            strm << pqxx::to_string(Obj.major()) << ","
                 << pqxx::to_string(Obj.minor()) << ","
                 << pqxx::to_string(Obj.patch()) << ","
                 << pqxx::to_string(Obj.prerelease()) << "," // TODO: fix these 2 to encode properly
                 << pqxx::to_string(Obj.build()) << ")";
            return strm.str();
        }
    };

    template<> struct string_traits<BeatMods::db::ModRange>
    {
        static constexpr const char *name() noexcept { return "BeatMods::db::ModRange"; }
        static constexpr bool has_null() noexcept { return false; }
        static bool is_null(BeatMods::db::ModRange const&) { return false; }
        [[noreturn]] static BeatMods::db::ModRange null()
        { internal::throw_null_conversion(name()); }
        static void from_string(const char Str[], BeatMods::db::ModRange& Obj) 
        { 
            auto const len = std::strlen(Str);
            char copy[len+1]; // TODO: see if i actually need heap allocations because of stack size limitations
            std::memcpy(copy, Str, len);
            // copy now contains the string data from Str, and is on the stack
            // this lets me modify it, to have a series of c-strings to pass to 
            //   from_string

            int str_idx = 0;
            int str_front = -1;
            for (size_t i = 0; i < len; ++i) {
                switch (copy[i]) {
                    case '(':
                    case ')':
                    case ',':
                        copy[i] = 0;

                        if (str_front >= 0) {
                            if (copy[str_front+1] == '"' || copy[str_front+1] == '\'') {
                                copy[++str_front] = 0;
                                copy[i-1] = 0; // delete quotes from string
                            }

                            switch (str_idx++) {
                                case 0: // id
                                    std::strcpy(Obj.id, &copy[str_front+1]);
                                    break;
                                case 1: // verrange
                                    std::strcpy(Obj.versionRange, &copy[str_front+1]);
                                    break;
                            }
                        }

                        str_front = i; // str_front will be the null before
                        continue;
                }
            }
        }
        static std::string to_string(BeatMods::db::ModRange const& Obj) 
        {
            std::ostringstream strm{"("};
            strm << "'" << pqxx::to_string(Obj.id) << "'," // TODO: probably quote these properly, perhaps don't need it bc of requirements of id and range
                 << "'" << pqxx::to_string(Obj.versionRange) << "')";
            return strm.str();
        }
    };

    template<> struct string_traits<BeatMods::db::JSON>
    {
        static constexpr const char *name() noexcept { return "BeatMods::db::JSON"; }
        static constexpr bool has_null() noexcept { return false; }
        static bool is_null(BeatMods::db::JSON const&) { return false; }
        [[noreturn]] static BeatMods::db::JSON null()
        { internal::throw_null_conversion(name()); }
        static void from_string(const char Str[], BeatMods::db::JSON& Obj) 
        { 
            Obj.Clear();
            const auto len = std::strlen(Str);
            char stackCopy[len+1]; // TODO: see if i actually need heap allocations because of stack size limitations
            std::memcpy(stackCopy, Str, len);

            Obj.ParseInsitu(stackCopy);
        }
        static std::string to_string(BeatMods::db::JSON const& Obj) 
        {
            std::ostringstream out;
            rapidjson::OStreamWrapper wrap {out};
            rapidjson::Writer<rapidjson::OStreamWrapper> write {wrap};
            Obj.Accept(write);
            return out.str();
        }
    };

    #define ENUM_FROM_STRING(type, value, inp, outp) if (std::string{inp} == #value) outp = type::value;
    template<> struct string_traits<BeatMods::db::System>
    {
        static constexpr const char *name() noexcept { return "BeatMods::db::System"; }
        static constexpr bool has_null() noexcept { return false; }
        static bool is_null(BeatMods::db::System) { return false; }
        [[noreturn]] static BeatMods::db::System null()
        { internal::throw_null_conversion(name()); }
        static void from_string(const char Str[], BeatMods::db::System& Obj) 
        { 
            ENUM_FROM_STRING(BeatMods::db::System, PC, Str, Obj)
            else ENUM_FROM_STRING(BeatMods::db::System, Quest, Str, Obj)
            else Obj = BeatMods::db::System::PC; // default 
        }
        static std::string to_string(BeatMods::db::System Obj) 
        { return std::to_string(Obj); }
    };

    template<> struct string_traits<BeatMods::db::Visibility>
    {
        static constexpr const char *name() noexcept { return "BeatMods::db::Visibility"; }
        static constexpr bool has_null() noexcept { return false; }
        static bool is_null(BeatMods::db::Visibility) { return false; }
        [[noreturn]] static BeatMods::db::Visibility null()
        { internal::throw_null_conversion(name()); }
        static void from_string(const char Str[], BeatMods::db::Visibility& Obj) 
        { 
            ENUM_FROM_STRING(BeatMods::db::Visibility, Public, Str, Obj)
            else ENUM_FROM_STRING(BeatMods::db::Visibility, Groups, Str, Obj)
            else Obj = BeatMods::db::Visibility::Public; // default 
        }
        static std::string to_string(BeatMods::db::Visibility Obj) 
        { return std::to_string(Obj); }
    };

    template<> struct string_traits<BeatMods::db::DownloadType>
    {
        static constexpr const char *name() noexcept { return "BeatMods::db::DownloadType"; }
        static constexpr bool has_null() noexcept { return false; }
        static bool is_null(BeatMods::db::DownloadType) { return false; }
        [[noreturn]] static BeatMods::db::DownloadType null()
        { internal::throw_null_conversion(name()); }
        static void from_string(const char Str[], BeatMods::db::DownloadType& Obj) 
        { 
            ENUM_FROM_STRING(BeatMods::db::DownloadType, Steam, Str, Obj)
            else ENUM_FROM_STRING(BeatMods::db::DownloadType, Oculus, Str, Obj)
            else ENUM_FROM_STRING(BeatMods::db::DownloadType, Universal, Str, Obj)
            else Obj = BeatMods::db::DownloadType::Universal; // default 
        }
        static std::string to_string(BeatMods::db::DownloadType Obj) 
        { return std::to_string(Obj); }
    };

    template<> struct string_traits<BeatMods::db::Approval>
    {
        static constexpr const char *name() noexcept { return "BeatMods::db::Approval"; }
        static constexpr bool has_null() noexcept { return false; }
        static bool is_null(BeatMods::db::Approval) { return false; }
        [[noreturn]] static BeatMods::db::Approval null()
        { internal::throw_null_conversion(name()); }
        static void from_string(const char Str[], BeatMods::db::Approval& Obj) 
        { 
            ENUM_FROM_STRING(BeatMods::db::Approval, Approved, Str, Obj)
            else ENUM_FROM_STRING(BeatMods::db::Approval, Declined, Str, Obj)
            else ENUM_FROM_STRING(BeatMods::db::Approval, Pending, Str, Obj)
            else ENUM_FROM_STRING(BeatMods::db::Approval, Inactive, Str, Obj)
            else Obj = BeatMods::db::Approval::Inactive; // default 
        }
        static std::string to_string(BeatMods::db::Approval Obj) 
        { return std::to_string(Obj); }
    };

    template<> struct string_traits<BeatMods::db::Permission>
    {
        static constexpr const char *name() noexcept { return "BeatMods::db::Permission"; }
        static constexpr bool has_null() noexcept { return false; }
        static bool is_null(BeatMods::db::Permission) { return false; }
        [[noreturn]] static BeatMods::db::Permission null()
        { internal::throw_null_conversion(name()); }
        static void from_string(const char Str[], BeatMods::db::Permission& Obj) 
        { 
            #define M(x) else if (std::string{Str} == #x) Obj = BeatMods::db::Permission::x;
            if (false) {} // empty to make macro easier
            PERMISSIONS(M)
            #undef M
        }
        static std::string to_string(BeatMods::db::Permission Obj) 
        { return std::to_string(Obj); }
    };
}

#endif