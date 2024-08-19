#pragma once

#include <cstring>
#include <chrono>
#include <iostream>
#include <random>
#include <sstream>
#include <memory>
#include <vector>

#include <boost/signals2.hpp>

#include "model.h"

namespace sig = boost::signals2;

namespace app {

using namespace std::chrono_literals;

class Application {
public:
    using TickHandler = sig::signal<void(std::chrono::milliseconds)>;

    [[nodiscard]] sig::connection DoOnTick(const TickHandler::slot_type & handler) {
        return tick_signal_.connect(handler);
    }

    void Tick(std::chrono::milliseconds delta_time) {
        tick_signal_(delta_time);
    }
private:
    TickHandler tick_signal_;
};

class Player {
public:
    Player (const std::string & token, int player_id, const std::string & player_name, unsigned int session_id) : token_(token), player_id_(player_id), player_name_(player_name), session_id_(session_id) {}

    std::string GetToken() {
        return token_;
    }

    std::string GetToken() const {
        return token_;
    }

    int GetPlayerId() {
        return player_id_;
    }

    int GetPlayerId() const {
        return player_id_;
    }

    std::string GetPlayerName() {
        return player_name_;
    }

    std::string GetPlayerName() const {
        return player_name_;
    }

    unsigned int GetSessionId() const {
        return session_id_;
    }

    unsigned int GetScores() noexcept {
        return scores_;
    }

    unsigned int GetScores() const noexcept {
        return scores_;
    }

    void AddScores(unsigned int scores) {
        scores_ += scores;
    }

    std::chrono::duration<double> GetPlayingTime() {
        return playing_time_;
    }

    void AddPlayingTime(std::chrono::duration<double> time) {
        playing_time_ += time;
    }

    std::chrono::duration<double> GetIdleTime() {
        return idle_time_;
    }

    void AddIdleTime(std::chrono::duration<double> time) {
        idle_time_ += time;
    }

    void ResetIdleTime() {
        idle_time_ = 0.0s;
    }

private:
    std::string token_;
    int player_id_;
    std::string player_name_;
    unsigned int session_id_;

    unsigned int scores_ = 0;

    std::chrono::duration<double> playing_time_ = 0.0s;
    std::chrono::duration<double> idle_time_ = 0.0s;
};

class PlayerTokens {
public:
    static PlayerTokens & Instance() {

        static PlayerTokens pt;

        return pt;
    }

    std::string GetToken() {
        std::stringstream ss;
        std::stringstream half_hex_string;

        auto gen1 = generator1_();
        auto gen2 = generator2_();

        half_hex_string << std::hex << gen1;

        if (half_hex_string.str().size() < 16) {

            for (int i = 0; i < 16 - half_hex_string.str().size(); ++i) {
                ss << "0";
            }
        }

        ss << half_hex_string.str();

        half_hex_string.str("");
        half_hex_string << std::hex << gen2;

        if (half_hex_string.str().size() < 16) {

            for (int i = 0; i < 16 - half_hex_string.str().size(); ++i) {
                ss << "0";
            }
        }

        ss << half_hex_string.str();

        return ss.str();
    }

private:
    PlayerTokens() {}

    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
}; 

class PlayersManager {
public:
    static PlayersManager & Instance() {
        static PlayersManager pm;

        return pm;
    }

    Player & AddNewPlayer(const std::string & player_name, model::GameSession * session) {
        std::string token{PlayerTokens::Instance().GetToken()};

        players_.emplace_back(token, player_id_++, player_name, session->GetId());

        session->NewPlayer(players_.back().GetPlayerId());

        return players_.back();
    }

    Player * GetPlayerByToken(const std::string & token) {
        Player * player = 0;

        for (Player & p : players_) {
            if (std::strcmp(p.GetToken().c_str(), token.c_str()) == 0) {
                player = &p;
                break;
            }
        }

        return player;
    }

    Player * GetPlayerById(int id) {
        Player * player = 0;

        for (Player & p : players_) {
            if (p.GetPlayerId() == id) {
                player = &p;
            }
        }

        return player;
    }

    std::vector<Player> & GetPlayers() {
        return players_;
    }

    const std::vector<Player> & GetPlayers() const {
        return players_;
    }

    int GetNextPlayerId() const {
        return player_id_;
    }

    void SetNextPlayerId(int player_id) {
        player_id_ = player_id;
    }

    void RemovePlayer(unsigned int id) {
        for (auto player_it = players_.begin(); player_it != players_.end(); ++player_it) {
            if (id == player_it->GetPlayerId()) {
                players_.erase(player_it);
                break;
            }
        }
    }
private:
    PlayersManager() {}

    int player_id_ = 0;
    std::vector<Player> players_;
};

struct GameResult {
public:
    GameResult (std::string_view name, unsigned int scores, unsigned int session_duration) : name_{name.begin(), name.end()}, scores_{scores}, session_duration_{session_duration} {}

    std::string name_;
    unsigned int scores_;
    unsigned int session_duration_;
};

class IGameResultRepository {
public:
    virtual std::vector<GameResult> GetResults(unsigned int offset = 0, unsigned int results_count = max_results_count_) = 0;
    virtual void AddResult(const app::GameResult & game_result) = 0;
protected:
    ~IGameResultRepository() {}
private:
    static const unsigned int max_results_count_ = 100;
};

class IUnitOfWork {
public:
    virtual IGameResultRepository * Results() = 0;
    virtual void Commit() = 0;
protected:
    ~IUnitOfWork() {}

};

class IUnitOfWorkFactory {
public:
    virtual std::shared_ptr<IUnitOfWork> NewUnitOfWork() = 0; 

protected:
    ~IUnitOfWorkFactory() {}
};
} //namespace app