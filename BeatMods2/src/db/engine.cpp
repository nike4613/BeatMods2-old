#include "db/engine.h"
#include <map>
#include <iomanip>
#include <functional>

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
    sassert(part.size() != 0);
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

    sassert(false);
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

    sassert(false);
    return {};
}

std::string std::to_string(System e)
{
    switch (e)
    {
        ESSC(System, PC);
        ESSC(System, Quest);
    }

    sassert(false);
    return {};
}

std::string std::to_string(Permission e)
{
    #define M(name) EXEC(ESSC(Permission, name))
    switch (e) { PERMISSIONS(M); }
    #undef M

    sassert(false);
    return {};
}

namespace {

    struct SerializeResponse {
        enum {
            kNormal, kPrepared, kJoinPart
        } statementType;
        std::string nameOrQuery; // becomes join target table if a join part
        std::vector<std::string> queryParams;
        std::function<void(std::basic_ostream<char>&)> selectCols;
        std::function<void(std::basic_ostream<char>&, int&)> wherePart;
    };

    template<typename T>
    SerializeResponse serialize_for_lookup(
        pqxx::connection_base& conn, 
        typename T::request const& responseFields, 
        typename T::request const& fields, 
        T const* values,
        PgCompareOp compareOp,
        bool joinPart,
        std::string_view additionalClauses);
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
    auto serialized = serialize_for_lookup(transaction.conn(), returnFields, searchFields, searchValues, compareOp, false, additionalClauses);

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

template<typename T>
std::string _make_request_instantiable<T>::get_id_string(T const& arg)
{ return arg.id; } // TODO: specialize for the types that need it

namespace {

    template<typename TR,
        typename = std::enable_if_t<std::is_same_v<typename TR::_type::request, TR>>>
    __attribute__ ((pure)) std::string to_string(TR const& v)
    {
        constexpr size_t trSize = sizeof(TR);
        auto vd = reinterpret_cast<uint8_t const*>(&v);

        std::ostringstream str {};
        str << std::setfill('0') << std::hex;
        for (size_t i = 0; i < trSize; ++i)
            str << std::setw(2) << static_cast<uint16_t>(vd[i]);
        return str.str();
    }

    template<typename TReq, 
        typename = std::enable_if_t<std::is_same_v<typename TReq::_type::request, TReq>>>
    struct comparator { // impl less than
        __attribute__ ((pure)) bool operator()(std::pair<TReq, TReq> const& a,
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

    #define COUNT_WHERE_CLAUSE(name, fields, whereCount) if (fields.name) ++whereCount;
    #define WHERE_CLAUSE_FIELD(name, type, op, fields, stream, fieldNum) \
        if (fields.name) { \
            if (fieldBefore) stream << "AND "; \
            fieldBefore = true; \
            stream << type::table << ".\"" #name "\" " << op << " $" << fieldNum++ << " "; \
        }
    #define WHERE_VALUES_FIELD(name, values, fields, vector) \
        if (fields.name) vector.push_back(pqxx::to_string(values->name));
    #define SELECT_CLAUSE_FIELD(name, type, responseFields, stream) \
        if (responseFields.name) { \
            if (fieldBefore) stream << ","; \
            fieldBefore = true; \
            stream << type::table << ".\"" #name "\""; \
        }
    #define BASIC_DESERIALIZE_FIELD(name, responseFields, row, value, cid) \
        if (responseFields.name) \
            value->name = row[cid++].as(value->name);

    // only has non-id fields
    #define _Users_FIELDS(M, ...) \
        M(name, __VA_ARGS__) M(created, __VA_ARGS__) M(githubId, __VA_ARGS__)
    #define Users_FIELDS(M, ...) EXEC(_Users_FIELDS(M, __VA_ARGS__))

    // TODO: fix this to work across connections
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
        std::string_view additionalClauses)
    { // currently only works if used on exactly one connection
        auto whereCount = 0;
        if (fields.id) ++whereCount;
        Users_FIELDS(COUNT_WHERE_CLAUSE, fields, whereCount)
        if (whereCount > 0) sassert(values != nullptr);

        auto generateWhereClause = [=](std::basic_ostream<char>& stream, int& fieldNum) {
            auto op = to_pg_op(compareOp);
            stream << "(";
            bool fieldBefore = false;
            if (fields.id) {
                fieldBefore = true;
                stream << User::table << ".\"id\" " << op << " $" << fieldNum++ << " ";
            }
            Users_FIELDS(WHERE_CLAUSE_FIELD, User, op, fields, stream, fieldNum)

            stream << ")";
        };

        auto getWhereValues = [&]() -> std::vector<std::string> {
            std::vector<std::string> vector; // initialize to the max number of known fields
            vector.reserve(whereCount); // initialize to the number of fields

            if (fields.id) vector.push_back(pqxx::to_string(values->id));
            Users_FIELDS(WHERE_VALUES_FIELD, values, fields, vector)
            return vector;
        };

        auto generateSelectFields = [=](std::basic_ostream<char>& stream) {
            bool fieldBefore = false;
            //if (responseFields.id) {
                fieldBefore = true; // always request ID
                stream << User::table << ".\"id\"";
            //}
            Users_FIELDS(SELECT_CLAUSE_FIELD, User, responseFields, stream)
            if (!fieldBefore) // no fields requested
                throw std::runtime_error("No fields requested in lookup");
        };

        auto generateSql = [&](std::string_view moreClauses = "") {
            std::ostringstream sql {};
            sql << "SELECT ";
            generateSelectFields(sql);
            sql << " FROM " << User::table;
            
            if (whereCount > 0) {
                sql << " WHERE ";
                int i = 1;
                generateWhereClause(sql, i);
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

            return {SerializeResponse::kPrepared, name, getWhereValues()};
        }
        else if (!joinPart && additionalClauses.size() != 0)
            return {SerializeResponse::kNormal, generateSql(additionalClauses), getWhereValues()};
        else if (joinPart) 
            return {SerializeResponse::kJoinPart, User::table, getWhereValues(), generateSelectFields, generateWhereClause};

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
        // TODO: implement ID based pointer caching

        //if (responseFields.id)
            value->id = row[cid++].as(value->id); // id is always requested
        Users_FIELDS(BASIC_DESERIALIZE_FIELD, responseFields, row, value, cid)

        return value;
    }

    #define JOIN_GET_RESPONSE(name, remoteId, type, responseFields, fields, values, conn, compareOp, whereCount) \
        SerializeResponse name ## _response; \
        if (responseFields.name ## _resolve || fields.name ## _resolve) { \
            type const* uvalues = nullptr; \
            if (values) { \
                auto suv = std::get_if<std::shared_ptr<type>>(&values->name); \
                if (suv) uvalues = suv->get(); \
            } \
            if (fields.name ## _resolve) ++whereCount;\
            name ## _response = serialize_for_lookup<type>( \
                conn, \
                responseFields.name ## _resolve ? responseFields.name ## _request : type::request{}, \
                fields.name ## _resolve ? fields.name ## _request : type::request{}, \
                uvalues, \
                compareOp, \
                true, ""); \
        }
    #define JOIN_WHERE_PART(name, remoteId, fields, stream, fieldNum) \
        if (fields.name ## _resolve) { \
            if (fieldBefore) stream << "AND "; \
            fieldBefore = true; \
            name ## _response.wherePart(stream, fieldNum); \
        }
    #define JOIN_WHERE_VALUES(name, remoteId, vector, fields) \
        if (fields.name ## _resolve) \
            vector.insert(std::end(vector), std::begin(name ## _response.queryParams), std::end(name ## _response.queryParams)); 
    #define JOIN_SELECT_PART(name, remoteId, responseFields, stream) \
        if (responseFields.name ## _resolve) { \
            if (fieldBefore) stream << ","; \
            fieldBefore = true; \
            name ## _response.selectCols(stream); \
        }
    #define JOIN_FROM_TABLE(name, remoteId, stream, responseFields) \
        if (responseFields.name ## _resolve) \
            stream << " LEFT JOIN " << decltype(responseFields.name ## _request)::_type::table \
                << " ON " << std::remove_reference_t<decltype(responseFields)>::_type::table << \
                ".\"" #name "\" = " << decltype(responseFields.name ## _request)::_type::table << \
                ".\"" #remoteId "\"";
    #define JOIN_DESERIALIZE_FIELD(name, remoteId, responseFields, value, row, cid) \
        if (responseFields.name ## _resolve) \
            value->name = \
                deserialize_from_request<decltype(responseFields.name ## _request)::_type> \
                    (row, responseFields.name ## _request, true, cid);

    // only has non-id fields
    #define _NewsItem_FIELDS(M, ...) \
        M(title, __VA_ARGS__) M(author, __VA_ARGS__) M(body, __VA_ARGS__) \
        M(posted, __VA_ARGS__) M(edited, __VA_ARGS__) M(system, __VA_ARGS__) 
    #define NewsItem_FIELDS(M, ...) EXEC(_NewsItem_FIELDS(M, __VA_ARGS__))
    #define _NewsItem_FOREIGN_FIELDS(M, ...) \
        M(author, id, __VA_ARGS__)
    #define NewsItem_FOREIGN_FIELDS(M, ...) EXEC(_NewsItem_FOREIGN_FIELDS(M, __VA_ARGS__))


    // TODO: fix this to work across connections
    // cache to keep track of prepped statements
    prepared_name_cache<NewsItem> lookup_NewsItems_prepared_stmts {};
    template<>
    SerializeResponse serialize_for_lookup(
        pqxx::connection_base& conn,
        NewsItem::request const& responseFields,
        NewsItem::request const& fields,
        NewsItem const* values,
        PgCompareOp compareOp,
        bool joinPart,
        std::string_view additionalClauses)
    { // currently only works if used on exactly one connection
        auto whereCount = 0;
        if (fields.id)      ++whereCount;
        NewsItem_FIELDS(COUNT_WHERE_CLAUSE, fields, whereCount)
        if (whereCount > 0) sassert(values != nullptr);

        NewsItem_FOREIGN_FIELDS(JOIN_GET_RESPONSE, User, responseFields, fields, values, conn, compareOp, whereCount)

        auto generateWhereClause = [=](std::basic_ostream<char>& stream, int& fieldNum) {
            auto op = to_pg_op(compareOp);
            stream << "(";
            bool fieldBefore = false;
            if (fields.id) {
                fieldBefore = true;
                stream << NewsItem::table << ".\"id\" " << op << " $" << fieldNum++ << " ";
            }
            NewsItem_FIELDS(WHERE_CLAUSE_FIELD, NewsItem, op, fields, stream, fieldNum)

            NewsItem_FOREIGN_FIELDS(JOIN_WHERE_PART, fields, stream, fieldNum);

            stream << ")";
        };

        auto getWhereValues = [&]() -> std::vector<std::string> {
            std::vector<std::string> vector; // initialize to the max number of known fields
            vector.reserve(whereCount); // initialize to the number of fields

            if (fields.id) vector.push_back(pqxx::to_string(values->id));
            NewsItem_FIELDS(WHERE_VALUES_FIELD, values, fields, vector)
            
            NewsItem_FOREIGN_FIELDS(JOIN_WHERE_VALUES, vector, fields);

            return vector;
        };

        auto generateSelectFields = [=](std::basic_ostream<char>& stream) {
            bool fieldBefore = false;
            //if (responseFields.id) {
                fieldBefore = true; // always request ID
                stream << NewsItem::table << ".\"id\"";
            //}
            NewsItem_FIELDS(SELECT_CLAUSE_FIELD, NewsItem, responseFields, stream)
            NewsItem_FOREIGN_FIELDS(JOIN_SELECT_PART, responseFields, stream);
            if (!fieldBefore) // no fields requested
                throw std::runtime_error("No fields requested in lookup");
        };

        auto generateSql = [&](std::string_view moreClauses = "") {
            std::ostringstream sql {};
            sql << "SELECT ";
            generateSelectFields(sql);
            sql << " FROM " << NewsItem::table;
            
            NewsItem_FOREIGN_FIELDS(JOIN_FROM_TABLE, sql, responseFields)
            
            if (whereCount > 0) {
                sql << " WHERE ";
                int i = 1;
                generateWhereClause(sql, i);
            }

            sql << " " << moreClauses << ";";
            return sql.str();
        };

        if (!joinPart && additionalClauses.size() == 0)
        {
            std::string name; bool generate = false;
            auto prepNameIt = lookup_NewsItems_prepared_stmts.find({responseFields, fields});
            if (prepNameIt == lookup_NewsItems_prepared_stmts.end())
            { // prepare the statement
                name = "NewsItem-getBy-" + to_string(responseFields) + "-" + to_string(fields);
                conn.prepare(name, generateSql());
                lookup_NewsItems_prepared_stmts.insert({{responseFields, fields}, name});
            } else name = prepNameIt->second;

            return {SerializeResponse::kPrepared, name, getWhereValues()};
        }
        else if (!joinPart && additionalClauses.size() != 0)
            return {SerializeResponse::kNormal, generateSql(additionalClauses), getWhereValues()};
        else if (joinPart) 
            return {SerializeResponse::kJoinPart, NewsItem::table, getWhereValues(), generateSelectFields, generateWhereClause};

        #ifdef __GNUC__
        __builtin_unreachable();
        #endif

        return {}; // should never reach
    }

    template<>
    std::shared_ptr<NewsItem> deserialize_from_request(
        pqxx::row const& row, 
        typename NewsItem::request const& responseFields,
        bool isFromJoin,
        int& cid)
    { // prefer deserializing by index
        auto value = std::make_shared<NewsItem>();
        // TODO: implement ID based pointer caching

        //if (responseFields.id)
            value->id = row[cid++].as(value->id); // id is always requested
        NewsItem_FIELDS(BASIC_DESERIALIZE_FIELD, responseFields, row, value, cid)
        NewsItem_FOREIGN_FIELDS(JOIN_DESERIALIZE_FIELD, responseFields, value, row, cid)

        return value;
    }

}