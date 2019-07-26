#include "db/engine.h"
#include "db/string_traits.h"
#include "util/algorithm.h"
#include "util/macro.h"
#include <map>
#include <iomanip>
#include <functional>
#include <memory>

#define ENUM_STRING_SWITCH_CASE(pre, name) case pre::name: return #name;
#define ESSC(p,n) ENUM_STRING_SWITCH_CASE(p, n) // just a short form of the above

using namespace BeatMods::db;

std::string BeatMods::db::to_pg_op(PgCompareOp op) {
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

std::string std::to_string(CompareOp e) {
    switch (e) {
        ESSC(CompareOp, Greater);
        ESSC(CompareOp, GreaterEqual);
        ESSC(CompareOp, Equal);
        ESSC(CompareOp, LessEqual);
        ESSC(CompareOp, Less);
    }

    sassert(false);
    return {};
}

std::string std::to_string(DownloadType e) {
    switch (e) {
        ESSC(DownloadType, Steam);
        ESSC(DownloadType, Oculus);
        ESSC(DownloadType, Universal);
    }

    sassert(false);
    return {};
}

std::string std::to_string(System e) {
    switch (e) {
        ESSC(System, PC);
        ESSC(System, Quest);
    }

    sassert(false);
    return {};
}

std::string std::to_string(Approval e) {
    switch (e) {
        ESSC(Approval, Approved);
        ESSC(Approval, Declined);
        ESSC(Approval, Pending);
        ESSC(Approval, Inactive);
    }

    sassert(false);
    return {};
}

std::string std::to_string(Visibility e) {
    switch (e) {
        ESSC(Visibility, Public);
        ESSC(Visibility, Groups);
    }

    sassert(false);
    return {};
}

std::string std::to_string(state::LogLevel e) {
    switch (e) {
        ESSC(state::LogLevel, Debug);
        ESSC(state::LogLevel, Warning);
        ESSC(state::LogLevel, Error);
        ESSC(state::LogLevel, Fatal);
        ESSC(state::LogLevel, Info);
        ESSC(state::LogLevel, Security);
    }

    sassert(false);
    return {};
}

std::string std::to_string(Permission e) {
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
        typename T::request responseFields, 
        typename T::request fields, 
        T const* values,
        PgCompareOp compareOp,
        bool joinPart,
        std::string_view additionalClauses);
    template<typename T>
    std::shared_ptr<T> deserialize_from_request(
        pqxx::row const& row, 
        typename T::request responseFields,
        bool isFromJoin,
        int& colStartOffset);
    
    template<typename T>
    SerializeResponse serialize_for_insert(
        pqxx::connection_base& conn, 
        std::vector<std::shared_ptr<T>> const& values);
    template<typename T>
    void update_after_insert(
        pqxx::row const& row,
        std::shared_ptr<T>& obj);
}

template<typename T>
std::vector<std::shared_ptr<T>> _request_instantiator<T>::lookup(
    pqxx::transaction_base& transaction,
    typename T::request returnFields, 
    typename T::request searchFields,
    T const* searchValues,
    PgCompareOp compareOp,
    std::string_view additionalClauses)
{
    auto serialized = serialize_for_lookup(transaction.conn(), returnFields, searchFields, searchValues, compareOp, false, additionalClauses);

    pqxx::result queryResult;
    switch (serialized.statementType) {
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

    for (auto i = 0; i < queryResult.size(); ++i) {
        int colId = 0;
        deserializedResult[i] = deserialize_from_request<T>(queryResult[i], returnFields, false, colId);
    }

    return deserializedResult;
}

template<typename T>
size_t _request_instantiator<T>::insert(
    pqxx::transaction_base& transaction,
    std::vector<std::shared_ptr<T>>& values) 
{
    auto serialized = serialize_for_insert(transaction.conn(), values);

    pqxx::result queryResult;
    switch (serialized.statementType) {
    case SerializeResponse::kNormal: 
        queryResult = transaction.exec_params(serialized.nameOrQuery, pqxx::prepare::make_dynamic_params(serialized.queryParams));
        break;
    case SerializeResponse::kPrepared: 
        queryResult = transaction.exec_prepared(serialized.nameOrQuery, pqxx::prepare::make_dynamic_params(serialized.queryParams));
        break;
    case SerializeResponse::kJoinPart:
        throw std::runtime_error("serialize_for_insert returned a kJoinPart");
        break;
    }

    for (auto i = 0; i < queryResult.size(); ++i)
        update_after_insert(queryResult[i], values[i]);

    return queryResult.size();
}

template<typename T>
typename _id_instantiator<T>::IdType& _id_instantiator<T>::id(T& arg)
{ return arg.id; }
template<>
typename _id_instantiator<Mod>::IdType& _id_instantiator<Mod>::id(Mod& arg)
{ return arg.uuid; }

template<typename T>
typename _id_instantiator<T>::IdType _id_instantiator<T>::id(T const& arg)
{ return arg.id; }
template<>
typename _id_instantiator<Mod>::IdType _id_instantiator<Mod>::id(Mod const& arg)
{ return arg.uuid; }

template<typename T>
std::shared_mutex _id_instantiator<T>::mutex;
template<typename T>
std::map<typename _id_instantiator<T>::IdType, std::weak_ptr<T>> _id_instantiator<T>::ptr_map;

template<typename T>
std::shared_ptr<T> _id_instantiator<T>::from_id(_id_instantiator<T>::IdType const& id) {
    std::unique_lock ulock {mutex, std::defer_lock};
    std::shared_lock slock {mutex};

    auto loc = ptr_map.find(id);
    if (loc != ptr_map.end()) {
        if (!loc->second.expired())
            return loc->second.lock();
        else {
            slock.unlock();
            ulock.lock(); //upgrade

            ptr_map.erase(loc);
        }
    }

    if (slock) slock.unlock();
    if (!ulock) ulock.lock();

    auto ptr = std::make_shared<T>();
    BeatMods::db::id(*ptr) = id;
    ptr->id_valid = true;
    ptr_map.insert({id, ptr});
    return ptr;
}

template<typename T>
std::shared_ptr<T> _id_instantiator<T>::insert(std::shared_ptr<T> const& ptr) {
    std::unique_lock ulock {mutex, std::defer_lock};
    std::shared_lock slock {mutex};

    auto loc = ptr_map.find(id(*ptr));
    if (loc != ptr_map.end()) {
        if (!loc->second.expired())
            return loc->second.lock();
        else {
            slock.unlock();
            ulock.lock(); //upgrade

            ptr_map.erase(loc);
        }
    }

    if (slock) slock.unlock();
    if (!ulock) ulock.lock();
    
    sassert(ptr->id_valid); // must have valid id field to behave
    ptr_map.insert({id(*ptr), ptr});
    return ptr;
}

namespace {

    template<typename TR,
        typename = std::enable_if_t<is_table_request_v<TR>>>
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
        typename = std::enable_if_t<is_table_request_v<TReq>>>
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

    template<typename TV, typename OStream>
    void insert_write_field_placeholder(OStream& sql, TV const& value, int& fid) {
        if constexpr (is_nullable_v<TV>) { \
            if (value.has_value()) sql << "$" << fid++; \
            else sql << "NULL"; \
        } else {
            sql << "$" << fid++; 
        }
    }

    template<typename TV>
    void insert_append_value(TV const& value, std::vector<std::string>& vec) {
        if constexpr (is_nullable_v<TV>) { \
            if (value.has_value()) vec.emplace_back(pqxx::to_string(value)); \
        } else vec.emplace_back(pqxx::to_string(value));
    }


    // TODO: when we get static reflection, kill ALL of this garbage

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
            value->name = row[cid++].as<decltype(value->name)>();

    #define JOIN_GET_RESPONSE(name, remoteId, responseFields, fields, values, conn, compareOp, whereCount) \
        SerializeResponse name ## _response; \
        if (responseFields.name ## _resolve || fields.name ## _resolve) { \
            using type = foreign_key_resolved_t<decltype(values->name)>; \
            type const* uvalues = nullptr; \
            if (values) { \
                auto suv = get_resolved_if(&values->name); \
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
    #define JOIN_FROM_TABLE(name, remoteId, stream, responseFields, inpFields) \
        if (responseFields.name ## _resolve || inpFields.name ## _resolve) \
            stream << " LEFT JOIN " << decltype(responseFields.name ## _request)::_type::table \
                << " ON " << std::remove_reference_t<decltype(responseFields)>::_type::table << \
                ".\"" #name "\" = " << decltype(responseFields.name ## _request)::_type::table << \
                ".\"" #remoteId "\"";
    #define JOIN_DESERIALIZE_FIELD(name, remoteId, responseFields, value, row, cid) \
        if (responseFields.name ## _resolve) \
            value->name = \
                deserialize_from_request<decltype(responseFields.name ## _request)::_type> \
                    (row, responseFields.name ## _request, true, cid);

    #define INSERT_ENCODE_FIELD(name, obj, vec) \
        insert_append_value(obj->name, vec);
    
    #define INSERT_ENCODE_FOREIGN_FIELD(name, _id, obj, vec) \
        if (is_resolved(obj->name)) sassert(get_resolved(obj->name)->id_valid) \
        else sassert(is_unresolved(obj->name)) /* can actually be neither */ \
        INSERT_ENCODE_FIELD(name, obj, vec);

    #define INSERT_VALUE_IDS(name, _, hasBefore, sql,  ...) \
        if (hasBefore) sql << ","; \
        sql << "\"" STR(name) "\""; \
        hasBefore = true;

    #define INSERT_VALUE_VALUES(name, _, vals, i, sql, fid, hasBefore, ...) \
        if (hasBefore) sql << ","; \
        insert_write_field_placeholder(sql, vals[i]->name, fid); \
        hasBefore = true;

    #define SPECIALIZE_SERIALIZER_FOR(_TYPE, idField, auto_ID) \
        template<> \
        SerializeResponse serialize_for_lookup( \
            pqxx::connection_base& conn, \
            _TYPE::request responseFields, \
            _TYPE::request fields, \
            _TYPE const* values, \
            PgCompareOp compareOp, \
            bool joinPart, \
            std::string_view additionalClauses) \
        { \
            auto whereCount = 0; \
            if constexpr (auto_ID) \
                if (fields.idField) ++whereCount; \
            EXEC(_TYPE##_FIELDS (COUNT_WHERE_CLAUSE, fields, whereCount)) \
            if (whereCount > 0) sassert(values != nullptr); \
            \
            EXEC(_TYPE##_FOREIGN_FIELDS (JOIN_GET_RESPONSE, responseFields, fields, values, conn, compareOp, whereCount)) \
            \
            auto generateWhereClause = [=](std::basic_ostream<char>& stream, int& fieldNum) { \
                auto op = to_pg_op(compareOp); \
                stream << "("; \
                bool fieldBefore = false; \
                if constexpr (auto_ID) \
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
                if constexpr (auto_ID) \
                    if (fields.idField) vector.push_back(pqxx::to_string(values->idField)); \
                EXEC(_TYPE##_FIELDS (WHERE_VALUES_FIELD, values, fields, vector)) \
                EXEC(_TYPE##_FOREIGN_FIELDS (JOIN_WHERE_VALUES, vector, fields)); \
                \
                return vector; \
            }; \
            \
            auto generateSelectFields = [=](std::basic_ostream<char>& stream) { \
                bool fieldBefore = false; \
                if constexpr (auto_ID) { \
                    fieldBefore = true; \
                    stream << _TYPE::table << ".\"" #idField "\""; \
                } \
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
                EXEC(_TYPE##_FOREIGN_FIELDS (JOIN_FROM_TABLE, sql, responseFields, fields)) \
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
        } 

    #define SPECIALIZE_DESERIALIZER_FOR(_TYPE, idField, CREATE_FROM_ID) \
        template<> \
        std::shared_ptr<_TYPE> deserialize_from_request( \
            pqxx::row const& row, \
            typename _TYPE::request responseFields, \
            bool isFromJoin, \
            int& cid) \
        { /* prefer deserializing by index */ \
            auto value = EXEC(CREATE_FROM_ID(_TYPE, idField)); \
            EXEC(_TYPE##_FIELDS (BASIC_DESERIALIZE_FIELD, responseFields, row, value, cid)) \
            EXEC(_TYPE##_FOREIGN_FIELDS (JOIN_DESERIALIZE_FIELD, responseFields, value, row, cid)) \
            return value; \
        }

    #define _SPECIALIZE_INSERTER_FOR(_TYPE, idField, INSERT_ID, INSERT_ID_VALUE, RETURN_ID, CHECK_ID_VALID) \
        template<> \
        SerializeResponse serialize_for_insert( \
            pqxx::connection_base& conn, \
            std::vector<std::shared_ptr<_TYPE>> const& values) \
        { \
            std::ostringstream sql {}; \
            \
            int fid = 1; \
            sql << "INSERT INTO " << _TYPE::table << " ("; \
            \
            bool hasBefore = false; \
            EXEC(INSERT_ID(id, sql, hasBefore)); \
            EXEC(_TYPE##_FIELDS(INSERT_VALUE_IDS, _, hasBefore, sql)); \
            EXEC(_TYPE##_FOREIGN_FIELDS(INSERT_VALUE_IDS, hasBefore, sql)); \
            sql << ") "; \
            \
            for (size_t i = 0; i < values.size(); ++i) { \
                hasBefore = false; \
                sql << "VALUES ("; \
                \
                EXEC(INSERT_ID_VALUE(sql, hasBefore)); \
                EXEC(_TYPE##_FIELDS(INSERT_VALUE_VALUES, _, values, i, sql, fid, hasBefore)); \
                EXEC(_TYPE##_FOREIGN_FIELDS(INSERT_VALUE_VALUES, values, i, sql, fid, hasBefore)); \
                sql << ") "; \
            } \
            EXEC(RETURN_ID(id, sql)); \
            \
            SerializeResponse resp {SerializeResponse::kNormal, sql.str()}; \
            resp.queryParams.reserve(fid - 1); \
            \
            for (auto const& ptr : values) { \
                EXEC(CHECK_ID_VALID(ptr)); \
                EXEC(_TYPE##_FIELDS(INSERT_ENCODE_FIELD, ptr, resp.queryParams)); \
                EXEC(_TYPE##_FOREIGN_FIELDS(INSERT_ENCODE_FOREIGN_FIELD, ptr, resp.queryParams)); \
            } \
            \
            return resp; \
        }
    #define SPECIALIZE_INSERTER_FOR(name, id, PPACK) EXEC(PPACK(_SPECIALIZE_INSERTER_FOR, name, id))

    #define SPECIALIZE_INSERT_UPDATE(_TYPE, UPDATE_ID) \
        template<> \
        void update_after_insert( \
            pqxx::row const& row, \
            std::shared_ptr<_TYPE>& obj) \
        { EXEC(UPDATE_ID(row, obj)); }

    #define NO_OP(...) // do nothing
    
    #define DEFAULT_CREATE_FROM_ID(_TYPE, idField) _id_instantiator<_TYPE>::from_id(row[cid++].as<decltype(_TYPE{}.idField)>())
    #define NO_CREATE_FROM_ID(_TYPE, idField) std::make_shared<_TYPE>()

    #define DEFAULT_INSERT_ID(id, sql, hasBefore) sql << "\"" STR(id) "\""; hasBefore = true; // this gets popped for the no id case
    #define DEFAULT_INSERT_ID_VALUE(sql, hasBefore) sql << "DEFAULT"; hasBefore = true; // popped in the no ID case
    #define DEFAULT_RETURN_ID(id, sql) sql << "RETURNING \"" STR(id) "\";";
    #define NO_RETURN_ID(id, sql) sql << ";";
    #define DEFAULT_CHECK_ID_VALID(ptr) sassert(!ptr->id_valid);

    #define DEFAULT_INSERTER_PARAMS(M, ...) EXEC(M(__VA_ARGS__, DEFAULT_INSERT_ID, DEFAULT_INSERT_ID_VALUE, DEFAULT_RETURN_ID, DEFAULT_CHECK_ID_VALID))
    #define NO_OP_INSERTER_PARAMS(M, ...) EXEC(M(__VA_ARGS__, NO_OP, NO_OP, NO_RETURN_ID, NO_OP))

    #define DEFAULT_UPDATE_ID(row, obj) \
        id(*obj) = row[0].as<id_type_t<std::remove_reference_t<decltype(*obj)>>>(); \
        obj->id_valid = true; \
        update_shared(obj); \

    #define SPECIALIZE_WITH_ID(_TYPE, idType) \
        SPECIALIZE_SERIALIZER_FOR(_TYPE, idType, true) \
        SPECIALIZE_DESERIALIZER_FOR(_TYPE, idType, DEFAULT_CREATE_FROM_ID) \
        SPECIALIZE_INSERTER_FOR(_TYPE, idType, DEFAULT_INSERTER_PARAMS) \
        SPECIALIZE_INSERT_UPDATE(_TYPE, DEFAULT_UPDATE_ID) \

    #define SPECIALIZE_WITHOUT_ID(_TYPE, idType) \
        SPECIALIZE_SERIALIZER_FOR(_TYPE, idType, false) \
        SPECIALIZE_DESERIALIZER_FOR(_TYPE, idType, NO_CREATE_FROM_ID) \
        SPECIALIZE_INSERTER_FOR(_TYPE, idType, NO_OP_INSERTER_PARAMS) \
        SPECIALIZE_INSERT_UPDATE(_TYPE, NO_OP)

    // Actual definitions //

    #define _User_FIELDS(M, ...) \
        M(name, __VA_ARGS__) M(profile, __VA_ARGS__) M(created, __VA_ARGS__) M(githubId, __VA_ARGS__)
    #define User_FIELDS(M, ...) EXEC(_User_FIELDS(M, __VA_ARGS__))
    #define User_FOREIGN_FIELDS(M, ...)

    SPECIALIZE_WITH_ID(User, id)

    #define _NewsItem_FIELDS(M, ...) \
        M(title, __VA_ARGS__) M(body, __VA_ARGS__) M(posted, __VA_ARGS__) \
        M(edited, __VA_ARGS__) M(system, __VA_ARGS__) 
    #define NewsItem_FIELDS(M, ...) EXEC(_NewsItem_FIELDS(M, __VA_ARGS__))
    #define _NewsItem_FOREIGN_FIELDS(M, ...) \
        M(author, id, __VA_ARGS__)
    #define NewsItem_FOREIGN_FIELDS(M, ...) EXEC(_NewsItem_FOREIGN_FIELDS(M, __VA_ARGS__))

    SPECIALIZE_WITH_ID(NewsItem, id)

    #define _Tag_FIELDS(M, ...) \
        M(name, __VA_ARGS__)
    #define Tag_FIELDS(M, ...) EXEC(_Tag_FIELDS(M, __VA_ARGS__))
    #define Tag_FOREIGN_FIELDS(M, ...)

    SPECIALIZE_WITH_ID(Tag, id)

    #define _GameVersion_FIELDS(M, ...) \
        M(version, __VA_ARGS__) M(steamBuildId, __VA_ARGS__) M(visibility, __VA_ARGS__) 
    #define GameVersion_FIELDS(M, ...) EXEC(_GameVersion_FIELDS(M, __VA_ARGS__))
    #define GameVersion_FOREIGN_FIELDS(M, ...)

    SPECIALIZE_WITH_ID(GameVersion, id)

    #define _Group_FIELDS(M, ...) \
        M(name, __VA_ARGS__) M(permissions, __VA_ARGS__)
    #define Group_FIELDS(M, ...) EXEC(_Group_FIELDS(M, __VA_ARGS__))
    #define Group_FOREIGN_FIELDS(M, ...)

    SPECIALIZE_WITH_ID(Group, id)

    #define _Mod_FIELDS(M, ...) \
        M(id, __VA_ARGS__) M(name, __VA_ARGS__) M(description, __VA_ARGS__) \
        M(version, __VA_ARGS__) M(system, __VA_ARGS__) M(uploaded, __VA_ARGS__) \
        M(approvalState, __VA_ARGS__) M(approved, __VA_ARGS__) M(required, __VA_ARGS__) \
        M(dependsOn, __VA_ARGS__) M(conflictsWith, __VA_ARGS__)
    #define Mod_FIELDS(M, ...) EXEC(_Mod_FIELDS(M, __VA_ARGS__))
    #define _Mod_FOREIGN_FIELDS(M, ...) \
        M(author, id, __VA_ARGS__) M(gameVersion, id, __VA_ARGS__)
    #define Mod_FOREIGN_FIELDS(M, ...) EXEC(_Mod_FOREIGN_FIELDS(M, __VA_ARGS__))

    SPECIALIZE_WITH_ID(Mod, uuid)
    
    #define _Download_FIELDS(M, ...) \
        M(type, __VA_ARGS__) M(cdnFile, __VA_ARGS__) M(hashes, __VA_ARGS__) 
    #define Download_FIELDS(M, ...) EXEC(_Download_FIELDS(M, __VA_ARGS__))
    #define _Download_FOREIGN_FIELDS(M, ...) \
        M(mod, uuid, __VA_ARGS__)
    #define Download_FOREIGN_FIELDS(M, ...) EXEC(_Download_FOREIGN_FIELDS(M, __VA_ARGS__))

    SPECIALIZE_WITHOUT_ID(Download, mod)

    #define GameVersion_VisibleGroups_JoinItem_FIELDS(M, ...)
    #define _GameVersion_VisibleGroups_JoinItem_FOREIGN_FIELDS(M, ...) \
        M(gameVersion, id, __VA_ARGS__) M(group, id, __VA_ARGS__)
    #define GameVersion_VisibleGroups_JoinItem_FOREIGN_FIELDS(M, ...) EXEC(_GameVersion_VisibleGroups_JoinItem_FOREIGN_FIELDS(M, __VA_ARGS__))

    SPECIALIZE_WITHOUT_ID(GameVersion_VisibleGroups_JoinItem, group)

    #define Mods_Tags_JoinItem_FIELDS(M, ...)
    #define _Mods_Tags_JoinItem_FOREIGN_FIELDS(M, ...) \
        M(mod, uuid, __VA_ARGS__) M(tag, id, __VA_ARGS__)
    #define Mods_Tags_JoinItem_FOREIGN_FIELDS(M, ...) EXEC(_Mods_Tags_JoinItem_FOREIGN_FIELDS(M, __VA_ARGS__))

    SPECIALIZE_WITHOUT_ID(Mods_Tags_JoinItem, mod)

    #define Users_Groups_JoinItem_FIELDS(M, ...)
    #define _Users_Groups_JoinItem_FOREIGN_FIELDS(M, ...) \
        M(user, id, __VA_ARGS__) M(group, id, __VA_ARGS__)
    #define Users_Groups_JoinItem_FOREIGN_FIELDS(M, ...) EXEC(_Users_Groups_JoinItem_FOREIGN_FIELDS(M, __VA_ARGS__))

    SPECIALIZE_WITHOUT_ID(Users_Groups_JoinItem, user)

    using namespace state;
    #define _LogItem_FIELDS(M, ...) \
        M(time, __VA_ARGS__) M(message, __VA_ARGS__) M(level, __VA_ARGS__)
    #define LogItem_FIELDS(M, ...) EXEC(_LogItem_FIELDS(M, __VA_ARGS__))
    #define LogItem_FOREIGN_FIELDS(M, ...)
    
    SPECIALIZE_WITH_ID(LogItem, id)

    #define _Token_FIELDS(M, ...) \
        M(token, __VA_ARGS__) 
    #define Token_FIELDS(M, ...) EXEC(_Token_FIELDS(M, __VA_ARGS__))
    #define _Token_FOREIGN_FIELDS(M, ...) \
        M(user, id, __VA_ARGS__)
    #define Token_FOREIGN_FIELDS(M, ...) EXEC(_Token_FOREIGN_FIELDS(M, __VA_ARGS__))

    SPECIALIZE_WITHOUT_ID(Token, token)

}