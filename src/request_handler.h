#define BOOST_BEAST_USE_STD_STRING_VIEW

#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>

#include <filesystem>
#include <iostream>
#include <syncstream>
#include <iostream>
#include <chrono>

#include "model.h"
#include "app.h"
#include "http_server.h"
#include "json_builder.h"
#include "endpoint.h"
#include "http_path_utils.h"
#include "http_content_type.h"
#include "extra_data.h"
#include "loot_generator.h"

namespace http_handler {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
namespace json = boost::json;

using namespace std::literals;

using HttpRequest = http::request<http::string_body>;

using HttpResponse = http::response<http::string_body>;
using HttpFileResponse = http::response<http::file_body>;

namespace fs = std::filesystem;

HttpResponse ConstructJsonResponse (http::status status, unsigned version, bool keep_alive = false);
HttpFileResponse ConstructFileResponse (HttpRequest req, const fs::path & root, sys::error_code & ec);
HttpResponse ConstructMethodNotAllowedResponse (HttpRequest request, std::string_view methods);

class RequestHandler {
public:

    using Strand = net::strand<net::io_context::executor_type>;

    RequestHandler(model::Game& game, loot_gen::LootGenerator& generator, std::vector<extra_data::MapExtraData> maps_extra_data, Strand strand, const fs::path & root)
        : game_{game}, generator_{generator}, maps_extra_data_{maps_extra_data}, strand_{strand}, root_{root} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;    

    template <typename Send>
    void operator()(HttpRequest&& req, Send&& send) {
        
        /* СЮДА НАЧАЛЬНУЮ ТОЧКУ ВРЕМЕНИ */
        std::chrono::time_point<std::chrono::system_clock> start_response_time = std::chrono::system_clock::now();

        std::string_view target_str_encoded{req.target()};

        req.target(http_path_utils::UrlUncode(target_str_encoded).c_str());

        HttpResponse response = ConstructJsonResponse(http::status::not_found, req.version());
        response.body() = json_builder::GetPageNotFound_s();

        std::vector<endpoint::Endpoint> endpoints;

// *    Mapping:
// *    /api/v1/maps
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {

            HttpResponse res = ConstructJsonResponse(http::status::ok, req.version());
            res.set(http::field::cache_control, "no-cache");
            res.body() = json_builder::GetMaps_s(self->game_.GetMaps());
            res.prepare_payload();
            send(std::move(res), start_response_time);
        }));

// *    Mapping:
// *    /api/v1/maps/{map_id}/
// *    Methods: GET, HEAD
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {

            HttpResponse res;
            
            fs::path target_path{fs::weakly_canonical(fs::path{req.target()}.filename())};

            std::string map_id{target_path.c_str()};

            const model::Map * map = self->game_.FindMap(model::Map::Id{map_id});

            if (map) {

                extra_data::MapExtraData * map_extra_data;
                for (extra_data::MapExtraData & ed : self->maps_extra_data_) {
                    if (ed.GetMapId() == *(map->GetId())) {
                        map_extra_data = &ed;
                        break;
                    }
                }

                res = ConstructJsonResponse(http::status::ok, req.version());
                res.body() = json_builder::GetMapWithExtraData_s(*map, *map_extra_data); 
            }
            else { 
                res = ConstructJsonResponse(http::status::not_found, req.version());
                res.body() = json_builder::GetMapNotFound_s();
            }

            res.set(http::field::cache_control, "no-cache");
            res.prepare_payload();
            send(std::move(res), start_response_time);
        }, 1));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::head, [self = this, start_response_time, &send] (const HttpRequest & req) {

            HttpResponse res;
            
            fs::path target_path{fs::weakly_canonical(fs::path{req.target()}.filename())};

            std::string map_id{target_path.c_str()};

            const model::Map * map = self->game_.FindMap(model::Map::Id{map_id});

            if (map) {
                res = ConstructJsonResponse(http::status::ok, req.version());
            } else { 
                res = ConstructJsonResponse(http::status::not_found, req.version());
            }

            res.set(http::field::cache_control, "no-cache");
            res.prepare_payload();
            send(std::move(res), start_response_time);
        }, 1));


// *    Mapping:
// *    /api/v1/maps/{map_id}
// *    Methods: POST, PUT, DELETE, OPTIONS
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::post, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "GET, HEAD")};
            send(std::move(res), start_response_time);
        }, 1));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::delete_, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "GET, HEAD")};
            send(std::move(res), start_response_time);
        }, 1));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::options, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "GET, HEAD")};
            send(std::move(res), start_response_time);
        }, 1));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::put, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "GET, HEAD")};
            send(std::move(res), start_response_time);
        }, 1));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::patch, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "GET, HEAD")};
            send(std::move(res), start_response_time);
        }, 1));

// *    Join, METHOD: POST
// *    Mapping:
// *    /api/v1/game/join
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/join"}, http::verb::post, [self = this, start_response_time, &send] (const HttpRequest & req) {

            net::dispatch(self->strand_, [self, start_response_time, &send, &req] {
                HttpResponse res;

                sys::error_code ec;
                json::value request_body_val = json::parse(req.body().data(), ec);

                try {
                    if (req.at(http::field::content_type) != http_content_type::JSON) {
                        throw "Content-Type is invalid";
                    }
                } catch (...) {
                    res = ConstructJsonResponse(http::status::bad_request, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = json_builder::GetParseJsonError_s("Content-Type is invalid");
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }

                if (ec) {

                    res = ConstructJsonResponse(http::status::bad_request, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = json_builder::GetParseJsonError_s("Join game request parse error");
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }

                if (!request_body_val.as_object().contains("userName")) {

                    json::value val = {{"code", "invalidArgument"},{"message", "Join game request parse error"}};

                    res = ConstructJsonResponse(http::status::bad_request, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = json::serialize(val);
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }

                if (!request_body_val.as_object().contains("mapId")) {

                    json::value val = {{"code", "invalidArgument"},{"message", "Join game request parse error"}};

                    res = ConstructJsonResponse(http::status::bad_request, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = json::serialize(val);
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }

                std::string player_name{request_body_val.at("userName").as_string()};
                std::string map_id{request_body_val.at("mapId").as_string()};

                if (player_name.size() == 0) {

                    json::value val = {{"code", "invalidArgument"},{"message", "Invalid player name"}};

                    res = ConstructJsonResponse(http::status::bad_request, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = json::serialize(val);
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }

                const model::Map * map = self->game_.FindMap(model::Map::Id{map_id});

                if (map == nullptr) {

                    json::value val = {{"code", "mapNotFound"}, {"message", "Map not found"}};

                    res = ConstructJsonResponse(http::status::not_found, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = json::serialize(val);
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }

                model::GameSession * session = self->game_.NewSession(const_cast<model::Map *>(map));

                app::Player player = app::PlayersManager::Instance().AddNewPlayer(player_name, session);

                json::value value = {{"authToken", player.GetToken()}, {"playerId", player.GetPlayerId()}};

                res = ConstructJsonResponse(http::status::ok, req.version());
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(value);
                res.prepare_payload();

                return send(std::move(res), start_response_time);
            });
        }));

// *    !!!FIX THIS
// *    Join, METHOD: GET, PUT, DELETE, HEAD
// *    /api/v1/game/join
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/join"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/join"}, http::verb::put, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/join"}, http::verb::delete_, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/join"}, http::verb::head, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/join"}, http::verb::options, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/join"}, http::verb::patch, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));


// *    Mapping:
// *    /api/v1/game/players
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/players"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {

            HttpResponse res;
            
            try {
                std::string token{req.at(http::field::authorization).data(), req.at(http::field::authorization).size()};

                token.erase(0, 7);
                token.erase(32, 3);

                if (token.size() != 32) throw "Invalid token";

                app::Player * player = app::PlayersManager::Instance().GetPlayerByToken(token);

                if (player == nullptr) {
                    json::value val = {{"code", "unknownToken"}, {"message", "Player token has not been found"}};

                    res = ConstructJsonResponse(http::status::unauthorized, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = json::serialize(val);
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }

                res = ConstructJsonResponse(http::status::ok, req.version());
                res.set(http::field::cache_control, "no-cache");
                res.body() = json_builder::GetPlayers_s(player->GetSession());
                res.prepare_payload();

                return send(std::move(res), start_response_time);
            } catch (...) {

                json::value val = {{"code", "invalidToken"}, {"message", "Authorization header is missing"}};

                res = ConstructJsonResponse(http::status::unauthorized, req.version());
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(val);
                res.prepare_payload();

                return send(std::move(res), start_response_time);
            }
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/players"}, http::verb::head, [self = this, start_response_time, &send] (const HttpRequest & req) {

            HttpResponse res;
            
            try {
                std::string token{req.at(http::field::authorization).data(), req.at(http::field::authorization).size()};

                token.erase(0, 7);
                token.erase(32, 3);

                if (token.size() != 32) throw "Invalid token";

                app::Player * player = app::PlayersManager::Instance().GetPlayerByToken(token);

                if (player == nullptr) {

                    res = ConstructJsonResponse(http::status::unauthorized, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }

                res = ConstructJsonResponse(http::status::ok, req.version());
                res.set(http::field::cache_control, "no-cache");
                res.prepare_payload();

                return send(std::move(res), start_response_time);
            } catch (...) {

                res = ConstructJsonResponse(http::status::unauthorized, req.version());
                res.set(http::field::cache_control, "no-cache");
                res.prepare_payload();

                return send(std::move(res), start_response_time);
            }
        }));

// *    POST, PUT, OPTIONS, DELETE, PATCH
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/players"}, http::verb::post, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "GET, HEAD")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/players"}, http::verb::put, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "GET, HEAD")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/players"}, http::verb::delete_, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "GET, HEAD")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/players"}, http::verb::options, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "GET, HEAD")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/players"}, http::verb::patch, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "GET, HEAD")};
            send(std::move(res), start_response_time);
        }));

// *    Mapping:
// *    /api/v1/game/state
// *    Methods: GET, HEAD
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/state"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {

            HttpResponse res;
            
            try {
                std::string token{req.at(http::field::authorization).data(), req.at(http::field::authorization).size()};

                token.erase(0, 7);
                token.erase(32, 3);

                if (token.size() != 32) throw "Invalid token";

                app::Player * player = app::PlayersManager::Instance().GetPlayerByToken(token);

                if (player == nullptr) {
                    json::value val = {{"code", "unknownToken"}, {"message", "Player token has not been found"}};

                    res = ConstructJsonResponse(http::status::unauthorized, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = json::serialize(val);
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }

                res = ConstructJsonResponse(http::status::ok, req.version());
                res.set(http::field::cache_control, "no-cache");
                res.body() = json_builder::GetPlayersInfo_s(player->GetSession());
                res.prepare_payload();

                return send(std::move(res), start_response_time);
            } catch (...) {

                json::value val = {{"code", "invalidToken"}, {"message", "Authorization header is missing"}};

                res = ConstructJsonResponse(http::status::unauthorized, req.version());
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(val);
                res.prepare_payload();

                return send(std::move(res), start_response_time);
            }
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/state"}, http::verb::head, [self = this, start_response_time, &send] (const HttpRequest & req) {

            HttpResponse res;
            
            try {
                std::string token{req.at(http::field::authorization).data(), req.at(http::field::authorization).size()};

                token.erase(0, 7);
                token.erase(32, 3);

                if (token.size() != 32) throw "Invalid token";

                app::Player * player = app::PlayersManager::Instance().GetPlayerByToken(token);

                if (player == nullptr) {
                    
                    res = ConstructJsonResponse(http::status::unauthorized, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }

                res = ConstructJsonResponse(http::status::ok, req.version());
                res.set(http::field::cache_control, "no-cache");
                res.prepare_payload();

                return send(std::move(res), start_response_time);
            } catch (...) {

                res = ConstructJsonResponse(http::status::unauthorized, req.version());
                res.set(http::field::cache_control, "no-cache");
                res.prepare_payload();

                return send(std::move(res), start_response_time);
            }
        }));

// *    OPTIONS, POST, PUT, PATCH, DELETE
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/state"}, http::verb::post, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "GET, HEAD")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/state"}, http::verb::put, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "GET, HEAD")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/state"}, http::verb::delete_, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "GET, HEAD")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/state"}, http::verb::options, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "GET, HEAD")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/state"}, http::verb::patch, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "GET, HEAD")};
            send(std::move(res), start_response_time);
        }));

// *    Mapping:
// *    /api/v1/game/player/action
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/player/action"}, http::verb::post, [self = this, start_response_time, &send] (const HttpRequest & req) {

            net::dispatch(self->strand_, [&self, start_response_time, &send, &req] {
                HttpResponse res;
                
                try {
                    if (req.at(http::field::content_type) != http_content_type::JSON) {
                        throw "Content-Type is invalid";
                    }
                } catch (...) {
                    res = ConstructJsonResponse(http::status::bad_request, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = json_builder::GetParseJsonError_s("Content-Type is invalid");
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }
                
                try {
                    std::string token{req.at(http::field::authorization).data(), req.at(http::field::authorization).size()};

                    token.erase(0, 7);
                    token.erase(32, 3);

                    if (token.size() != 32) throw "Invalid token";

                    app::Player * player = app::PlayersManager::Instance().GetPlayerByToken(token);

                    if (player == nullptr) {
                        json::value val = {{"code", "unknownToken"}, {"message", "Player token has not been found"}};

                        res = ConstructJsonResponse(http::status::unauthorized, req.version());
                        res.set(http::field::cache_control, "no-cache");
                        res.body() = json::serialize(val);
                        res.prepare_payload();

                        return send(std::move(res), start_response_time);
                    }

                    json::value val = json::parse(req.body());

                    if (!val.as_object().contains("move")) {

                        res = ConstructJsonResponse(http::status::bad_request, req.version());
                        res.set(http::field::cache_control, "no-cache");
                        res.body() = json_builder::GetInvalidArgument_s("Incorrect Json: no field \"move\"");
                        res.prepare_payload();

                        return send(std::move(res), start_response_time);
                    }

                    model::Direction dir;

                    if (val.at("move").as_string() == "L") {

                        dir = model::Direction::WEST;
                    } else if (val.at("move").as_string() == "R") {
                        
                        dir = model::Direction::EAST;
                    } else if (val.at("move").as_string() == "U") {

                        dir = model::Direction::NORTH;
                    } else if (val.at("move").as_string() == "D") {

                        dir = model::Direction::SOUTH;
                    } else if (val.at("move").as_string() == "") {

                        dir = model::Direction::ZERO;
                    } else {

                        res = ConstructJsonResponse(http::status::bad_request, req.version());
                        res.set(http::field::cache_control, "no-cache");
                        res.body() = json_builder::GetInvalidArgument_s("Incorrect Json: invalid value in field \"move\"");
                        res.prepare_payload();

                        return send(std::move(res), start_response_time);
                    }

                    model::Dog * dog = player->GetSession()->GetDogById(player->GetPlayerId());
                    if (dir == model::Direction::NORTH) {

                        dog->SetSpeed(model::Vector2{0, -player->GetSession()->GetMap()->GetDogSpeed()});
                    } else if (dir == model::Direction::EAST) {

                        dog->SetSpeed(model::Vector2{player->GetSession()->GetMap()->GetDogSpeed(), 0});
                    } else if (dir == model::Direction::SOUTH) {

                        dog->SetSpeed(model::Vector2{0, player->GetSession()->GetMap()->GetDogSpeed()});
                    } else if (dir == model::Direction::WEST) {
                        
                        dog->SetSpeed(model::Vector2{-player->GetSession()->GetMap()->GetDogSpeed(), 0});
                    } else {

                        dog->SetSpeed(model::Vector2{0, 0});
                    }

                    if (dir != model::Direction::ZERO) {
                        dog->SetDirection(dir);
                    }

                    res = ConstructJsonResponse(http::status::ok, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = json::serialize(json::value{});
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                } catch (...) {

                    json::value val = {{"code", "invalidToken"}, {"message", "Authorization header is missing"}};

                    res = ConstructJsonResponse(http::status::unauthorized, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = json::serialize(val);
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }
            });
        }));

// *    GET, HEAD, PUT, DELETE, OPTIONS, PATCH
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/player/action"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/player/action"}, http::verb::head, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/player/action"}, http::verb::put, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/player/action"}, http::verb::options, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/player/action"}, http::verb::patch, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/player/action"}, http::verb::delete_, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

// *    Mapping:
// *    /api/v1/game/tick
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/tick"}, http::verb::post, [self = this, start_response_time, &send] (const HttpRequest & req) {

            if (!self->game_.IsTimerStopped()) {
                HttpResponse res;

                res = ConstructJsonResponse(http::status::bad_request, req.version());
                res.set(http::field::cache_control, "no-cache");
                res.body() = json_builder::GetBadRequest_s();
                res.prepare_payload();

                return send(std::move(res), start_response_time);

            }

            net::dispatch(self->strand_, [self, start_response_time, &send, &req] {
                HttpResponse res;

                try {
                    if (req.at(http::field::content_type) != http_content_type::JSON) {
                        throw "Content-Type is invalid";
                    }
                } catch (...) {
                    res = ConstructJsonResponse(http::status::bad_request, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = json_builder::GetParseJsonError_s("Content-Type is invalid");
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }
                
                sys::error_code ec;

                json::value val = json::parse(req.body(), ec);

                if (ec) {

                    res = ConstructJsonResponse(http::status::bad_request, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = json_builder::GetInvalidArgument_s("Json parse error");
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }

                if (!val.as_object().contains("timeDelta")) {

                    res = ConstructJsonResponse(http::status::bad_request, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = json_builder::GetInvalidArgument_s("No field \"timeDelta\"");
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }

                try {
                    int time_delta = val.at("timeDelta").as_int64();

                    self->game_.Tick(time_delta, [&self, &time_delta] {
                        for (model::GameSession & session : self->game_.GetSessions()) {
                            session.AddItems(self->generator_.Generate(std::chrono::milliseconds(time_delta), session.GetItems().size(), session.GetDogs().size()));
                        }
                    });

                    res = ConstructJsonResponse(http::status::ok, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = "{}";
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                } catch (...) {

                    res = ConstructJsonResponse(http::status::bad_request, req.version());
                    res.set(http::field::cache_control, "no-cache");
                    res.body() = json_builder::GetInvalidArgument_s("Invalid type of field \"timeDelta\"");
                    res.prepare_payload();

                    return send(std::move(res), start_response_time);
                }
            });

        }));

// *    GET, HEAD, PUT, DELETE, OPTIONS, PATCH
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/tick"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/tick"}, http::verb::head, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/tick"}, http::verb::put, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/tick"}, http::verb::delete_, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/tick"}, http::verb::options, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/tick"}, http::verb::patch, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse res{ConstructMethodNotAllowedResponse(req, "POST")};
            send(std::move(res), start_response_time);
        }));

// *    Mapping:
// *    /api/...
        endpoints.emplace_back(endpoint::Endpoint({"/api/"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {

            HttpResponse res = ConstructJsonResponse(http::status::bad_request, req.version());
            res.body() = json_builder::GetBadRequest_s();

            send(std::move(res), start_response_time);
        }, true));

// *    Mapping local file access:
// *    {root}/...
        endpoints.emplace_back(endpoint::Endpoint({std::string(req.target().data(), req.target().size()).c_str()}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {

            sys::error_code ec;

            HttpFileResponse res = ConstructFileResponse(req, self->root_, ec);
            
            if (ec) {

                HttpResponse not_found_res(http::status::not_found, req.version());

                not_found_res.set(http::field::content_type, http_content_type::TEXT);
                not_found_res.body() = json_builder::GetPageNotFound_s();
                not_found_res.keep_alive(false);

                send(std::move(not_found_res), start_response_time);

                return;
            }

            send(std::move(res), start_response_time); 
        }));

// *    Looking for suitable path
        for (const endpoint::Endpoint & endpoint : endpoints) {

            if (endpoint.IsMatch({req.target()}, req.method())) {

                endpoint.Invoke(req);
                return;
            }
        }

        send(std::move(response), start_response_time);
    }

private:
    model::Game& game_;
    loot_gen::LootGenerator& generator_;
    std::vector<extra_data::MapExtraData> maps_extra_data_;
    Strand strand_;
    fs::path root_;
};

}  // namespace http_handler
