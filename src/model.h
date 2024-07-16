#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <cstdlib>

#include "tagged.h"
#include "model_properties.h"
#include "random_generator.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

enum Direction { NORTH, SOUTH, WEST, EAST, ZERO };

class GameSession;

struct Point {
    Coord x, y;
};

struct Vector2 {
public:
    Vector2 () : x(0), y(0) {}
    Vector2 (double x, double y) : x(x), y(y) {}

    double x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

    static constexpr double WIDTH = 0.5;

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class ItemType {
public:
    ItemType(int type, unsigned int cost) : type_(type), cost_(cost) {}

    int GetType() {
        return type_;
    }

    unsigned int GetCost() noexcept {
        return cost_;
    }

    unsigned int GetCost() const noexcept {
        return cost_;
    }

private:
    int type_;
    unsigned int cost_;
};

class Item {
public:
    Item(int id, ItemType type, Point position) : id_{id}, type_{type}, position_{position} {}

    int GetId() {
        return id_;
    }

    int GetId() const {
        return id_;
    }

    ItemType GetType() {
        return type_;
    }

    ItemType GetType() const {
        return type_;
    }

    Point GetPosition() { 
        return position_;
    }

    Point GetPosition() const {
        return position_;
    }

    static constexpr double WIDTH = 0;

private:
    int id_;
    ItemType type_;
    Point position_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;
    using ItemsTypes = std::vector<ItemType>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    const ItemsTypes& GetItemsTypes() const noexcept {
        return items_types_;
    }

    float GetDogSpeed() const noexcept {
        return dog_speed_;
    }

    unsigned int GetInventorySize() const noexcept {
        return inventory_size_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    void AddItemType(ItemType type) {
        items_types_.emplace_back(type);
    }

    void SetSpeed(float speed) {
        dog_speed_ = speed;
    }

    void SetInventorySize(unsigned int inventory_size) {
        inventory_size_ = inventory_size;
    }



private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    ItemsTypes items_types_;

    float dog_speed_;
    unsigned int inventory_size_;
};

class Dog {
public:
    Dog(unsigned int id, Vector2 position, Vector2 speed = Vector2{0, 0}) : id_(id), position_(position), speed_(speed) {}

    unsigned int GetId() {
        return id_;
    }

    unsigned int GetId() const {
        return id_;
    }

    Vector2 GetPosition() {
        return position_;
    }

    Vector2 GetPosition() const {
        return position_;
    }

    Vector2 GetPrevPosition() {
        return prev_position_;
    }

    Vector2 GetPrevPosition() const {
        return prev_position_;
    }

    Vector2 GetSpeed() {
        return speed_;
    }

    Vector2 GetSpeed() const {
        return speed_;
    }

    Direction GetDirection() {
        return direction_;
    }

    Direction GetDirection() const {
        return direction_;
    }

    std::vector<Item> GetItems() {
        return items_;
    }

    std::vector<Item> GetItems() const {
        return items_;
    }

    size_t GetItemsCount() {
        return items_.size();
    }
    
    size_t GetItemsCount() const { 
        return items_.size();
    }

    void SetPosition(const Vector2 & position) {
        prev_position_ = position_;
        position_ = position;
    }

    void SetSpeed(const Vector2 & speed) {
        speed_ = speed;
    }

    void SetDirection(Direction direction) {
        direction_ = direction;
    }

    void AddItem(Item item) {
        items_.emplace_back(item);
    }

    void PutItems() {
        items_.clear();
    }

    static constexpr double WIDTH = 0.6;

private:
    unsigned int id_;
    Vector2 position_;
    Vector2 prev_position_;
    Vector2 speed_ = {0, 0};
    Direction direction_ = Direction::NORTH;
    std::vector<Item> items_;
};

class GameSession {
public:
    using Dogs = std::vector<Dog>;
    using Items = std::vector<Item>;

    explicit GameSession(Map * map, bool random_position = false) : map_(map), random_position_(random_position) {}

    Map * GetMap() {

        return map_;
    }

    void NewPlayer(unsigned int id) {

        Vector2 position{0, 0};

        position.x = map_->GetRoads().front().GetStart().x;
        position.y = map_->GetRoads().front().GetStart().y;
        
        if (random_position_) {
            Road picked_road = map_->GetRoads()[std::rand()%map_->GetRoads().size()];

            if (picked_road.IsHorizontal()) {
                double x = picked_road.GetStart().x+(picked_road.GetEnd().x-picked_road.GetStart().x)*RandomGenerator::GenerateUpTo(100)/100;
                position = Vector2{x, picked_road.GetStart().y};
            } else {
                double y = picked_road.GetStart().y+(picked_road.GetEnd().y-picked_road.GetStart().y)*RandomGenerator::GenerateUpTo(100)/100;
                position = Vector2{picked_road.GetStart().x, y};
            }
        }

        dogs_.emplace_back(id, position, Vector2{0, 0});
    }

    Dog * GetDogById(unsigned int id) {
        for (Dog & dog : dogs_) {
            if (dog.GetId() == id) {
                return &dog;
            }
        }

        return nullptr;
    }

    Dogs & GetDogs() {
        return dogs_;
    }

    Items & GetItems() {
        return items_;
    }

    void AddItems(int count) {
        for (int i = 0; i < count; ++i) {
            int picked_item_type = std::rand()%map_->GetItemsTypes().size();
            Road picked_road = map_->GetRoads()[std::rand()%map_->GetRoads().size()];

            Point position{0, 0};

            if(picked_road.IsHorizontal()) {
                position.x = picked_road.GetStart().x+(picked_road.GetEnd().x-picked_road.GetStart().x)*RandomGenerator::GenerateUpTo(100)/100;
                position.y = picked_road.GetStart().y;
            } else {
                position.x = picked_road.GetStart().x;
                position.y = picked_road.GetStart().y+(picked_road.GetEnd().y-picked_road.GetStart().y)*RandomGenerator::GenerateUpTo(100)/100;
            }

            items_.emplace_back(++item_last_id_, map_->GetItemsTypes()[picked_item_type], position);
        }
    }

    void RemoveItemById(int id) {
        for (auto item = items_.begin(); item != items_.end(); ++item) {
            if (item->GetId() == id) {
                items_.erase(item);
                break;
            }
        }
    }

    void Update(unsigned int delta_time);

private:
    Dogs dogs_;
    Map * map_;
    Items items_;
    bool random_position_ = false;

    int item_last_id_ = 0;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    GameSession * NewSession(Map * map) {

        for (GameSession & session : sessions_) {
            
            if (session.GetMap() == map) {

                return &session;
            }
        }

        sessions_.emplace_back(map, random_position_);

        return &sessions_.back();
    }

    std::vector<GameSession> & GetSessions() {

        return sessions_;
    }

    template <typename Action>
    void Tick(int delta_time, Action&& action) {

        for (GameSession & session : sessions_) {

            session.Update(delta_time);
        }
        action();
    }

    void SetRandomPosition(bool random_position) {
        random_position_ = random_position;
    }

    bool IsRandomPosition() {

        return random_position_;
    }

    bool IsRandomPosition() const {

        return random_position_;
    }

    bool IsTimerStopped() {

        return timer_stopped_;
    }

    bool IsTimerStopped() const {

        return timer_stopped_;
    }

    void SetTimerStopped(bool timer_stopped) {

        timer_stopped_ = timer_stopped;
    }

    double GetLootSpawnPeriod() {
        return loot_spawn_period_;
    }

    void SetLootSpawnPeriod(double loot_spawn_period) {
        loot_spawn_period_ = loot_spawn_period;
    }

    double GetLootSpawnProbability() {
        return loot_spawn_probability_;
    }

    void SetLootSpawnProbability(double loot_spawn_probability) {
        loot_spawn_probability_ = loot_spawn_probability;
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;

    std::vector<GameSession> sessions_;

    double loot_spawn_period_ = 0;
    double loot_spawn_probability_ = 0;

    bool timer_stopped_ = false;
    bool random_position_ = false;
};

std::vector<Road> GetDogStandRoads(const Dog & dog, const Map & map);

}  // namespace model
