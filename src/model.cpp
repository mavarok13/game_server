#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

bool operator==(const Vector2 & v1, const Vector2 & v2) {
    return v1.x == v2.x && v1.y == v2.y;
}

bool operator!=(const Vector2 & v1, const Vector2 & v2) {
    return !(v1==v2);
}

std::vector<Road> GetDogStandRoads(const Dog & dog, const Map & map) {
    std::vector<Road> picked_roads;

    Vector2 position = dog.GetPosition();

    for (const Road & road : map.GetRoads()) {
        Point start{0, 0};
        Point end{0, 0};

        if (road.IsHorizontal() && road.GetStart().x <= road.GetEnd().x) {
            start = road.GetStart();
            end = road.GetEnd();
        } else if (road.IsHorizontal() && road.GetStart().x > road.GetEnd().x) {
            start = road.GetEnd();
            end = road.GetStart();
        }

        if (road.IsVertical() && road.GetStart().y <= road.GetEnd().y) {
            start = road.GetStart();
            end = road.GetEnd();
        } else if (road.IsVertical() && road.GetStart().y > road.GetEnd().y) {
            start = road.GetEnd();
            end = road.GetStart();
        }

        if (position.x >= start.x - ROAD_WIDTH/2 && position.x <= end.x + ROAD_WIDTH/2 && position.y >= start.y - ROAD_WIDTH/2 && position.y <= end.y + ROAD_WIDTH/2) {
            picked_roads.emplace_back(road);
        }
    }

    return picked_roads;
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

void GameSession::Update(unsigned int delta_time) {
    for (Dog & dog : dogs_) {
        Vector2 position = dog.GetPosition();
        Vector2 speed = dog.GetSpeed();
        Direction dir = dog.GetDirection();

        std::vector <Road> picked_roads = GetDogStandRoads(dog, *map_);

        if (picked_roads.size() != 0) {
            position.x += speed.x * (delta_time/MILLISECONDS_IN_SECOND);
            position.y += speed.y * (delta_time/MILLISECONDS_IN_SECOND);
        }

        for (const Road & road : picked_roads) {
            Point start{0, 0};
            Point end{0, 0};

            if (road.IsHorizontal() && road.GetStart().x <= road.GetEnd().x) {
                start = road.GetStart();
                end = road.GetEnd();
            } else if (road.IsHorizontal() && road.GetStart().x > road.GetEnd().x) {
                start = road.GetEnd();
                end = road.GetStart();
            }

            if (road.IsVertical() && road.GetStart().y <= road.GetEnd().y) {
                start = road.GetStart();
                end = road.GetEnd();
            } else if (road.IsVertical() && road.GetStart().y > road.GetEnd().y) {
                start = road.GetEnd();
                end = road.GetStart();
            }

            if (picked_roads.size() != 2) {

                if (position.x < start.x - ROAD_WIDTH/2) {
                    position.x = start.x - ROAD_WIDTH/2;
                    dog.SetSpeed(Vector2{0, 0});
                } else if (position.x > end.x + ROAD_WIDTH/2) {
                    position.x = end.x + ROAD_WIDTH/2; 
                    dog.SetSpeed(Vector2{0, 0});
                } else if (position.y < start.y - ROAD_WIDTH/2) {
                    position.y = start.y - ROAD_WIDTH/2;
                    dog.SetSpeed(Vector2{0, 0});
                } else if (position.y > end.y + ROAD_WIDTH/2) {
                    position.y = end.y + ROAD_WIDTH/2;
                    dog.SetSpeed(Vector2{0, 0});
                }
            } else {
                if (road.IsHorizontal() && (dog.GetDirection() == Direction::WEST || dog.GetDirection() == Direction::EAST)) {
                    if (position.x < start.x - ROAD_WIDTH/2) {
                        position.x = start.x - ROAD_WIDTH/2;
                        dog.SetSpeed(Vector2{0, 0});
                    } else if (position.x > end.x + ROAD_WIDTH/2) {
                        position.x = end.x + ROAD_WIDTH/2;
                        dog.SetSpeed(Vector2{0, 0});
                    }
                } else if (road.IsVertical() && (dog.GetDirection() == Direction::SOUTH || dog.GetDirection() == Direction::NORTH)) {
                    if (position.y < start.y - ROAD_WIDTH/2) {
                        position.y = start.y - ROAD_WIDTH/2;
                        dog.SetSpeed(Vector2{0, 0});
                    } else if (position.y > end.y + ROAD_WIDTH/2) {
                        position.y = end.y + ROAD_WIDTH/2;
                        dog.SetSpeed(Vector2{0, 0});
                    }
                }
            }
        }

        dog.SetPosition(position);
    
    }
}

}  // namespace model
