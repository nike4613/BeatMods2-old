#include "db/engine.h"
#include <map>
#include <iomanip>

#define ENUM_STRING_SWITCH_CASE(pre, name) case pre::name: return #name;
#define ESSC(p,n) ENUM_STRING_SWITCH_CASE(p, n) // just a short form of the above

using namespace BeatMods::db;

std::string std::to_string(CompareOp e)
{
    switch (e)
    {
        ESSC(CompareOp, Greater);
        ESSC(CompareOp, GreaterEqual);
        ESSC(CompareOp, Equal);
        ESSC(CompareOp, LessEqual);
        ESSC(CompareOp, Less);
    }

    assert(false);
    return {};
}

std::string std::to_string(DownloadType e)
{
    switch (e)
    {
        ESSC(DownloadType, Steam);
        ESSC(DownloadType, Oculus);
        ESSC(DownloadType, Universal);
    }

    assert(false);
    return {};
}

std::string std::to_string(Permission e)
{
    #define M(name) EXEC(ESSC(Permission, name))
    switch (e) { PERMISSIONS(M); }
    #undef M

    assert(false);
    return {};
}

namespace {

    struct SerializeResponse {
        enum {
            kNormal, kPrepared, kJoinPart
        } statementType;
        std::string nameOrQuery;
        std::vector<std::string> queryParams;
    };

    template<typename T>
    SerializeResponse serialize_for_lookup(
        pqxx::connection_base& conn, 
        typename T::request const& responseFields, 
        typename T::request const& fields, 
        T const& values,
        bool joinPart);
    template<typename T>
    SerializeResponse serialize_for_insert(
        pqxx::connection_base& conn, 
        T const& values);
    template<typename T>
    std::shared_ptr<T> deserialize_from_request(
        pqxx::row const& row, 
        typename T::request const& responseFields,
        bool isFromJoin,
        size_t colStartOffset = 0);

}

template<typename T>
std::vector<std::shared_ptr<T>> _make_request_instantiable<T>::lookup(
    pqxx::transaction_base& transaction,
    typename T::request const& returnFields, 
    typename T::request const& searchFields,
    T const& searchValues)
{
    auto serialized = serialize_for_lookup(transaction.conn(), returnFields, searchFields, searchValues, false);

    pqxx::result queryResult;
    switch (serialized.statementType)
    {
    case SerializeResponse::kNormal: 
        queryResult = transaction.exec_params(serialized.nameOrQuery, pqxx::prepare::make_dynamic_params(serialized.queryParams));
        break;
    case SerializeResponse::kPrepared: 
        queryResult = transaction.exec_prepared(serialized.nameOrQuery, pqxx::prepare::make_dynamic_params(serialized.queryParams));
        break;
    case SerializeResponse::kJoinPart:
        throw std::runtime_error("serialize_for_lookup returned a kJoinPart");
        break;
    }

    std::vector<std::shared_ptr<T>> deserializedResult {queryResult.size()};

    for (auto i = 0; i < queryResult.size(); ++i)
        deserializedResult[i] = deserialize_from_request<T>(queryResult[i], returnFields, false);

    return deserializedResult;
}

namespace {

    template<typename TR,
        typename = std::enable_if_t<std::is_same_v<typename TR::_type::request, TR>>>
    std::string to_string(TR const& v)
    {
        constexpr size_t trSize = sizeof(TR);
        auto vd = reinterpret_cast<uint8_t const*>(&v);

        std::stringstream str {};
        str << std::setfill('0') << std::hex << std::setw(2);
        for (size_t i = 0; i < trSize; ++i)
            str << static_cast<uint16_t>(vd[i]);
        return str.str();
    }

    template<typename TReq, 
        typename = std::enable_if_t<std::is_same_v<typename TReq::_type::request, TReq>>>
    struct comparator { // impl less than
        bool operator()(std::pair<TReq, TReq> const& a,
                        std::pair<TReq, TReq> const& b) const
        {
            constexpr size_t trSize = sizeof(TReq);

            auto a1 = reinterpret_cast<uint8_t const*>(&a.first);
            auto a2 = reinterpret_cast<uint8_t const*>(&a.second);
            auto b1 = reinterpret_cast<uint8_t const*>(&b.first);
            auto b2 = reinterpret_cast<uint8_t const*>(&b.second);

            for (size_t i = 0; i < trSize; ++i)
                if (a1[i] < b1[i]) return true;
            for (size_t i = 0; i < trSize; ++i)
                if (a2[i] < b2[i]) return true;
            return false;
        }
    };

    template<typename T>
    using prepared_name_cache = std::map<std::pair<typename T::request, typename T::request>, std::string, comparator<typename T::request>>;

    // cache to keep track of prepped statements
    prepared_name_cache<User> lookup_Users_prepared_stmts {};
    template<>
    SerializeResponse serialize_for_lookup(
        pqxx::connection_base& conn,
        User::request const& responseFields,
        User::request const& fields,
        User const& values,
        bool joinPart)
    { // currently only works if used on exactly one connection
        // TODO:: implement joinPart

        std::string name; bool generate = false;
        auto prepNameIt = lookup_Users_prepared_stmts.find({responseFields, fields});
        if (prepNameIt == lookup_Users_prepared_stmts.end())
        { // prepare the statement
            name = "User-getBy-" + to_string(responseFields) + "-" + to_string(fields);

            std::stringstream sql {};
            sql << "SELECT ";

            bool fieldBefore = false;
            if (responseFields.id) {
                fieldBefore = true;
                sql << "\"id\"";
            }
            if (responseFields.name) {
                if (fieldBefore) sql << ",";
                fieldBefore = true;
                sql << "\"name\"";
            }
            if (responseFields.created){
                if (fieldBefore) sql << ",";
                fieldBefore = true;
                sql << "\"created\"";
            }
            if (responseFields.githubId){
                if (fieldBefore) sql << ",";
                fieldBefore = true;
                sql << "\"githubId\"";
            }
            if (!fieldBefore) // no fields requested
                throw std::runtime_error("No fields requested in lookup");
            sql << " FROM " << User::table;
            // TODO: implement WITH clause from fields a values
            sql << ";";

            conn.prepare(name, sql.str());

            prepNameIt = lookup_Users_prepared_stmts.insert({{responseFields, fields}, name}).first;
        }

        // TODO: replace {} with values for WITH clause
        return {SerializeResponse::kPrepared, name, {}};
    }

    template<>
    std::shared_ptr<User> deserialize_from_request(
        pqxx::row const& row, 
        typename User::request const& responseFields,
        bool isFromJoin,
        size_t colStartOffset)
    { // prefer deserializing by index


        return nullptr;
    }

}