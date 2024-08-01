#ifndef BOOST_BEAST_USE_STD_STRING_VIEW
#define BOOST_BEAST_USE_STD_STRING_VIEW
#endif

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
#include "update_items.h"
#include "loot_generator.h"
#include "http_server.h"
#include "json_builder.h"
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

using HttpRequest = http::request<http::string_body>;

using HttpResponse = http::response<http::string_body>;
using HttpFileResponse = http::response<http::file_body>;

namespace fs = std::filesystem;

HttpResponse ConstructJsonResponse (http::status status, unsigned version, bool keep_alive = false);
HttpFileResponse ConstructFileResponse (HttpRequest request, const fs::path & root, sys::error_code & ec);
HttpResponse ConstructOkResponse(HttpRequest request);
HttpResponse ConstructOkResponse(HttpRequest request, std::string_view body);
HttpResponse ConstructMethodNotAllowedResponse (HttpRequest request, std::string_view methods);
HttpResponse ConstructInvalidArgumentResponse(HttpRequest request, std::string_view msg);
HttpResponse ConstructMapNotFoundResponse(HttpRequest request);
HttpResponse ConstructMapNotFoundResponse_head(HttpRequest request);
HttpResponse ConstructBadRequestResponse(HttpRequest request, std::string_view body);
HttpResponse ConstructBadRequestResponse(HttpRequest request);
HttpResponse ConstructUnauthorizedResponse(HttpRequest request, std::string_view body);
HttpResponse ConstructUnauthorizedResponse(HttpRequest request);

class RequestHandler {
public:
    using Strand = net::strand<net::io_context::executor_type>;

    RequestHandler(model::Game& game, app::Application & application, loot_gen::LootGenerator& generator, std::vector<extra_data::MapExtraData> maps_extra_data, Strand strand, const fs::path & root)
        : game_{game}, app_{application}, generator_{generator}, maps_extra_data_{maps_extra_data}, strand_{strand}, root_{root} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;    

    template <typename Send>
    void operator()(HttpRequest&& req, Send&& send) {
        std::chrono::time_point<std::chrono::system_clock> start_response_time = std::chrono::system_clock::now();

        std::string_view target_str_encoded{req.target()};
        req.target(http_utils::UrlDecode(target_str_encoded));

        HttpResponse response = ConstructJsonResponse(http::status::not_found, req.version());
        response.body() = json_builder::GetPageNotFound_s();

        std::vector<endpoint::Endpoint> endpoints;

//  *   Mapping:
//  *   /api/v1/maps
//  *   Methods: GET, HEAD
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructOkResponse(req, json_builder::GetMaps_s(self->game_.GetMaps()));
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::head, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructOkResponse(req);
            send(std::move(response), start_response_time);
        }));

//  *   Methods: POST, PUT, DELETE, PATCH, OPTIONS
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::post, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD");
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::delete_, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD");
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::options, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD");
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::put, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD");
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::patch, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD");
            send(std::move(response), start_response_time);
        }));

// *    Mapping:
// *    /api/v1/maps/{map_id}/
// *    Methods: GET, HEAD
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response;
            
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

                response = ConstructOkResponse(req, json_builder::GetMapWithExtraData_s(*map, *map_extra_data));
            }
            else { 
                response = ConstructMapNotFoundResponse(req);
            }

            send(std::move(response), start_response_time);
        }, 1));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::head, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response;
            
            fs::path target_path{fs::weakly_canonical(fs::path{req.target()}.filename())};

            std::string map_id{target_path.c_str()};

            const model::Map * map = self->game_.FindMap(model::Map::Id{map_id});

            if (map) {
                response = ConstructOkResponse(req);
            } else { 
                response = ConstructMapNotFoundResponse_head(req);
            }

            send(std::move(response), start_response_time);
        }, 1));


// *    Mapping:
// *    /api/v1/maps/{map_id}
// *    Methods: POST, PUT, DELETE, OPTIONS
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::post, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD");
            send(std::move(response), start_response_time);
        }, 1));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::delete_, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD");
            send(std::move(response), start_response_time);
        }, 1));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::options, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD");
            send(std::move(response), start_response_time);
        }, 1));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::put, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD");
            send(std::move(response), start_response_time);
        }, 1));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/maps"}, http::verb::patch, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD");
            send(std::move(response), start_response_time);
        }, 1));

// *    Join, METHOD: POST
// *    Mapping:
// *    /api/v1/game/join
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/join"}, http::verb::post, [self = this, start_response_time, &send] (const HttpRequest & req) {
            net::dispatch(self->strand_, [self, start_response_time, &send, &req] {
                sys::error_code ec;
                json::value request_body_val = json::parse(req.body().data(), ec);

                try {
                    if (req.at(http::field::content_type) != http_content_type::JSON) {
                        throw "Content-Type is invalid";
                    }
                } catch (...) {
                    HttpResponse response = ConstructBadRequestResponse(req, "Content-Type is invalid"sv);
                    return send(std::move(response), start_response_time);
                }

                if (ec) {
                    HttpResponse response = ConstructInvalidArgumentResponse(req, ec.category().name());
                    return send(std::move(response), start_response_time);
                }

                if (!request_body_val.as_object().contains("userName")) {
                    HttpResponse response = ConstructInvalidArgumentResponse(req, "JSON parse error: Couldn't find \"userName\" field"sv);
                    return send(std::move(response), start_response_time);
                }

                if (!request_body_val.as_object().contains("mapId")) {
                    HttpResponse response = ConstructInvalidArgumentResponse(req, "JSON parse error: Couldn't find \"mapId\" field"sv);
                    return send(std::move(response), start_response_time);
                }

                std::string player_name{request_body_val.at("userName").as_string()};
                std::string map_id{request_body_val.at("mapId").as_string()};

                if (player_name.size() == 0) {
                    HttpResponse response = ConstructInvalidArgumentResponse(req, "Invalid value: player name can't be empty"sv);
                    return send(std::move(response), start_response_time);
                }

                const model::Map * map = self->game_.FindMap(model::Map::Id{map_id});

                if (map == nullptr) {
                    HttpResponse response = ConstructMapNotFoundResponse(req);
                    return send(std::move(response), start_response_time);
                }

                model::GameSession * session = self->game_.NewSession(const_cast<model::Map *>(map));

                app::Player player = app::PlayersManager::Instance().AddNewPlayer(player_name, session);

                HttpResponse response = ConstructOkResponse(req, json_builder::GetTokenAndPlayerId_s(player.GetToken(), player.GetPlayerId()));

                return send(std::move(response), start_response_time);
            });
        }));

// *    Join, METHOD: GET, PUT, DELETE, HEAD
// *    /api/v1/game/join
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/join"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/join"}, http::verb::put, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/join"}, http::verb::delete_, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/join"}, http::verb::head, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/join"}, http::verb::options, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/join"}, http::verb::patch, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));


// *    Mapping:
// *    /api/v1/game/players
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/players"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {
            try {
                std::string token{http_utils::FormatToken(req.at(http::field::authorization))};

                app::Player * player = app::PlayersManager::Instance().GetPlayerByToken(token);

                if (player == nullptr) {
                    HttpResponse response = ConstructUnauthorizedResponse(req, json_builder::GetUnknownToken_s());
                    return send(std::move(response), start_response_time);
                }

                HttpResponse response = ConstructOkResponse(req, json_builder::GetPlayers_s(player->GetSession()));
                return send(std::move(response), start_response_time);
            } catch (std::exception & ex) {
                HttpResponse response = ConstructUnauthorizedResponse(req, json_builder::GetInvalidToken_s());
                return send(std::move(response), start_response_time);
            }
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/players"}, http::verb::head, [self = this, start_response_time, &send] (const HttpRequest & req) {
            try {
                std::string token{http_utils::FormatToken(req.at(http::field::authorization))};

                app::Player * player = app::PlayersManager::Instance().GetPlayerByToken(token);

                if (player == nullptr) {
                    HttpResponse response = ConstructUnauthorizedResponse(req);
                    return send(std::move(response), start_response_time);
                }

                HttpResponse response = ConstructOkResponse(req);
                return send(std::move(response), start_response_time);
            } catch (std::exception & ex) {
                HttpResponse response = ConstructUnauthorizedResponse(req);
                return send(std::move(response), start_response_time);
            }
        }));

// *    POST, PUT, OPTIONS, DELETE, PATCH
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/players"}, http::verb::post, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/players"}, http::verb::put, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/players"}, http::verb::delete_, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/players"}, http::verb::options, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/players"}, http::verb::patch, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD"sv);
            send(std::move(response), start_response_time);
        }));

// *    Mapping:
// *    /api/v1/game/state
// *    Methods: GET, HEAD
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/state"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {
            try {
                std::string token{http_utils::FormatToken(req.at(http::field::authorization))};

                app::Player * player = app::PlayersManager::Instance().GetPlayerByToken(token);

                if (player == nullptr) {
                    HttpResponse response = ConstructUnauthorizedResponse(req, json_builder::GetUnknownToken_s());
                    return send(std::move(response), start_response_time);
                }

                HttpResponse response = ConstructOkResponse(req, json_builder::GetPlayersInfo_s(player->GetSession()));
                return send(std::move(response), start_response_time);
            } catch (std::exception & ex) {
                HttpResponse response = ConstructUnauthorizedResponse(req, json_builder::GetInvalidToken_s());
                return send(std::move(response), start_response_time);
            }
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/state"}, http::verb::head, [self = this, start_response_time, &send] (const HttpRequest & req) {
            try {
                std::string token{http_utils::FormatToken(req.at(http::field::authorization))};

                app::Player * player = app::PlayersManager::Instance().GetPlayerByToken(token);

                if (player == nullptr) {
                    HttpResponse response = ConstructUnauthorizedResponse(req);
                    return send(std::move(response), start_response_time);
                }

                HttpResponse response = ConstructOkResponse(req);
                return send(std::move(response), start_response_time);
            } catch (std::exception & ex) {
                HttpResponse response = ConstructUnauthorizedResponse(req);
                return send(std::move(response), start_response_time);
            }
        }));

// *    OPTIONS, POST, PUT, PATCH, DELETE
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/state"}, http::verb::post, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD");
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/state"}, http::verb::put, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD");
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/state"}, http::verb::delete_, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD");
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/state"}, http::verb::options, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD");
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/state"}, http::verb::patch, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "GET, HEAD");
            send(std::move(response), start_response_time);
        }));

// *    Mapping:
// *    /api/v1/game/player/action
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/player/action"}, http::verb::post, [self = this, start_response_time, &send] (const HttpRequest & req) {
            net::dispatch(self->strand_, [&self, start_response_time, &send, &req] {
                try {
                    if (req.at(http::field::content_type) != http_content_type::JSON) {
                        throw "Content-Type is invalid";
                    }
                } catch (std::exception & ex) {
                    HttpResponse response = ConstructBadRequestResponse(req, ex.what());
                    return send(std::move(response), start_response_time);
                }
                
                try {
                    std::string token{http_utils::FormatToken(req.at(http::field::authorization))};

                    app::Player * player = app::PlayersManager::Instance().GetPlayerByToken(token);

                    if (player == nullptr) {
                        HttpResponse response = ConstructUnauthorizedResponse(req, "Player token has not been found"sv);
                        return send(std::move(response), start_response_time);
                    }

                    json::value val = json::parse(req.body());

                    if (!val.as_object().contains("move")) {
                        HttpResponse response = ConstructBadRequestResponse(req, "Incorrect Json: can't find field \"move\""sv);
                        return send(std::move(response), start_response_time);
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
                        HttpResponse response = ConstructBadRequestResponse(req, "Incorrect Json: invalid value in field \"move\""sv);
                        return send(std::move(response), start_response_time);
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

                    HttpResponse response = ConstructOkResponse(req);
                    return send(std::move(response), start_response_time);
                } catch (std::exception & ex) {
                    HttpResponse response = ConstructUnauthorizedResponse(req, ex.what());
                    return send(std::move(response), start_response_time);
                }
            });
        }));

// *    GET, HEAD, PUT, DELETE, OPTIONS, PATCH
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/player/action"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/player/action"}, http::verb::head, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/player/action"}, http::verb::put, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/player/action"}, http::verb::options, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/player/action"}, http::verb::patch, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/player/action"}, http::verb::delete_, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

// *    Mapping:
// *    /api/v1/game/tick
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/tick"}, http::verb::post, [self = this, start_response_time, &send] (const HttpRequest & req) {

            if (!self->game_.IsTimerStopped()) {
                HttpResponse response = ConstructBadRequestResponse(req, json_builder::GetBadRequest_s());
                return send(std::move(response), start_response_time);
            }

            net::dispatch(self->strand_, [self, start_response_time, &send, &req] {
                HttpResponse response;

                try {
                    if (req.at(http::field::content_type) != http_content_type::JSON) {
                        throw "Content-Type is invalid";
                    }
                } catch (std::exception & ex) {
                    HttpResponse response = ConstructBadRequestResponse(req, ex.what());
                    return send(std::move(response), start_response_time);
                }
                
                sys::error_code ec;

                json::value val = json::parse(req.body(), ec);

                if (ec) {
                    HttpResponse response = ConstructBadRequestResponse(req, "Json parse error"sv);
                    return send(std::move(response), start_response_time);
                }

                if (!val.as_object().contains("timeDelta")) {
                    HttpResponse response = ConstructBadRequestResponse(req, "No field \"timeDelta\""sv);
                    return send(std::move(response), start_response_time);
                }

                try {
                    int time_delta = val.at("timeDelta").as_int64();

                    self->game_.Tick(time_delta, [&self, &time_delta] {
                        for (model::GameSession & session : self->game_.GetSessions()) {
                            session.AddItems(self->generator_.Generate(std::chrono::milliseconds(time_delta), session.GetItems().size(), session.GetDogs().size()));
                            collision_detector::UpdateSessionItems(session, time_delta);
                        }
                    });

                    self->app_.Tick(std::chrono::milliseconds(time_delta));

                    response = ConstructOkResponse(req);
                    return send(std::move(response), start_response_time);
                } catch (std::exception & ex) {
                    HttpResponse response = ConstructBadRequestResponse(req, "Invalid type of field \"timeDelta\""sv);
                    return send(std::move(response), start_response_time);
                }
            });

        }));

// *    GET, HEAD, PUT, DELETE, OPTIONS, PATCH
        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/tick"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/tick"}, http::verb::head, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/tick"}, http::verb::put, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/tick"}, http::verb::delete_, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/tick"}, http::verb::options, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

        endpoints.emplace_back(endpoint::Endpoint({"/api/v1/game/tick"}, http::verb::patch, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructMethodNotAllowedResponse(req, "POST"sv);
            send(std::move(response), start_response_time);
        }));

// *    Mapping:
// *    /api/...
        endpoints.emplace_back(endpoint::Endpoint({"/api/"}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {
            HttpResponse response = ConstructBadRequestResponse(req, json_builder::GetBadRequest_s());
            send(std::move(response), start_response_time);
        }, true));

// *    Mapping local file access:
// *    {root}/...
        endpoints.emplace_back(endpoint::Endpoint({std::string(req.target().data(), req.target().size()).c_str()}, http::verb::get, [self = this, start_response_time, &send] (const HttpRequest & req) {
            sys::error_code ec;

            HttpFileResponse response = ConstructFileResponse(req, self->root_, ec);
            
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
    app::Application & app_;
    loot_gen::LootGenerator& generator_;
    std::vector<extra_data::MapExtraData> maps_extra_data_;
    Strand strand_;
    fs::path root_;
};

}  // namespace http_handler
