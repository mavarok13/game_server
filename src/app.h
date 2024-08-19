#pragma once

#include <random>
#include <sstream>
#include <iostream>
#include <cstring>
#include <chrono>

#include <boost/signals2.hpp>

#include "model.h"

namespace sig = boost::signals2;

namespace app {

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
    Player (std::string token, int player_id, std::string player_name, model::GameSession * session) : token_(token), player_id_(player_id), player_name_(player_name), session_(session) {}

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

    model::GameSession * GetSession() {
        return session_;
    }

    model::GameSession * GetSession() const {
        return session_;
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

private:
    model::GameSession * session_;
    std::string token_;
    int player_id_;
    std::string player_name_;

    unsigned int scores_ = 0;
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

    Player AddNewPlayer(std::string player_name, model::GameSession * session) {
        std::string token{PlayerTokens::Instance().GetToken()};
        Player player{token, player_id_++, player_name, session};

        players_.emplace_back(player);

        session->NewPlayer(player.GetPlayerId());

        return player;
    }

    Player * GetPlayerByToken(std::string token) {
        Player * player = 0;

        for (Player & p : players_) {
            if (std::strcmp(p.GetToken().c_str(), token.c_str()) == 0) {
                player = &p;
                break;
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
private:
    PlayersManager() {}

    int player_id_ = 0;
    std::vector<Player> players_;
};
} //namespace app