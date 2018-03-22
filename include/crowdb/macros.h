#pragma once

//#include <boost/preprocessor/config/config.hpp>
#include <boost/preprocessor.hpp>

#ifndef BOOST_PP_VARIADICS
#error Variadic macro is not supported. crow::db is unavailable.
#endif

#define CROW_DB_I_TABLE_MEMBERS_AUX(r, data, elem) crow::db::wrap<BOOST_PP_TUPLE_ELEM(1, elem)> BOOST_PP_TUPLE_ELEM(0, elem);
#define CROW_DB_I_TABLE_MEMBERS(list) BOOST_PP_LIST_FOR_EACH(CROW_DB_I_TABLE_MEMBERS_AUX, _, list)
#define CROW_DB_TABLE_WITH_NAME(table_name, real_name, ...) \
    struct table_name \
    { \
        static constexpr char const * const _table_name = real_name;\
        CROW_DB_I_TABLE_MEMBERS(BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__)) \
        void* _session{nullptr}; \
    };
#define CROW_DB_TABLE(table_name, ...) CROW_DB_TABLE_WITH_NAME(table_name, #table_name, __VA_ARGS__)
#define CROW_DB_SP(sp_name, prototype)
