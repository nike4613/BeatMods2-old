#include "db/engine.h"
#include <map>
#include <iomanip>

#define ENUM_STRING_SWITCH_CASE(pre, name) case pre::name: return #name;
#define ESSC(p,n) ENUM_STRING_SWITCH_CASE(p, n) // just a short form of the above

using namespace BeatMods::db;

std::string BeatMods::db::to_pg_op(PgCompareOp op) 
{
    if (op == PgCompareOp::Less) return "<";
    if (op == PgCompareOp::Greater) return ">";
    if (op == PgCompareOp::Equal) return "=";
    if (op == PgCompareOp::LessEqual) return "<=";
    if (op == PgCompareOp::GreaterEqual) return ">=";
    if (op == PgCompareOp::NotEqual) return "<>";

    if (op == PgCompareOp::Like) return "LIKE";
    if (op == PgCompareOp::ILike) return "ILIKE";
    if (op == PgCompareOp::NotLike) return "NOT LIKE";
    if (op == PgCompareOp::NotILike) return "NOT ILIKE";
    if (op == PgCompareOp::Similar) return "SIMILAR TO";
    if (op == PgCompareOp::NotSimilar) return "NOT SIMILAR TO";
    if (op == PgCompareOp::PosixRE) return "~*";
    if (op == PgCompareOp::NotPosixRE) return "!~*";

    std::string part; // all the rest follow a nice pattern
    if (static_cast<int>(op) & static_cast<int>(PgCompareOp::Distinct))
        part = "DISTINCT FROM";
    else if (static_cast<int>(op) & static_cast<int>(PgCompareOp::Null))
        part = "NULL";
    else if (static_cast<int>(op) & static_cast<int>(PgCompareOp::True))
        part = "TRUE";
    else if (static_cast<int>(op) & static_cast<int>(PgCompareOp::False))
        part = "FALSE";
    else if (static_cast<int>(op) & static_cast<int>(PgCompareOp::Unknown))
        part = "UNKNOWN";
    assert(part.size() != 0);
    if (static_cast<int>(op) & static_cast<int>(PgCompareOp::Not))
        part = "NOT " + part;
    return "IS " + part;
}

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
        T const* values,
        PgCompareOp compareOp,
        bool joinPart,
        std::string_view additionalClauses,
        int& paramFieldNum);
    template<typename T>
    SerializeResponse serialize_for_insert(
        pqxx::connection_base& conn, 
        T const* values);
    template<typename T>
    std::shared_ptr<T> deserialize_from_request(
        pqxx::row const& row, 
        typename T::request const& responseFields,
        bool isFromJoin,
        int& colStartOffset);

}

template<typename T>
std::vector<std::shared_ptr<T>> _make_request_instantiable<T>::lookup(
    pqxx::transaction_base& transaction,
    typename T::request const& returnFields, 
    typename T::request const& searchFields,
    T const* searchValues,
    PgCompareOp compareOp,
    std::string_view additionalClauses)
{
    int pf = 1;
    auto serialized = serialize_for_lookup(transaction.conn(), returnFields, searchFields, searchValues, compareOp, false, additionalClauses, pf);

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
    {
        int colId = 0;
        deserializedResult[i] = deserialize_from_request<T>(queryResult[i], returnFields, false, colId);
    }

    return deserializedResult;
}

namespace {

    template<typename TR,
        typename = std::enable_if_t<std::is_same_v<typename TR::_type::request, TR>>>
    std::string to_string(TR const& v)
    {
        constexpr size_t trSize = sizeof(TR);
        auto vd = reinterpret_cast<uint8_t const*>(&v);

        std::ostringstream str {};
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
        User const* values,
        PgCompareOp compareOp,
        bool joinPart,
        std::string_view additionalClauses,
        int& paramFieldNum)
    { // currently only works if used on exactly one connection
        auto whereCount = 0;
        if (fields.id)          ++whereCount;
        if (fields.name)        ++whereCount;
        if (fields.created)     ++whereCount;
        if (fields.githubId)    ++whereCount;
        if (whereCount > 0)     assert(values != nullptr);

        auto generateWhereClause = [&](std::ostringstream& stream, int& fieldNum) {
            auto op = to_pg_op(compareOp);
            stream << "(";
            bool fieldBefore = false;
            if (fields.id) {
                fieldBefore = true;
                stream << User::table << ".\"id\" " << op << " $" << fieldNum++ << " ";
            }
            if (fields.name) {
                if (fieldBefore) stream << "AND ";
                fieldBefore = true;
                stream << User::table << ".\"name\" " << op << " $" << fieldNum++ << " ";
            }
            if (fields.created) {
                if (fieldBefore) stream << "AND ";
                fieldBefore = true;
                stream << User::table << ".\"created\" " << op << " $" << fieldNum++ << " ";
            }
            if (fields.githubId) {
                if (fieldBefore) stream << "AND ";
                fieldBefore = true;
                stream << User::table << ".\"githubId\" " << op << " $" << fieldNum++ << " ";
            }

            stream << ")";
        };

        auto getWhereValues = [&]() -> std::vector<std::string> {
            std::vector<std::string> vector; // initialize to the max number of known fields
            vector.reserve(whereCount); // initialize to the number of fields

            if (fields.id)          vector.push_back(pqxx::to_string(values->id));
            if (fields.name)        vector.push_back(pqxx::to_string(values->name));
            if (fields.created)     vector.push_back(pqxx::to_string(values->created));
            if (fields.githubId)    vector.push_back(pqxx::to_string(values->githubId));

            return vector;
        };

        auto generateSql = [&](std::string_view moreClauses = "") {
                std::ostringstream sql {};
                sql << "SELECT ";

                bool fieldBefore = false;
                //if (responseFields.id) {
                    fieldBefore = true; // always request ID
                    sql << "\"id\"";
                //}
                if (responseFields.name) {
                    if (fieldBefore) sql << ",";
                    fieldBefore = true;
                    sql << "\"name\"";
                }
                if (responseFields.created) {
                    if (fieldBefore) sql << ",";
                    fieldBefore = true;
                    sql << "\"created\"";
                }
                if (responseFields.githubId) {
                    if (fieldBefore) sql << ",";
                    fieldBefore = true;
                    sql << "\"githubId\"";
                }
                if (!fieldBefore) // no fields requested
                    throw std::runtime_error("No fields requested in lookup");
                sql << " FROM " << User::table;
                // TODO: implement WHERE clause from fields and values
                if (whereCount > 0) {
                    sql << " WHERE ";
                    generateWhereClause(sql, paramFieldNum);
                }

                sql << " " << moreClauses << ";";
                return sql.str();
        };

        if (!joinPart && additionalClauses.size() == 0)
        {
            std::string name; bool generate = false;
            auto prepNameIt = lookup_Users_prepared_stmts.find({responseFields, fields});
            if (prepNameIt == lookup_Users_prepared_stmts.end())
            { // prepare the statement
                name = "User-getBy-" + to_string(responseFields) + "-" + to_string(fields);
                conn.prepare(name, generateSql());
                lookup_Users_prepared_stmts.insert({{responseFields, fields}, name});
            } else name = prepNameIt->second;

            // TODO: replace {} with values for WHERE clause
            return {SerializeResponse::kPrepared, name, getWhereValues()};
        }
        else if (!joinPart && additionalClauses.size() != 0)
        {
            // TODO: replace {} with values for WHERE clause
            return {SerializeResponse::kNormal, generateSql(additionalClauses), getWhereValues()};
        }
        else if (joinPart) 
        {
            
            return {}; // TODO: implement
        }

        #ifdef __GNUC__
        __builtin_unreachable();
        #endif

        return {}; // should never reach
    }

    template<>
    std::shared_ptr<User> deserialize_from_request(
        pqxx::row const& row, 
        typename User::request const& responseFields,
        bool isFromJoin,
        int& cid)
    { // prefer deserializing by index
        auto value = std::make_shared<User>();

        if (responseFields.id)
            value->id = row[cid++].as(value->id);
        if (responseFields.name) 
            value->name = row[cid++].as(value->name);
        if (responseFields.created)
            value->created = row[cid++].as(value->created);
        if (responseFields.githubId)
            value->githubId = row[cid++].as(value->githubId);

        return value;
    }

}