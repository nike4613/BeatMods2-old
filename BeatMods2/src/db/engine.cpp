#include "db/engine.h"
#include <map>
#include <iomanip>
#include <functional>
#include <memory>

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
{ return arg.id; }
template<>
std::string _make_request_instantiable<Mod>::get_id_string(Mod const& arg)
{ return arg.uuid; }

namespace {

    template<typename T, typename IType = decltype(T{}.id)>
    std::shared_ptr<T> make_cached_shared(IType const& id)
    {
        static std::map<IType, std::weak_ptr<T>> ptrCache;

        auto loc = ptrCache.find(id);
        if (loc != ptrCache.end()) {
            if (!loc->second.expired())
                return loc->second.lock();
            else ptrCache.erase(loc);
        }

        auto ptr = std::make_shared<T>();
        ptr->id = id;
        ptrCache.insert({id, ptr});
        return ptr;
    }
    
    template<>
    std::shared_ptr<Mod> make_cached_shared<Mod, UUID>(UUID const& id)
    {
        static std::map<UUID, std::weak_ptr<Mod>> ptrCache;

        auto loc = ptrCache.find(id);
        if (loc != ptrCache.end()) {
            if (!loc->second.expired())
                return loc->second.lock();
            else ptrCache.erase(loc);
        }

        auto ptr = std::make_shared<Mod>();
        ptr->uuid = id;
        ptrCache.insert({id, ptr});
        return ptr;
    }

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

    #define SPECIALIZE_SERIALIZERS_FOR(_TYPE, idField) \
        template<> \
        SerializeResponse serialize_for_lookup( \
            pqxx::connection_base& conn, \
            _TYPE::request const& responseFields, \
            _TYPE::request const& fields, \
            _TYPE const* values, \
            PgCompareOp compareOp, \
            bool joinPart, \
            std::string_view additionalClauses) \
        { \
            auto whereCount = 0; \
            if (fields.idField) ++whereCount; \
            EXEC(_TYPE##_FIELDS (COUNT_WHERE_CLAUSE, fields, whereCount)) \
            if (whereCount > 0) sassert(values != nullptr); \
            \
            EXEC(_TYPE##_FOREIGN_FIELDS (JOIN_GET_RESPONSE, User, responseFields, fields, values, conn, compareOp, whereCount)) \
            \
            auto generateWhereClause = [=](std::basic_ostream<char>& stream, int& fieldNum) { \
                auto op = to_pg_op(compareOp); \
                stream << "("; \
                bool fieldBefore = false; \
                if (fields.idField) { \
                    fieldBefore = true; \
                    stream << _TYPE::table << ".\"" #idField "\" " << op << " $" << fieldNum++ << " "; \
                } \
                EXEC(_TYPE##_FIELDS (WHERE_CLAUSE_FIELD, _TYPE, op, fields, stream, fieldNum)) \
                EXEC(_TYPE##_FOREIGN_FIELDS (JOIN_WHERE_PART, fields, stream, fieldNum)); \
                stream << ")"; \
            }; \
            \
            auto getWhereValues = [&]() -> std::vector<std::string> { \
                std::vector<std::string> vector; /* initialize to the max number of known fields */ \
                vector.reserve(whereCount); /* initialize to the number of fields */ \
                \
                if (fields.idField) vector.push_back(pqxx::to_string(values->idField)); \
                EXEC(_TYPE##_FIELDS (WHERE_VALUES_FIELD, values, fields, vector)) \
                EXEC(_TYPE##_FOREIGN_FIELDS (JOIN_WHERE_VALUES, vector, fields)); \
                \
                return vector; \
            }; \
            \
            auto generateSelectFields = [=](std::basic_ostream<char>& stream) { \
                bool fieldBefore = true; \
                stream << _TYPE::table << ".\"" #idField "\""; \
                EXEC(_TYPE##_FIELDS (SELECT_CLAUSE_FIELD, _TYPE, responseFields, stream)) \
                EXEC(_TYPE##_FOREIGN_FIELDS (JOIN_SELECT_PART, responseFields, stream)); \
                if (!fieldBefore) /* no fields requested */ \
                    throw std::runtime_error("No fields requested in lookup"); \
            }; \
            \
            auto generateSql = [&](std::string_view moreClauses = "") { \
                std::ostringstream sql {}; \
                sql << "SELECT "; \
                generateSelectFields(sql); \
                sql << " FROM " << _TYPE::table; \
                \
                EXEC(_TYPE##_FOREIGN_FIELDS (JOIN_FROM_TABLE, sql, responseFields)) \
                \
                if (whereCount > 0) { \
                    sql << " WHERE "; \
                    int i = 1; \
                    generateWhereClause(sql, i); \
                } \
                \
                sql << " " << moreClauses << ";"; \
                return sql.str(); \
            }; \
            \
            if (!joinPart) \
                return {SerializeResponse::kNormal, generateSql(additionalClauses), getWhereValues()}; \
            else \
                return {SerializeResponse::kJoinPart, _TYPE::table, getWhereValues(), generateSelectFields, generateWhereClause}; \
        } \
        \
        template<> \
        std::shared_ptr<_TYPE> deserialize_from_request( \
            pqxx::row const& row, \
            typename _TYPE::request const& responseFields, \
            bool isFromJoin, \
            int& cid) \
        { /* prefer deserializing by index */ \
            auto value = make_cached_shared<_TYPE>(row[cid++].as<decltype(_TYPE{}.idField)>()); \
            EXEC(_TYPE##_FIELDS (BASIC_DESERIALIZE_FIELD, responseFields, row, value, cid)) \
            EXEC(_TYPE##_FOREIGN_FIELDS (JOIN_DESERIALIZE_FIELD, responseFields, value, row, cid)) \
            return value; \
        }
    
    #define _User_FIELDS(M, ...) \
        M(name, __VA_ARGS__) M(created, __VA_ARGS__) M(githubId, __VA_ARGS__)
    #define User_FIELDS(M, ...) EXEC(_User_FIELDS(M, __VA_ARGS__))
    #define User_FOREIGN_FIELDS(M, ...)

    #define _NewsItem_FIELDS(M, ...) \
        M(title, __VA_ARGS__) M(author, __VA_ARGS__) M(body, __VA_ARGS__) \
        M(posted, __VA_ARGS__) M(edited, __VA_ARGS__) M(system, __VA_ARGS__) 
    #define NewsItem_FIELDS(M, ...) EXEC(_NewsItem_FIELDS(M, __VA_ARGS__))
    #define _NewsItem_FOREIGN_FIELDS(M, ...) \
        M(author, id, __VA_ARGS__)
    #define NewsItem_FOREIGN_FIELDS(M, ...) EXEC(_NewsItem_FOREIGN_FIELDS(M, __VA_ARGS__))

    SPECIALIZE_SERIALIZERS_FOR(User, id)
    SPECIALIZE_SERIALIZERS_FOR(NewsItem, id)

}