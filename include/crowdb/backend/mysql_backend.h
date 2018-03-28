#pragma once

#include "../db.h"
#include <mysql.h>
#include <stdexcept>
#include <tuple>
#include <mutex>
#include <boost/lexical_cast.hpp>
#include <deque>
#include <iostream>
#include <chrono>

namespace crow
{
    namespace db
    {
        namespace mpl
        {
            template <typename ... T>
            struct first;
            template <typename T1, typename ... T>
            struct first<T1, T...>
            {
                using type = T1;
            };
            template <>
            struct first<>
            {
                using type = long long;
            };
            template <typename ... T>
            using first_t = typename first<T...>::type;


            template<class T> using Invoke = typename T::type;
            template<unsigned...> struct seq{ using type = seq; };

            template<class S1, class S2> struct concat;

            template<unsigned... I1, unsigned... I2>
            struct concat<seq<I1...>, seq<I2...>>
              : seq<I1..., (sizeof...(I1)+I2)...>{};

            template<class S1, class S2>
            using Concat = Invoke<concat<S1, S2>>;

            template<unsigned N> struct gen_seq;
            template<unsigned N> using GenSeq = Invoke<gen_seq<N>>;

            template<unsigned N>
            struct gen_seq : Concat<GenSeq<N/2>, GenSeq<N - N/2>>{};

            template<> struct gen_seq<0> : seq<>{};
            template<> struct gen_seq<1> : seq<0>{};
        }
        namespace mysql
        {
            template <typename T>
            T convert_data(char* data, MYSQL_FIELD& /*field*/, unsigned long length)
            {
                if (!data)
                    return {};
                return boost::lexical_cast<T>(data, length);
            }

            template <typename ...ColumnType>
            class ResultProxy;

            template <typename ...ColumnType>
            class RowProxy
            {
                public:
                    RowProxy(ResultProxy<ColumnType...>* self, std::tuple<ColumnType...>&& data)
                        : self_(self),
                        data_(std::move(data))
                    {
                    }
                    operator std::tuple<ColumnType...>& () { return data_; }
                    ResultProxy<ColumnType...>* self_;
                    std::tuple<ColumnType...> data_;
            };

            template <typename ...ColumnType>
            class ResultProxy
            {
                public:
                    struct iterator 
                    {
                        iterator() {}
                        iterator(ResultProxy* self) : self_(self)
                        {
                        }

                        template <unsigned ... Index>
                        RowProxy<ColumnType...> to_tuple(mpl::seq<Index...>)
                        {
                            if (!self_ || !self_->row_)
                            {
                                throw std::runtime_error("row not exist");
                            }
                            return {self_, std::make_tuple(
                                    convert_data<typename std::tuple_element<Index, std::tuple<ColumnType...>>::type>(
                                        self_->row_[Index],
                                        self_->fields_[Index],
                                        self_->lengths_[Index]
                                    )...
                            )};
                        }

                        RowProxy<ColumnType...> operator*() 
                        {
                            return to_tuple(mpl::gen_seq<sizeof...(ColumnType)>());
                        }

                        iterator& operator++() 
                        {
                            if (self_)
                                self_->row_ = mysql_fetch_row(self_->res_);
                            return *this;
                        }

                        bool operator != (const iterator& rhs) const 
                        {
                            return 
                                !((self_ == nullptr || self_->row_ == nullptr) && 
                                (rhs.self_ == nullptr || rhs.self_->row_ == nullptr));
                        }

                        ResultProxy* self_{nullptr};
                    };
                public:
                    ResultProxy(MYSQL& mysql)
                        : mysql_(mysql),
                        res_(mysql_use_result(&mysql))
                    {
                        if (res_)
                        {
                            row_ = mysql_fetch_row(res_);
                            fields_ = mysql_fetch_fields(res_);
                            lengths_ = mysql_fetch_lengths(res_);
                        }
                    }

                    ~ResultProxy()
                    {
                        if (res_)
                        { 
                            if (row_ != nullptr)
                                while(mysql_fetch_row(res_) != nullptr);
                            mysql_free_result(res_);
                        }
                    }

                    size_t rowcount() { return mysql_affected_rows(&mysql_); }

                    //std::tuple<ColumnType...> fetchone();
                    mpl::first_t<ColumnType...> scalar()
                    {
                        return std::get<0>((*begin()).data_);
                    }

                    iterator begin() { return {this}; }
                    iterator end() { return {}; }
                private:
                    MYSQL& mysql_;
                    MYSQL_RES* res_{nullptr};
                    MYSQL_ROW row_{nullptr};
                    MYSQL_FIELD* fields_{nullptr};
                    unsigned long* lengths_{nullptr};
                    friend struct iterator;
            };
            class Engine;
            class Connection : public crow::db::Connection
            {
                public:
                    Connection(Engine* engine);
                    ~Connection()
                    {
                        mysql_close(&mysql);
                    }

                    MYSQL* native_handle() { return &mysql; }

                    ResultProxy<int>
                    execute(const std::string& query)
                    {
                        if (mysql_real_query(&mysql, query.data(), query.size()))
                        {
                            fprintf(stderr, "query execution failed : %s\n%d\n%s\n", query.c_str(), (int)query.size(), mysql_error(&mysql));
                            throw std::runtime_error("query execution failed: " + query);
                        }
                        return {mysql};
                    }

                    template <typename ... ColumnType>
                    ResultProxy<ColumnType...>
                    execute(const std::string& query)
                    {
                        if (mysql_real_query(&mysql, query.data(), query.size()))
                        {
                            fprintf(stderr, "query execution failed : %s\n%d\n%s\n", query.c_str(), (int)query.size(), mysql_error(&mysql));
                            throw std::runtime_error("query execution failed: " + query);
                        }
                        return {mysql};
                    }

                    Engine* engine;
            private:
                    MYSQL mysql;
            };

            class Engine
            {
            public:
                Engine(std::string url) 
                {
                    // url: "mysql://ID:PW@HOST/DB"
                    if (url.substr(0, 8) != "mysql://")
                        throw std::runtime_error("engine mismatch");
                    url = url.substr(8);
                    auto index = url.find(':');
                    user = url.substr(0, index);
                    index++;
                    auto next_index = url.find('@', index);
                    pw = url.substr(index, next_index - index);
                    index = next_index + 1;
                    port = 3306;
                    next_index = url.find(':', index);
                    if (next_index != url.npos)
                    {
                        host = url.substr(index, next_index - index);
                        next_index = url.find('/', index);
                        port = std::stoi(url.substr(index, next_index - index));
                    }
                    else
                    {
                        next_index = url.find('/', index);
                        host = url.substr(index, next_index - index);
                    }
                    index = next_index + 1;

                    use_db = url.substr(index);
                }
                Engine() {}
                struct ReturnToPool
                {
                    void operator()(Connection* conn)
                    {
                        std::lock_guard<std::mutex> lg(conn->engine->mtx_);
                        conn->engine->pool_.emplace_back(
                                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(), 
                                conn);
                    }
                };
                std::unique_ptr<Connection, ReturnToPool> connect()
                {
                    mtx_.lock();
                    if (pool_.empty())
                    {
                        mtx_.unlock();
                        return std::unique_ptr<Connection, ReturnToPool>(new Connection (this));
                    }
                    Connection* c = pool_[0].second;
                    uint64_t tick = pool_[0].first;
                    pool_.pop_front();
                    mtx_.unlock();
                    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() - tick > 10000)
                    {
                        int ret = mysql_ping(c->native_handle());
                        if (ret != 0)
                        {
                            return std::unique_ptr<Connection, ReturnToPool>(new Connection(this));
                        }
                    }
                    return std::unique_ptr<Connection, ReturnToPool>(c);
                }
            private:
                std::mutex mtx_;
                std::deque<std::pair<uint64_t, Connection*>> pool_;

                friend class Connection;
                std::string use_db;
                std::string user;
                std::string pw;
                std::string host;
                int port;
            };
            using engine = Engine;
            inline Connection::Connection(Engine* engine)
                : engine(engine)
            {
                mysql_init(&mysql);
                //static const char* DB_HOST = "jypmaindb.cibomca45wzs.ap-northeast-1.rds.amazonaws.com";
                //if (!mysql_real_connect(&mysql, DB_HOST, "jypmaster", "wlsdudqkr0-0-", "jypmaster", 3306, nullptr, 0))
                if (!mysql_real_connect(&mysql, engine->host.c_str(), engine->user.c_str(), engine->pw.c_str(), engine->use_db.c_str(), engine->port, nullptr, 0))
                {
                    throw std::runtime_error("failed to connect to mysql(" + std::string(mysql_error(&mysql)) + ")");
                }
                mysql_set_character_set(&mysql, "utf8");
            }

        }


        template <int N, typename ... T>
        auto get(crow::db::mysql::RowProxy<T...>& row) -> decltype(std::get<N>(static_cast<std::tuple<T...>&>(row)))
        {
            return std::get<N>(static_cast<std::tuple<T...>&>(row));
        }
    }
}
