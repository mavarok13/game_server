#pragma once

#include <memory>
#include <mutex>
#include <condition_variable>

#include <pqxx/pqxx>

#include "app.h"

namespace database {

using pqxx::operator""_zv;

static constexpr auto SELECT_RESULTS_TAG = "select_result"_zv;
static constexpr auto INSERT_RESULT_TAG = "insert_result"_zv;

class ConnectionPool {
public:
    class ConnectionHolder {
    public:
        explicit ConnectionHolder(std::shared_ptr<pqxx::connection> conn, ConnectionPool & pool) : conn_{conn}, pool_{&pool} {}

        ConnectionHolder(const ConnectionHolder&) = delete;
        ConnectionHolder& operator=(const ConnectionHolder&) = delete;

        ConnectionHolder(ConnectionHolder&&) = default;
        ConnectionHolder& operator=(ConnectionHolder&&) = default;

        pqxx::connection& operator*() const& noexcept {
            return *conn_;
        }
        pqxx::connection& operator*() const&& = delete;

        pqxx::connection* operator->() const& noexcept {
            return conn_.get();
        }

        ~ConnectionHolder() {
            if (conn_) {
                pool_->ReturnConnection(std::move(*conn_));
            }
        }
    private:
        std::shared_ptr<pqxx::connection> conn_;
        ConnectionPool * pool_;
    };

    ConnectionPool(unsigned int size, std::string_view connection_string) {
        connections_.reserve(size);

        for (int conn_idx = 0; conn_idx < size; ++conn_idx) {
            connections_.emplace_back(connection_string.data());

            if (conn_idx == 0) {
                pqxx::work w(connections_.back());

                w.exec("CREATE TABLE IF NOT EXISTS retired_players(name TEXT NOT NULL, scores INTEGER DEFAULT(0), playTime INTEGER NOT NULL);"_zv);
                w.exec("CREATE INDEX IF NOT EXISTS top_retired_players_index ON retired_players(scores DESC, playTime ASC, name ASC);"_zv);

                w.commit();
            }

            connections_.back().prepare(SELECT_RESULTS_TAG, "SELECT name, scores, playTime FROM retired_players ORDER BY scores DESC, playTime ASC, name ASC LIMIT $1 OFFSET $2;"_zv);
            connections_.back().prepare(INSERT_RESULT_TAG, "INSERT INTO retired_players(name, scores, playTime) VALUES($1, $2, $3);"_zv);
        }
    }

    ConnectionHolder GetConnection() {
        std::unique_lock lock{m_};

        cv_.wait(lock, [this] {
            return using_connections_ < connections_.size();
        });

        ConnectionHolder conn_holder = ConnectionHolder(std::make_shared<pqxx::connection>(std::move(connections_.back())), *this);
        connections_.pop_back();
        ++using_connections_;

        return conn_holder;
    }

private:
    void ReturnConnection(pqxx::connection && conn) {
        std::unique_lock lock{m_};

        --using_connections_;
        connections_.emplace_back(std::move(conn));

        cv_.notify_one();
    }

    std::mutex m_;
    std::condition_variable cv_;
    std::vector<pqxx::connection> connections_;
    unsigned int using_connections_ = 0;
};

class GameResultRepository : public app::IGameResultRepository {
public:
    explicit GameResultRepository(pqxx::work & work) : work_{work} {}

    std::vector<app::GameResult> GetResults(unsigned int results_count = max_results_count_, unsigned int offset = 0) override {
        std::vector<app::GameResult> results;
        results.reserve(results_count);

        pqxx::result result = work_.exec_prepared(SELECT_RESULTS_TAG, results_count, offset);

        for (auto row = result.begin(); row != result.end(); ++row) {
            std::string name = row->at(0).as<std::string>();
            unsigned int scores = row->at(1).as<unsigned int>();
            unsigned int session_duration = row->at(2).as<unsigned int>();
            results.emplace_back(name, scores, session_duration);
        }

        return results;
    }
    void AddResult(const app::GameResult & game_result) override {
        work_.exec_prepared(INSERT_RESULT_TAG, game_result.name_, game_result.scores_, game_result.session_duration_);
    }
private:
    static const unsigned int max_results_count_ = 100;

    pqxx::work & work_;
};

class DbUnitOfWork : public app::IUnitOfWork {
public:
    using ConnectionHolder = ConnectionPool::ConnectionHolder;

    explicit DbUnitOfWork(ConnectionHolder && conn) : conn_{std::move(conn)}, work_{*conn_}, game_result_repos_{work_} {}

    ~DbUnitOfWork() {
        Commit();
    }

    app::IGameResultRepository * Results() override {
        return &game_result_repos_;
    }

    void Commit() override {
        work_.commit();
    }
private:
    ConnectionHolder conn_;
    pqxx::work work_;
    GameResultRepository game_result_repos_;
};

class DbUnitOfWorkFactory : public app::IUnitOfWorkFactory {
public:
    explicit DbUnitOfWorkFactory(std::shared_ptr<ConnectionPool> pool) : pool_{pool} {}

    std::shared_ptr<app::IUnitOfWork> NewUnitOfWork() override {
        return std::make_shared<DbUnitOfWork>(std::move(pool_->GetConnection()));
    }
private:
    std::shared_ptr<ConnectionPool> pool_;
};

} //namespace database