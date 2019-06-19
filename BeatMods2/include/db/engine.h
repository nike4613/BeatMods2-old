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

    struct User {
        UUID id; // PKey (when creating, this is never used; instead DB default is)
        std::string name; // Unique
        TimeStamp created;
        std::string githubId;
    };

    struct GameVersion {
        UUID id; // PKey (when creating, this is never used; instead DB default is)
        Version version;
        BigInteger steamBuildId;
        Visibility visibility;
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
    };

    struct Download {
        foreign_key<Mod, UUID> mod; // PKey, FKey Mods.uuid
        DownloadType type;          // PKey
        std::string cdnFile;
        JSON hashes;
    };

    struct Groups {
        BigInteger id; // PKey (when creating, this is never used; instead DB default is)
        std::string name;
        std::vector<Permission> permissions;
    };

    struct Tag {
        BigInteger id; // PKey (when creating, this is never used; instead DB default is)
        std::string name;
    };

    struct NewsItem {
        UUID id; // PKey (when creating, this is never used; instead DB default is)
        std::string title;
        foreign_key<User, UUID> author; // FKey Users.id
        std::string body;
        TimeStamp posted;
        std::optional<TimeStamp> edited;
        std::optional<System> system;
    };

    // TODO: add joiners

}

namespace std { 
    std::string to_string(BeatMods::db::CompareOp);
    std::string to_string(BeatMods::db::DownloadType);
    std::string to_string(BeatMods::db::Permission);
}


#endif