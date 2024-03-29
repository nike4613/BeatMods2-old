#ifndef BEATMODS2_DB_ENGINE_H
#define BEATMODS2_DB_ENGINE_H

#include <pqxx/pqxx>
#include <cstddef>
#include <variant>
#include <chrono>
#include <vector>
#include <optional>
#include <future>
#include <date/date.h>
#include <date/tz.h>
#include <exception>
#include <typeinfo>
#include <cstring>
#include <numeric>
#include <algorithm>
#include "util/json.h"
#include "util/semver.h"
#include "util/macro.h"
#include "util/thread.h"

namespace BeatMods::db {

    constexpr int mod_id_length = 128;

    using UUID = std::string; // just a string containing the uuid
    using Integer = int32_t;
    using BigInteger = int64_t;

    using Version = BeatMods::semver::Version;
    using TimeStamp = std::chrono::system_clock::time_point;
    using JSON = rapidjson::Document;

    template<typename TResolved, typename TUnresolved>
    using foreign_key = std::variant<std::shared_ptr<TResolved>, TUnresolved>;

    // all of these throw if the variant is not the right type
    template<typename TR, typename TU>
    [[nodiscard]] constexpr auto get_resolved(foreign_key<TR, TU>& ref) { return std::get<std::shared_ptr<TR>>(ref); }
    template<typename TR, typename TU>
    [[nodiscard]] constexpr auto get_resolved(foreign_key<TR, TU> const& ref) { return std::get<std::shared_ptr<TR>>(ref); }
    template<typename TR, typename TU>
    [[nodiscard]] constexpr auto get_unresolved(foreign_key<TR, TU>& ref) { return std::get<TU>(ref); }
    template<typename TR, typename TU>
    [[nodiscard]] constexpr auto get_unresolved(foreign_key<TR, TU> const& ref) { return std::get<TU>(ref); }

    // these, however, do not    
    template<typename TR, typename TU>
    [[nodiscard]] constexpr auto get_resolved_if(foreign_key<TR, TU> const* ref) noexcept { return std::get_if<std::shared_ptr<TR>>(ref); }
    template<typename TR, typename TU>
    [[nodiscard]] constexpr auto get_resolved_if(foreign_key<TR, TU>* ref) noexcept { return std::get_if<std::shared_ptr<TR>>(ref); }
    template<typename TR, typename TU>
    [[nodiscard]] constexpr auto get_unresolved_if(foreign_key<TR, TU> const* ref) noexcept { return std::get_if<TU>(ref); }
    template<typename TR, typename TU>
    [[nodiscard]] constexpr auto get_unresolved_if(foreign_key<TR, TU>* ref) noexcept { return std::get_if<TU>(ref); }
    
    template<typename TR, typename TU>
    [[nodiscard]] constexpr auto is_resolved(foreign_key<TR, TU> const& ref) noexcept { return std::holds_alternative<std::shared_ptr<TR>>(ref); }
    template<typename TR, typename TU>
    [[nodiscard]] constexpr auto is_unresolved(foreign_key<TR, TU> const& ref) noexcept { return std::holds_alternative<TU>(ref); }

    template<typename T>
    struct foreign_key_resolved {
        using type = std::remove_reference_t<decltype(*std::variant_alternative_t<0, T>{})>;
    };
    template<typename T>
    struct foreign_key_unresolved {
        using type = std::variant_alternative_t<1, T>;
    };

    template<typename T>
    using foreign_key_resolved_t = typename foreign_key_resolved<T>::type;
    template<typename T>
    using foreign_key_unresolved_t = typename foreign_key_unresolved<T>::type;

    template<typename T>
    using nullable = std::optional<T>;

    template<typename T>
    struct is_nullable : std::false_type {};
    template<typename T>
    struct is_nullable<nullable<T>> : std::true_type {};

    template<typename T>
    inline constexpr bool is_nullable_v = is_nullable<T>::value;

    enum class Approval {
        Approved, Declined, Pending, Inactive
    };

    enum class CompareOp {
        Greater, GreaterEqual, Equal, LessEqual, Less // different order than in DB; may need changing
    };

    enum class DownloadType {
        Steam, Oculus, Universal
    };

    struct ModRange {
        char id[mod_id_length]; // valid because DB type is varchar(128)
        char versionRange[64]; // valid because DB type is varchar(64)
        // TODO: add a parsed version that has useful representations of version range etc
    };

    #define _PERMISSIONS(X) \
        X(gameversion_add)  X(gameversion_edit) X(mod_create) X(mod_edit) \
        X(mod_reposess) X(user_delete) X(group_add) X(group_edit) X(group_delete) \
        X(mod_see_pending) X(mod_approve_deny) X(user_edit_groups) X(news_edit) X(news_add) \
        X(mod_upload_as)
    #define PERMISSIONS(X) EXEC(_PERMISSIONS(X))
    enum class Permission {
        #define M(n) n,
        PERMISSIONS(M)
        #undef M
    };

    enum class System {
        PC, Quest
    };

    enum class Visibility {
        Public, Groups
    };

    // Tables //

    template<typename T, typename = void>
    struct is_table : std::false_type {};
    template<typename T>
    struct is_table<T, std::void_t<
        std::enable_if_t<std::is_same_v<typename T::request::_type, T>>
    >> : std::true_type {};
    template<typename T>
    constexpr auto is_table_v = is_table<T>::value;

    template<typename T, typename = void>
    struct is_table_request : std::false_type {};
    template<typename T>
    struct is_table_request<T, std::void_t<
        std::enable_if_t<std::is_same_v<typename T::_type::request, T>>
    >> : std::true_type {};
    template<typename T>
    constexpr auto is_table_request_v = is_table_request<T>::value;

    struct User {
        bool id_valid = false;
        UUID id; // PKey (when creating, this is never used; instead DB default is)
        std::string name; // Unique
        std::string profile;
        TimeStamp created;
        std::string githubId;

        static constexpr char const* table = "\"mod-repo\".\"Users\"";
        struct request {
            using _type = User;
            bool id:1; bool name:1; bool profile:1; bool created:1; bool githubId:1;
        };
    };
    

    struct GameVersion {
        bool id_valid = false;
        UUID id; // PKey (when creating, this is never used; instead DB default is)
        Version version;
        BigInteger steamBuildId;
        Visibility visibility;

        static constexpr char const* table = "\"mod-repo\".\"GameVersions\"";
        struct request {
            using _type = GameVersion;
            bool id:1; bool version:1; bool steamBuildId:1;
            bool visibility:1;
        };
    };

    struct Mod {
        bool id_valid = false;
        UUID uuid; // PKey (when creating, this is never used; instead DB default is)
        std::string id;
        std::string name;
        std::string description;
        foreign_key<User, UUID> author; // FKey Users.id
        Version version;
        foreign_key<GameVersion, UUID> gameVersion; // FKey GameVersions.id
        System system;
        TimeStamp uploaded;
        Approval approvalState;
        nullable<TimeStamp> approved;
        bool required;
        std::vector<ModRange> dependsOn;
        std::vector<ModRange> conflictsWith;

        static constexpr char const* table = "\"mod-repo\".\"Mods\"";
        struct request {
            using _type = Mod;
            bool uuid:1; bool id:1; bool name:1; bool description:1;
            bool author:1; bool author_resolve:1; 
            bool version:1;
            bool gameVersion:1; bool gameVersion_resolve:1;
            bool system:1; bool uploaded:1; bool approvalState:1;
            bool approved:1; bool required:1; bool dependsOn:1;
            bool conflictsWith:1;

            User::request author_request;
            GameVersion::request gameVersion_request;
        };
    };

    struct Download {
        foreign_key<Mod, UUID> mod; // PKey, FKey Mods.uuid
        DownloadType type;          // PKey
        std::string cdnFile;
        JSON hashes;

        static constexpr char const* table = "\"mod-repo\".\"Downloads\"";
        struct request {
            using _type = Download;
            bool mod:1; bool mod_resolve:1; 
            bool type:1; bool cdnFile:1;
            bool hashes:1;

            Mod::request mod_request;
        };
    };

    struct Group {
        bool id_valid = false;
        BigInteger id; // PKey (when creating, this is never used; instead DB default is)
        std::string name;
        std::vector<Permission> permissions;

        static constexpr char const* table = "\"mod-repo\".\"Groups\"";
        struct request {
            using _type = Group;
            bool id:1; bool name:1; bool permissions:1;
        };
    };

    struct Tag {
        bool id_valid = false;
        BigInteger id; // PKey (when creating, this is never used; instead DB default is)
        std::string name;

        static constexpr char const* table = "\"mod-repo\".\"Tags\"";
        struct request {
            using _type = Tag;
            bool id:1; bool name:1;
        };
    };

    struct NewsItem {
        bool id_valid = false;
        UUID id; // PKey (when creating, this is never used; instead DB default is)
        std::string title;
        foreign_key<User, UUID> author; // FKey Users.id
        std::string body;
        TimeStamp posted;
        nullable<TimeStamp> edited;
        nullable<System> system;

        static constexpr char const* table = "\"mod-repo\".\"News\"";
        struct request {
            using _type = NewsItem;
            bool id:1; bool title:1; 
            bool author:1; bool author_resolve:1;
            bool body:1; bool posted:1; bool edited:1;
            bool system:1;

            User::request author_request;
        };
    };

    struct GameVersion_VisibleGroups_JoinItem {
        foreign_key<GameVersion, UUID> gameVersion; // PKey, FKey GameVersions.id
        foreign_key<Group, BigInteger> group;       // PKey, FKey Groups.id 

        static constexpr char const* table = "\"mod-repo\".\"GameVersion_VisibleGroups_Joiner\"";
        struct request {
            using _type = GameVersion_VisibleGroups_JoinItem;
            bool gameVersion:1; bool gameVersion_resolve:1;
            bool group:1; bool group_resolve:1;

            GameVersion::request gameVersion_request;
            Group::request group_request;
        };
    };

    struct Mods_Tags_JoinItem {
        foreign_key<Mod, UUID>       mod; // PKey, FKey Mods.uuid
        foreign_key<Tag, BigInteger> tag; // PKey, FKey Tags.id
        
        static constexpr char const* table = "\"mod-repo\".\"Mods_Tags_Joiner\"";
        struct request {
            using _type = Mods_Tags_JoinItem;
            bool mod:1; bool mod_resolve:1;
            bool tag:1; bool tag_resolve:1;

            Mod::request mod_request;
            Tag::request tag_request;
        };
    };

    struct Users_Groups_JoinItem {
        foreign_key<User, UUID>        user;  // PKey, FKey Users.id
        foreign_key<Group, BigInteger> group; // PKey, FKey Groups.id
        
        static constexpr char const* table = "\"mod-repo\".\"Users_Groups_Joiner\"";
        struct request {
            using _type = Users_Groups_JoinItem;
            bool user:1; bool user_resolve:1;
            bool group:1; bool group_resolve:1;

            User::request user_request;
            Group::request group_request;
        };
    };

    namespace state {
        enum class LogLevel {
            Debug, Warning, Error, Fatal, Info, Security
        };

        struct LogItem {
            bool id_valid = false;
            BigInteger id;
            TimeStamp time;
            std::string message;
            LogLevel level;

            static constexpr char const* table = "\"server-state\".\"Log\"";
            struct request {
                using _type = LogItem;
                bool id:1; bool time:1; 
                bool message:1; bool level:1;
            };
        };

        struct Token {
            foreign_key<User, UUID> user;
            std::string token;
            
            static constexpr char const* table = "\"server-state\".\"Tokens\"";
            struct request {
                using _type = LogItem;
                bool user:1; bool user_resolve:1;
                bool token:1;
                
                User::request user_request;
            };
        };
    }

    // Accessors // 

    enum class PgCompareOp {
        Not = 1<<0,
        Less = 1<<1,
        Greater = 1<<2,
        Equal = 1<<3,
        LessEqual = Less|Equal,
        GreaterEqual = Greater|Equal,
        NotLess = GreaterEqual,
        NotGreater = LessEqual,
        NotLessEqual = Greater,
        NotGreaterEqual = Less,
        NotEqual = Not|Equal,

        Distinct = 1<<4,
        NotDistinct = Not|Distinct,
        Null = 1<<5,
        NotNull = Not|Null,

        Like = 1<<6,
        NotLike = Not|Like,
        ILike = 1<<12,
        NotILike = Not|ILike,
        Similar = 1<<7,
        NotSimilar = Not|Similar,
        PosixRE = 1<<8, // always case insensitive match version74
        NotPosixRE = Not|PosixRE,

        True = 1<<9,
        False = 1<<10,
        Unknown = 1<<11,
        NotTrue = Not|True,
        NotFalse = Not|False,
        NotUnknown = Not|Unknown
    };

    [[nodiscard]] std::string to_pg_op(PgCompareOp);

    template<typename T>
    struct id_type { using type = decltype(T{}.id); };
    template<>
    struct id_type<Mod> { using type = decltype(Mod{}.uuid); };

    template<typename T>
    using id_type_t = typename id_type<T>::type;

    // TODO: when we get constexpr bit_cast, re-add this
    /*template<typename TR>
    constexpr auto all_fields() -> std::enable_if_t<is_table_request_v<TR>, TR> {
        uint8_t data[sizeof(TR)] = {0xFF};
        return *reinterpret_cast<TR*>(data);
    }*/

    template<typename T>
    struct _request_instantiator {
        static std::vector<std::shared_ptr<T>> lookup(
            pqxx::transaction_base& transaction,
            typename T::request returnFields, 
            typename T::request searchFields = {}, 
            T const* searchValues = nullptr,
            PgCompareOp compareOp = PgCompareOp::Equal,
            std::string_view additionalClauses = "");
        static size_t insert(
            pqxx::transaction_base& transaction,
            std::vector<std::shared_ptr<T>>& values);
        static void update(
            pqxx::transaction_base& transaction, 
            T const* values,
            typename T::request updateFields/* = all_fields<typename T::request>()*/);
     };

    template<typename T>
    auto lookup(
        pqxx::transaction_base& transaction,
        typename T::request returnFields,
        std::string_view additionalClauses = "", 
        typename T::request searchFields = {}, 
        T const* searchValues = nullptr,
        PgCompareOp compareOp = PgCompareOp::Equal)
    { return _request_instantiator<T>::lookup(transaction, returnFields, searchFields, searchValues, compareOp, additionalClauses); }

    // value ptrs must have all values set properly, and no valid id
    // ptrs will be added to internal maps once the id fields have been generated,
    // and it will be updated in place
    // the objects will have their ids even if the transaction is aborted, the object's ids still exist, however
    //   they are still valid to be passed into this function
    // all foreign keys must be added first (TODO: change this?)
    template<typename T>
    auto insert(
        pqxx::transaction_base& transaction,
        std::vector<std::shared_ptr<T>>& value)
    { return _request_instantiator<T>::insert(transaction, value); }

    // temporary overload
    template<typename T>
    auto insert(
        pqxx::transaction_base& transaction,
        std::vector<std::shared_ptr<T>>&& value)
    { return insert(transaction, value); }

    // values must be non-null and have a valid ID
    // this is a no-op for types where has_id_v<T> is false
    template<typename T>
    auto update(
        pqxx::transaction_base& transaction,
        T const* values,
        typename T::request updateFields/* = all_fields<typename T::request>()*/)
    { return _request_instantiator<T>::update(transaction, values, updateFields); }
    
    template struct _request_instantiator<NewsItem>;
    template struct _request_instantiator<User>; // so that i don't have to repeat the signature 5 billion times
    template struct _request_instantiator<Tag>;
    template struct _request_instantiator<Group>;
    template struct _request_instantiator<GameVersion>;
    template struct _request_instantiator<Download>;
    template struct _request_instantiator<Mod>;
    template struct _request_instantiator<GameVersion_VisibleGroups_JoinItem>;
    template struct _request_instantiator<Mods_Tags_JoinItem>;
    template struct _request_instantiator<Users_Groups_JoinItem>;
    template struct _request_instantiator<state::LogItem>;
    template struct _request_instantiator<state::Token>;

    template<typename T>
    struct _id_instantiator {
        using IdType = id_type_t<T>;
        static IdType& id(T&);
        static IdType id(T const&);
        
        static std::shared_ptr<T> from_id(IdType const&);
        static std::shared_ptr<T> insert(std::shared_ptr<T> const&);
        static void insert_modify(std::shared_ptr<T>& ptr)
        { ptr = insert(ptr); }

    private:
        static std::shared_mutex mutex;
        static std::map<IdType, std::weak_ptr<T>> ptr_map;
    };

    template<typename T>
    auto& id(T& arg) { return _id_instantiator<T>::id(arg); }
    template<typename T>
    auto id(T const& arg) { return _id_instantiator<T>::id(arg); }
    template<typename T>
    auto foreign_key_id(foreign_key<T, id_type_t<T>> const& arg) { 
        if (is_resolved(arg)) return id(*get_resolved(arg));
        else                  return get_unresolved(arg);
    }

    template<typename T>
    auto shared_from_id(id_type_t<T> const& id)
    { return _id_instantiator<T>::from_id(id); }
    template<typename T>
    auto store_shared(std::shared_ptr<T> const& sh)
    { return _id_instantiator<T>::insert(sh); }
    template<typename T>
    void update_shared(std::shared_ptr<T>& sh)
    { _id_instantiator<T>::insert_modify(sh); }

    template struct _id_instantiator<User>;
    template struct _id_instantiator<NewsItem>;
    template struct _id_instantiator<Tag>;
    template struct _id_instantiator<Group>;
    template struct _id_instantiator<GameVersion>;
    template struct _id_instantiator<Mod>;
    template struct _id_instantiator<state::LogItem>;

    template<typename T>
    struct has_id : std::false_type {};
    
    template<> struct has_id<User> : std::true_type {};
    template<> struct has_id<NewsItem> : std::true_type {};
    template<> struct has_id<Tag> : std::true_type {};
    template<> struct has_id<Group> : std::true_type {};
    template<> struct has_id<GameVersion> : std::true_type {};
    template<> struct has_id<Mod> : std::true_type {};
    template<> struct has_id<state::LogItem> : std::true_type {};

    template<typename T>
    constexpr bool has_id_v = has_id<T>::value;
}

namespace std { 
    std::string to_string(BeatMods::db::CompareOp);
    std::string to_string(BeatMods::db::DownloadType);
    std::string to_string(BeatMods::db::Permission);
    std::string to_string(BeatMods::db::System);
    std::string to_string(BeatMods::db::Approval);
    std::string to_string(BeatMods::db::Visibility);
    std::string to_string(BeatMods::db::state::LogLevel);
}

#endif