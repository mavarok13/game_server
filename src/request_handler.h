#ifndef BOOST_BEAST_USE_STD_STRING_VIEW
#define BOOST_BEAST_USE_STD_STRING_VIEW
#endif

#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <boost/algorithm/string.hpp>

#include <filesystem>
#include <iostream>
#include <syncstream>
#include <iostream>
#include <chrono>
#include <string>

#include "model.h"
#include "app.h"
#include "update_items.h"
#include "loot_generator.h"
#include "http_server.h"
#include "json_builder.h"
#include "json_loader.h"
#include "endpoint.h"
#include "http_utils.h"
#include "http_content_type.h"
#include "extra_data.h"

namespace http_handler {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
namespace json = boost::json;

using namespace std::literals;

using HttpResponse = http::response<http::string_body>;
using HttpFileResponse = http::response<http::file_body>;

namespace fs = std::filesystem;

HttpResponse ConstructJsonResponse (http::status status, unsigned version, bool keep_alive = false);
HttpFileResponse ConstructFileResponse (fs::path root, fs::path request_target, unsigned version, bool keep_alive, sys::error_code & ec);
HttpResponse ConstructOkResponse(unsigned version, bool keep_alive);
HttpResponse ConstructOkResponse(std::string_view body, unsigned version, bool keep_alive);
HttpResponse ConstructMethodNotAllowedResponse (std::string_view methods, unsigned version, bool keep_alive);
HttpResponse ConstructInvalidArgumentResponse(std::string_view msg, unsigned version, bool keep_alive);
HttpResponse ConstructMapNotFoundResponse(unsigned version, bool keep_alive);
HttpResponse ConstructMapNotFoundResponse_head(unsigned version, bool keep_alive);
HttpResponse ConstructBadRequestResponse(std::string_view body, unsigned version, bool keep_alive);
HttpResponse ConstructBadRequestResponse(unsigned version, bool keep_alive);
HttpResponse ConstructUnauthorizedResponse(std::string_view body, unsigned version, bool keep_alive);
HttpResponse ConstructUnauthorizedResponse(unsigned version, bool keep_alive);

class RequestHandler {
public:
    using Strand = net::strand<net::io_context::executor_type>;

    RequestHandler(std::shared_ptr<model::Game> game, app::Application & application, loot_gen::LootGenerator& generator, std::shared_ptr<app::IUnitOfWorkFactory> unit_of_work_factory, std::vector<extra_data::MapExtraData> maps_extra_data, std::shared_ptr<Strand> strand, const fs::path & root)
        : game_{game}, app_{application}, generator_{generator}, unit_of_work_factory_{unit_of_work_factory}, maps_extra_data_{maps_extra_data}, strand_{strand}, root_{root} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;    

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, Allocator> && request, Send&& send) {
        std::chrono::time_point<std::chrono::system_clock> start_response_time = std::chrono::system_clock::now();

//  *   REQUEST TARGET DECODING AND MAKE ABSOLUTE PATH
        std::string request_target_str{request.target().data(), request.target().size()};
        request_target_str = http_utils::UrlDecode(request_target_str);
        request.target(request_target_str);

        fs::path request_target{request_target_str};
        request_target = fs::weakly_canonical(request_target);

//  *   CONSTRUCT DEFAULT RESPONSE
        HttpResponse response = ConstructJsonResponse(http::status::not_found, request.version());
        response.body() = json_builder::GetPageNotFound_s();

        std::vector<endpoint::Endpoint<Body, Allocator>> endpoints;

//  *   Mapping:
//  *   /api/v1/maps
//  *   Methods: GET, HEAD
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/maps"}, http::verb::get, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructOkResponse(json_builder::GetMaps_s(self->game_->GetMaps()), req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/maps"}, http::verb::head, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructOkResponse(req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }));

//  *   Methods: POST, PUT, DELETE, PATCH, OPTIONS
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/maps"}, http::verb::post, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructMethodNotAllowedResponse("GET, HEAD", req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/maps"}, http::verb::delete_, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructMethodNotAllowedResponse("GET, HEAD", req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/maps"}, http::verb::options, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructMethodNotAllowedResponse("GET, HEAD", req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/maps"}, http::verb::put, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructMethodNotAllowedResponse("GET, HEAD", req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/maps"}, http::verb::patch, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructMethodNotAllowedResponse("GET, HEAD", req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }));

// *    Mapping:
// *    /api/v1/maps/{map_id}/
// *    Methods: GET, HEAD
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/maps"}, http::verb::get, [self = this, &send, request_target, start_response_time] (http::request<Body, Allocator> && req) {
            std::string map_id{request_target.filename().string()};

            const model::Map * map = self->game_->FindMap(model::Map::Id{map_id});

            if (map) {
                extra_data::MapExtraData * map_extra_data;
                for (extra_data::MapExtraData & ed : self->maps_extra_data_) {
                    if (ed.GetMapId() == *(map->GetId())) {
                        map_extra_data = &ed;
                        break;
                    }
                }

                HttpResponse response = ConstructOkResponse(json_builder::GetMapWithExtraData_s(*map, *map_extra_data), req.version(), req.keep_alive());
                send(std::move(response), start_response_time);
            }
            else { 
                HttpResponse response = ConstructMapNotFoundResponse(req.version(), req.keep_alive());
                send(std::move(response), start_response_time);
            }
        }, 1));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/maps"}, http::verb::head, [self = this, request_target, &send, start_response_time] (http::request<Body, Allocator> && req) {
            std::string map_id{request_target.filename().string()};

            const model::Map * map = self->game_->FindMap(model::Map::Id{map_id});

            if (map) {
                HttpResponse response = ConstructOkResponse(req.version(), req.keep_alive());
                send(std::move(response), start_response_time);
            }
            else { 
                HttpResponse response = ConstructMapNotFoundResponse(req.version(), req.keep_alive());
                send(std::move(response), start_response_time);
            }
        }, 1));


// *    Mapping:
// *    /api/v1/maps/{map_id}
// *    Methods: POST, PUT, DELETE, OPTIONS
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/maps"}, http::verb::post, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructMethodNotAllowedResponse("GET, HEAD", req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }, 1));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/maps"}, http::verb::delete_, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructMethodNotAllowedResponse("GET, HEAD", req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }, 1));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/maps"}, http::verb::options, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructMethodNotAllowedResponse("GET, HEAD", req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }, 1));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/maps"}, http::verb::put, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructMethodNotAllowedResponse("GET, HEAD", req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }, 1));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/maps"}, http::verb::patch, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructMethodNotAllowedResponse("GET, HEAD", req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }, 1));

// *    Join, METHOD: POST
// *    Mapping:
// *    /api/v1/game/join
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/join"}, http::verb::post, [self = this, &send, start_response_time, strand_ptr = std::shared_ptr<Strand>(strand_)] (http::request<Body, Allocator> && req) {
            sys::error_code ec;
            std::string connection_data{req.body().data(), req.body().size()};
            json::value request_body_val = json::parse(connection_data, ec);

            try {
                if (req.at(http::field::content_type) != http_content_type::JSON) {
                    throw "Content-Type is invalid";
                }
            } catch (...) {
                HttpResponse response = ConstructBadRequestResponse("Content-Type is invalid"sv, req.version(), req.keep_alive());
                return send(std::move(response), start_response_time);
            }

            if (ec) {
                std::string error_data{ec.message().data(), ec.message().size()};
                HttpResponse response = ConstructInvalidArgumentResponse(error_data, req.version(), req.keep_alive());
                return send(response, start_response_time);
            }

            if (!request_body_val.as_object().contains( json_fields::AUTENTICATE_PLAYER_NAME )) {
                HttpResponse response = ConstructInvalidArgumentResponse("JSON parse error: Couldn't find \"userName\" field"sv, req.version(), req.keep_alive());
                return send(std::move(response), start_response_time);
            }

            if (!request_body_val.as_object().contains( json_fields::AUTENTICATE_MAP_ID )) {
                HttpResponse response = ConstructInvalidArgumentResponse("JSON parse error: Couldn't find \"mapId\" field"sv, req.version(), req.keep_alive());
                return send(std::move(response), start_response_time);
            }

            std::string player_name{request_body_val.at( json_fields::AUTENTICATE_PLAYER_NAME ).as_string()};
            std::string map_id{request_body_val.at( json_fields::AUTENTICATE_MAP_ID ).as_string()};

            if (player_name.size() == 0) {
                HttpResponse response = ConstructInvalidArgumentResponse("Invalid value: player name can't be empty"sv, req.version(), req.keep_alive());
                return send(std::move(response), start_response_time);
            }

            const model::Map * map = self->game_->FindMap(model::Map::Id{map_id});
            if (map == nullptr) {
                HttpResponse response = ConstructMapNotFoundResponse(req.version(), req.keep_alive());
                return send(std::move(response), start_response_time);
            }

            net::dispatch(*strand_ptr, [self, send, start_response_time, req, strand_ptr, player_name, map] {
                model::GameSession * session = self->game_->NewSession(const_cast<model::Map *>(map));

                app::Player player = app::PlayersManager::Instance().AddNewPlayer(player_name, session);

                HttpResponse response = ConstructOkResponse(json_builder::GetTokenAndPlayerId_s(player.GetToken(), player.GetPlayerId()), req.version(), req.keep_alive());

                return send(std::move(response), start_response_time);
            });
        }));

// *    Join, METHOD: GET, PUT, DELETE, HEAD
// *    /api/v1/game/join
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/join"}, http::verb::get, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/join"}, http::verb::put, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/join"}, http::verb::delete_, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/join"}, http::verb::head, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/join"}, http::verb::options, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/join"}, http::verb::patch, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));


// *    Mapping:
// *    /api/v1/game/players
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/players"}, http::verb::get, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            try {
                std::string token{http_utils::FormatToken(req.at(http::field::authorization))};

                app::Player * player = app::PlayersManager::Instance().GetPlayerByToken(token);

                if (player == nullptr) {
                    HttpResponse response{ConstructUnauthorizedResponse(json_builder::GetUnknownToken_s(), req.version(), req.keep_alive())};
                    return send(std::move(response), start_response_time);
                }

                HttpResponse response{ConstructOkResponse(json_builder::GetPlayers_s(self->game_->GetSessionById(player->GetSessionId())), req.version(), req.keep_alive())};
                return send(std::move(response), start_response_time);
            } catch (std::exception & ex) {
                HttpResponse response{ConstructUnauthorizedResponse(json_builder::GetInvalidToken_s(), req.version(), req.keep_alive())};
                return send(std::move(response), start_response_time);
            }
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/players"}, http::verb::head, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            try {
                std::string token{http_utils::FormatToken(req.at(http::field::authorization))};

                app::Player * player = app::PlayersManager::Instance().GetPlayerByToken(token);

                if (player == nullptr) {
                    HttpResponse response{ConstructUnauthorizedResponse(req.version(), req.keep_alive())};
                    return send(std::move(response), start_response_time);
                }

                HttpResponse response{ConstructOkResponse(req.version(), req.keep_alive())};
                return send(std::move(response), start_response_time);
            } catch (std::exception & ex) {
                HttpResponse response{ConstructUnauthorizedResponse(req.version(), req.keep_alive())};
                return send(std::move(response), start_response_time);
            }
        }));

// *    POST, PUT, OPTIONS, DELETE, PATCH
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/players"}, http::verb::post, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("GET, HEAD"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/players"}, http::verb::put, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("GET, HEAD"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/players"}, http::verb::delete_, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("GET, HEAD"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/players"}, http::verb::options, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("GET, HEAD"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/players"}, http::verb::patch, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("GET, HEAD"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

// *    Mapping:
// *    /api/v1/game/state
// *    Methods: GET, HEAD
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/state"}, http::verb::get, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            try {
                std::string token{http_utils::FormatToken(req.at(http::field::authorization))};

                app::Player * player = app::PlayersManager::Instance().GetPlayerByToken(token);

                if (player == nullptr) {
                    HttpResponse response{ConstructUnauthorizedResponse(json_builder::GetUnknownToken_s(), req.version(), req.keep_alive())};
                    return send(std::move(response), start_response_time);
                }

                HttpResponse response{ConstructOkResponse(json_builder::GetPlayersInfo_s(self->game_->GetSessionById(player->GetSessionId())), req.version(), req.keep_alive())};
                return send(std::move(response), start_response_time);
            } catch (std::exception & ex) {
                HttpResponse response{ConstructUnauthorizedResponse(json_builder::GetInvalidToken_s(), req.version(), req.keep_alive())};
                return send(std::move(response), start_response_time);
            }
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/state"}, http::verb::head, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            try {
                std::string token{http_utils::FormatToken(req.at(http::field::authorization))};

                app::Player * player = app::PlayersManager::Instance().GetPlayerByToken(token);

                if (player == nullptr) {
                    HttpResponse response{ConstructUnauthorizedResponse(json_builder::GetUnknownToken_s(), req.version(), req.keep_alive())};
                    return send(std::move(response), start_response_time);
                }

                HttpResponse response{ConstructOkResponse(req.version(), req.keep_alive())};
                return send(std::move(response), start_response_time);
            } catch (std::exception & ex) {
                HttpResponse response{ConstructUnauthorizedResponse(json_builder::GetInvalidToken_s(), req.version(), req.keep_alive())};
                return send(std::move(response), start_response_time);
            }
        }));

// *    OPTIONS, POST, PUT, PATCH, DELETE
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/state"}, http::verb::post, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructMethodNotAllowedResponse("GET, HEAD", req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/state"}, http::verb::put, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructMethodNotAllowedResponse("GET, HEAD", req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/state"}, http::verb::delete_, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructMethodNotAllowedResponse("GET, HEAD", req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/state"}, http::verb::options, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructMethodNotAllowedResponse("GET, HEAD", req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/state"}, http::verb::patch, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructMethodNotAllowedResponse("GET, HEAD", req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }));

// *    Mapping:
// *    /api/v1/game/player/action
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/player/action"}, http::verb::post, [self = this, send, start_response_time, strand_ptr = std::shared_ptr<Strand>(strand_)] (http::request<Body, Allocator> && req) {
                try {
                    if (req.at(http::field::content_type) != http_content_type::JSON) {
                        throw std::logic_error("Content-Type is invalid");
                    }
                } catch (std::exception & ex) {
                    HttpResponse response = ConstructBadRequestResponse(ex.what(), req.version(), req.keep_alive());
                    return send(std::move(response), start_response_time);
                }
                
                app::Player * player = 0;
                std::string token;
                try {
                    token = std::string{req.at(http::field::authorization).data(), req.at(http::field::authorization).size()};
                    token = {http_utils::FormatToken(token)};

                    player = app::PlayersManager::Instance().GetPlayerByToken(token);

                    if (player == nullptr) {
                        HttpResponse response = ConstructUnauthorizedResponse("Player token has not been found"sv, req.version(), req.keep_alive());
                        return send(std::move(response), start_response_time);
                    }
                } catch (std::exception & ex) {
                    HttpResponse response = ConstructUnauthorizedResponse(ex.what(), req.version(), req.keep_alive());
                    return send(std::move(response), start_response_time);
                }

                json::value val = json::parse(req.body());

                if (!val.as_object().contains("move")) {
                    HttpResponse response = ConstructBadRequestResponse("Incorrect Json: can't find field \"move\""sv, req.version(), req.keep_alive());
                    return send(std::move(response), start_response_time);
                }

                model::Direction dir = model::Direction::ZERO;

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
                    HttpResponse response = ConstructBadRequestResponse("Incorrect Json: invalid value in field \"move\""sv, req.version(), req.keep_alive());
                    return send(std::move(response), start_response_time);
                }

                net::dispatch(*strand_ptr, [self, /*game_ptr, */start_response_time, dir, /*player,*/ strand_ptr, token] {
                    app::Player * player = app::PlayersManager::Instance().GetPlayerByToken(token);
                    model::GameSession * session = self->game_->GetSessionById(player->GetSessionId());
                    model::Dog * dog = session->GetDogById(player->GetPlayerId());

                    if (dir == model::Direction::NORTH) {
                        dog->SetSpeed(model::Vector2{0, -session->GetMap()->GetDogSpeed()});
                    } else if (dir == model::Direction::EAST) {
                        dog->SetSpeed(model::Vector2{session->GetMap()->GetDogSpeed(), 0});
                    } else if (dir == model::Direction::SOUTH) {
                        dog->SetSpeed(model::Vector2{0, session->GetMap()->GetDogSpeed()});
                    } else if (dir == model::Direction::WEST) {
                        dog->SetSpeed(model::Vector2{-session->GetMap()->GetDogSpeed(), 0});
                    } else {
                        dog->SetSpeed(model::Vector2{0, 0});
                    }

                    if (dir != model::Direction::ZERO) {
                        dog->SetDirection(dir);
                    }
                });

                HttpResponse response = ConstructOkResponse("{}"sv, req.version(), req.keep_alive());
                return send(std::move(response), start_response_time);
        }));

// *    GET, HEAD, PUT, DELETE, OPTIONS, PATCH
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/player/action"}, http::verb::get, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/player/action"}, http::verb::head, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/player/action"}, http::verb::put, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/player/action"}, http::verb::options, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/player/action"}, http::verb::patch, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/player/action"}, http::verb::delete_, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

// *    Mapping:
// *    /api/v1/game/tick
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/tick"}, http::verb::post, [self = this, &send, start_response_time, strand_ptr = std::shared_ptr<Strand>(strand_)] (http::request<Body, Allocator> && req) {
            if (!self->game_->IsTimerStopped()) {
                HttpResponse response{ConstructBadRequestResponse(json_builder::GetBadRequest_s(), req.version(), req.keep_alive())};
                return send(std::move(response), start_response_time);
            }

            try {
                if (req.at(http::field::content_type) != http_content_type::JSON) {
                    throw "Content-Type is invalid";
                }
            } catch (std::exception & ex) {
                HttpResponse response{ConstructBadRequestResponse(ex.what(), req.version(), req.keep_alive())};
                return send(std::move(response), start_response_time);
            }
            
            sys::error_code ec;

            json::value val = json::parse(req.body(), ec);

            if (ec) {
                HttpResponse response{ConstructBadRequestResponse("Json parse error"sv, req.version(), req.keep_alive())};
                return send(std::move(response), start_response_time);
            }

            if (!val.as_object().contains("timeDelta")) {
                HttpResponse response{ConstructBadRequestResponse("No field \"timeDelta\""sv, req.version(), req.keep_alive())};
                return send(std::move(response), start_response_time);
            }

            int time_delta = 0;
            try {
                time_delta = val.at("timeDelta").as_int64();
            } catch (std::exception & ex) {
                HttpResponse response{ConstructBadRequestResponse("Invalid type of field \"timeDelta\""sv, req.version(), req.keep_alive())};
                return send(std::move(response), start_response_time);
            }

            net::dispatch(*strand_ptr, [self, time_delta, strand_ptr] {                
                self->app_.Tick(std::chrono::milliseconds(time_delta));
                self->game_->Tick(time_delta, [self, time_delta] {
                    for (model::GameSession & session : self->game_->GetSessions()) {
                        session.AddItems(self->generator_.Generate(std::chrono::milliseconds(time_delta), session.GetItems().size(), session.GetDogs().size()));
                        collision_detector::UpdateSessionItems(session, time_delta);
                    }
                });
            });

            HttpResponse response{ConstructOkResponse("{}", req.version(), req.keep_alive())};
            response.set(http::field::content_type, "application/json");
            response.keep_alive(req.keep_alive());
            response.prepare_payload();

            return send(std::move(response), start_response_time);
        }));

// *    GET, HEAD, PUT, DELETE, OPTIONS, PATCH
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/tick"}, http::verb::get, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/tick"}, http::verb::head, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/tick"}, http::verb::put, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/tick"}, http::verb::delete_, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/tick"}, http::verb::options, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/tick"}, http::verb::patch, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("POST"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

// *    Mapping: GET, head
// *    /api/v1/game/records
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/records"}, http::verb::get, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            unsigned int offset = 0;
            unsigned int limit = 100;

            std::string url{req.target().data(), req.target().size()};

            std::vector<std::string> url_components;

            if (boost::split(url_components, url, boost::is_any_of("?")); url_components.size() == 2) {
                std::string params{url_components.at(1)};

                std::vector<std::string> params_splitted;
                boost::split(params_splitted, params, boost::is_any_of("&"));

                for (std::string & param : params_splitted) {
                    std::vector<std::string> param_key_val;

                    boost::split(param_key_val, param, boost::is_any_of("="));

                    if (param_key_val.at(0) == "start") {
                        offset = std::stoi(param_key_val.at(1));
                    } else if (param_key_val.at(0) == "maxItems") {
                        limit = std::stoi(param_key_val.at(1));
                    }
                }
            }

            if (limit > 100) {
                HttpResponse response{ConstructBadRequestResponse("Start top results must be less than 100"sv, req.version(), req.keep_alive())};
                send(std::move(response), start_response_time);
            }

            auto unit_of_work = self->unit_of_work_factory_->NewUnitOfWork();
            std::vector<app::GameResult> game_results = unit_of_work->Results()->GetResults(limit, offset);
            unit_of_work->Commit();

            HttpResponse response{ConstructOkResponse(json_builder::GetTopGameResults_s(game_results), req.version(), req.keep_alive())};
            response.set(http::field::content_type, "application/json");
            response.prepare_payload();

            send(std::move(response), start_response_time);
        }, true));
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/records"}, http::verb::head, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            unsigned int offset = 0;
            unsigned int limit = 100;

            std::string url{req.target().data(), req.target().size()};

            std::vector<std::string> url_components;

            if (boost::split(url_components, url, boost::is_any_of("?")); url_components.size() == 2) {
                std::string params{url_components.at(1)};

                std::vector<std::string> params_splitted;
                boost::split(params_splitted, params, boost::is_any_of("&"));

                for (std::string & param : params_splitted) {
                    std::vector<std::string> param_key_val;

                    boost::split(param_key_val, param, boost::is_any_of("="));

                    if (param_key_val.at(0) == "start") {
                        offset = std::stoi(param_key_val.at(1));
                    } else if (param_key_val.at(0) == "maxItems") {
                        limit = std::stoi(param_key_val.at(1));
                    }
                }
            }

            if (limit > 100) {
                HttpResponse response{ConstructBadRequestResponse("Start top results must be less than 100"sv, req.version(), req.keep_alive())};
                send(std::move(response), start_response_time);
            }

            HttpResponse response{ConstructOkResponse(req.version(), req.keep_alive())};
            response.set(http::field::content_type, "application/json");
            response.prepare_payload();

            send(std::move(response), start_response_time);
        }, true));
// *    POST, PUT, DELETE, PATCH, OPTIONS
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/records"}, http::verb::post, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("GET, HEAD"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/records"}, http::verb::put, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("GET, HEAD"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/records"}, http::verb::delete_, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("GET, HEAD"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/records"}, http::verb::patch, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("GET, HEAD"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/v1/game/records"}, http::verb::options, [&send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response{ConstructMethodNotAllowedResponse("GET, HEAD"sv, req.version(), req.keep_alive())};
            send(std::move(response), start_response_time);
        }));

// *    Mapping:
// *    /api/...
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>({"/api/"}, http::verb::get, [self = this, &send, start_response_time] (http::request<Body, Allocator> && req) {
            HttpResponse response = ConstructBadRequestResponse(json_builder::GetBadRequest_s(), req.version(), req.keep_alive());
            send(std::move(response), start_response_time);
        }, true));

// *    Mapping local file access:
// *    {root}/...
        endpoints.emplace_back(endpoint::Endpoint<Body, Allocator>(request_target, http::verb::get, [this, &send, start_response_time, request_target] (http::request<Body, Allocator> && req) {
            sys::error_code ec;

            HttpFileResponse response = ConstructFileResponse(root_, request_target, req.version(), req.keep_alive(), ec);
            
            if (ec) {
                HttpResponse not_found_res(http::status::not_found, req.version());

                not_found_res.set(http::field::content_type, http_content_type::TEXT);
                not_found_res.body() = json_builder::GetPageNotFound_s();
                not_found_res.keep_alive(false);

                send(std::move(not_found_res), start_response_time);

                return;
            }

            send(std::move(response), start_response_time); 
        }));

// *    Looking for suitable path
        for (const endpoint::Endpoint<Body, Allocator> & endpoint : endpoints) {
            if (endpoint.IsMatch(request_target, request.method())) {
                endpoint.Invoke(std::move(request));
                return;
            }
        }

        send(std::move(response), start_response_time);
    }

private:
    std::shared_ptr<model::Game> game_;
    app::Application & app_;
    loot_gen::LootGenerator & generator_;
    std::shared_ptr<app::IUnitOfWorkFactory> unit_of_work_factory_;
    std::vector<extra_data::MapExtraData> maps_extra_data_;
    std::shared_ptr<Strand> strand_;
    fs::path root_;
};

}  // namespace http_handler
