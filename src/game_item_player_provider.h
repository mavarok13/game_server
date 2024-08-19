#pragma once

#include "collision_detector.h"
#include "model.h"
#include "geom.h"

namespace collision_detector {

class GameItemPlayerProvider : public ItemGathererProvider {
public:
    GameItemPlayerProvider(std::vector<model::Item> items, std::vector<model::Dog> dogs) : items_{items}, dogs_{dogs} {}

    size_t ItemsCount() const override {
        return items_.size();
    }

    Item GetItem(size_t idx) const override {
        geom::Point2D position(items_.at(idx).GetPosition().x, items_.at(idx).GetPosition().y);

        return Item{position, model::Item::WIDTH};
    }

    size_t GatherersCount() const override {
        return dogs_.size();
    }

    Gatherer GetGatherer(size_t idx) const override {
        geom::Point2D start_pos(dogs_.at(idx).GetPrevPosition().x, dogs_.at(idx).GetPrevPosition().y);
        geom::Point2D end_pos(dogs_.at(idx).GetPosition().x, dogs_.at(idx).GetPosition().y);

        return Gatherer{start_pos, end_pos, model::Dog::WIDTH};
    }
private:
    std::vector<model::Item> items_;
    std::vector<model::Dog> dogs_;
};

} // collision_detector