#ifndef BEATMODS2_DB_ENGINE_H
#define BEATMODS2_DB_ENGINE_H

#include <pqxx/pqxx>
#include <cstddef>
#include <semver200.h>
#include <variant>
#include <chrono>
#include <vector>
#include <rapidjson/document.h>
#include <optional>
#include <future>

#ifndef EXEC
#define EXEC(...) __VA_ARGS__
#endif

namespace BeatMods::db {

    constexpr int mod_id_length = 128;

    using UUID = std::string; // just a strig containing the uuid
    using Integer = int32_t;
    using BigInteger = int64_t;

    using Version = version::Semver200_version;
    using TimeStamp = std::chrono::system_clock::time_point;
    using JSON = rapidjson::Document;

    template<typename TResolved, typename TUnresolved>
    using foreign_key = std::variant<std::shared_ptr<TResolved>, TUnresolved>;

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

    struct User {
        UUID id; // PKey (when creating, this is never used; instead DB default is)
        std::string name; // Unique
        TimeStamp created;
        std::string githubId;

        static constexpr char const* table = "\"mod-repo\".\"Users\"";
        struct request {
            using _type = User;
            bool id:1; bool name:1; bool created:1; bool githubId:1;
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
        char id[mod_id_length];
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

    template<typename T>
    struct _make_request_instantiable {
        [[nodiscard]] static std::vector<std::shared_ptr<T>> lookup(
            pqxx::transaction_base& transaction,
            typename T::request const& returnFields, 
            typename T::request const& searchFields = {}, 
            T const& searchValues = {});
    };

    template<typename T>
    [[nodiscard]] auto lookup(
        pqxx::transaction_base& transaction,
        typename T::request const& returnFields, 
        typename T::request const& searchFields = {}, 
        T const& searchValues = {})
    { return _make_request_instantiable<T>::lookup(transaction, returnFields, searchFields, searchValues); }

    template struct _make_request_instantiable<User>; // so that i don't have to repeat the signature 5 billion times
    /*template struct _make_request_instantiable<GameVersion>;
    template struct _make_request_instantiable<Mod>;
    template struct _make_request_instantiable<Download>;
    template struct _make_request_instantiable<Group>;
    template struct _make_request_instantiable<Tag>;
    template struct _make_request_instantiable<NewsItem>;
    template struct _make_request_instantiable<GameVersion_VisibleGroups_JoinItem>;
    template struct _make_request_instantiable<Mods_Tags_JoinItem>;
    template struct _make_request_instantiable<Users_Groups_JoinItem>;*/ // TODO: re-enable all of these
}

namespace std { 
    std::string to_string(BeatMods::db::CompareOp);
    std::string to_string(BeatMods::db::DownloadType);
    std::string to_string(BeatMods::db::Permission);
}


#endif