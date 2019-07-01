#ifndef BEATMODS2_DB_ENGINE_H
#define BEATMODS2_DB_ENGINE_H

#include <pqxx/pqxx>
#include <cstddef>
#include <semver200.h>
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
#include "util/json.h"

#ifndef EXEC
#define EXEC(...) __VA_ARGS__
#endif

#ifndef STR
#define _STR(s) #s
#define STR(s) _STR(s)
#endif

#define sassert(...) { if (!(__VA_ARGS__)) throw std::runtime_error("Assertion failed: " STR((__VA_ARGS__))); }

namespace BeatMods::db {

    constexpr int mod_id_length = 128;

    using UUID = std::string; // just a string containing the uuid
    using Integer = int32_t;
    using BigInteger = int64_t;

    using Version = version::Semver200_version;
    using TimeStamp = std::chrono::system_clock::time_point;
    using JSON = rapidjson::Document;

    template<typename TResolved, typename TUnresolved>
    using foreign_key = std::variant<std::shared_ptr<TResolved>, TUnresolved>;

    template<typename TR, typename TU>
    auto get_resolved(foreign_key<TR, TU>& ref) { return std::get<std::shared_ptr<TR>>(ref); }
    template<typename TR, typename TU>
    auto get_resolved(foreign_key<TR, TU> const& ref) { return std::get<std::shared_ptr<TR>>(ref); }
    template<typename TR, typename TU>
    auto get_unresolved(foreign_key<TR, TU>& ref) { return std::get<TU>(ref); }
    template<typename TR, typename TU>
    auto get_unresolved(foreign_key<TR, TU> const& ref) { return std::get<TU>(ref); }

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

    // TODO: add overloads of pqxx::string_traits<>::to_string and pqxx::string_traits<>::from_string for the above types

    // Tables //

    struct User {
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
    	std::optional<TimeStamp> approved;
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
        BigInteger id; // PKey (when creating, this is never used; instead DB default is)
        std::string name;

        static constexpr char const* table = "\"mod-repo\".\"Tags\"";
        struct request {
            using _type = Tag;
            bool id:1; bool name:1;
        };
    };

    struct NewsItem {
        UUID id; // PKey (when creating, this is never used; instead DB default is)
        std::string title;
        foreign_key<User, UUID> author; // FKey Users.id
        std::string body;
        TimeStamp posted;
        std::optional<TimeStamp> edited;
        std::optional<System> system;

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
        
        static constexpr char const* table = "\"mod-repo\".\" Users_Groups_Joiner\"";
        struct request {
            using _type = Users_Groups_JoinItem;
            bool user:1; bool user_resolve:1;
            bool group:1; bool group_resolve:1;

            User::request user_request;
            Group::request group_request;
        };
    };

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

    template<typename T>
    struct _make_request_instantiable {
        static std::vector<std::shared_ptr<T>> lookup(
            pqxx::transaction_base& transaction,
            typename T::request const& returnFields, 
            typename T::request const& searchFields = {}, 
            T const* searchValues = nullptr,
            PgCompareOp compareOp = PgCompareOp::Equal,
            std::string_view additionalClauses = "");
    };

    template<typename T>
    auto lookup(
        pqxx::transaction_base& transaction,
        typename T::request const& returnFields,
        std::string_view additionalClauses = "", 
        typename T::request const& searchFields = {}, 
        T const* searchValues = nullptr,
        PgCompareOp compareOp = PgCompareOp::Equal)
    { return _make_request_instantiable<T>::lookup(transaction, returnFields, searchFields, searchValues, compareOp, additionalClauses); }

    template<typename T>
    struct _id_instantiator {
        using IdType = id_type_t<T>;
        static IdType& id(T&);
        static IdType const& id(T const&);
    };

    template<typename T>
    auto& id(T& arg) { return _id_instantiator<T>::id(arg); }
    template<typename T>
    auto const& id(T const& arg) { return _id_instantiator<T>::id(arg); }

    template struct _make_request_instantiable<User>; // so that i don't have to repeat the signature 5 billion times
    template struct _make_request_instantiable<NewsItem>;
    template struct _make_request_instantiable<Tag>;
    template struct _make_request_instantiable<Group>;
    template struct _make_request_instantiable<GameVersion>;
    template struct _make_request_instantiable<Download>;
    template struct _make_request_instantiable<Mod>; 

    /*
    template struct _make_request_instantiable<GameVersion_VisibleGroups_JoinItem>;
    template struct _make_request_instantiable<Mods_Tags_JoinItem>;
    template struct _make_request_instantiable<Users_Groups_JoinItem>;
    */ // TODO: re-enable all of these
}

namespace std { 
    std::string to_string(BeatMods::db::CompareOp);
    std::string to_string(BeatMods::db::DownloadType);
    std::string to_string(BeatMods::db::Permission);
    std::string to_string(BeatMods::db::System);
    std::string to_string(BeatMods::db::Approval);
    std::string to_string(BeatMods::db::Visibility);
}

#endif