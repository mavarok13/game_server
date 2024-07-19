#pragma once

#include "collision_detector.h"
#include "model.h"
#include "geom.h"

namespace collision_detector {

class GameOfficePlayerProvider : public ItemGathererProvider {
public:
    GameOfficePlayerProvider(std::vector<model::Office> offices, std::vector<model::Dog> dogs) : offices_{offices}, dogs_{dogs} {}

    size_t ItemsCount() const override {
        return offices_.size();
    }

    Item GetItem(size_t idx) const override {
        geom::Point2D position(offices_[idx].GetPosition().x, offices_[idx].GetPosition().y);
        return Item{position, model::Office::WIDTH};
    }

    size_t GatherersCount() const override {
        return dogs_.size();
    }

    Gatherer GetGatherer(size_t idx) const override {
        geom::Point2D start_pos(dogs_[idx].GetPrevPosition().x, dogs_[idx].GetPrevPosition().y);
        geom::Point2D end_pos(dogs_[idx].GetPosition().x, dogs_[idx].GetPosition().y);
        
        return Gatherer{start_pos, end_pos, model::Dog::WIDTH};
    }

private:
    std::vector<model::Office> offices_;
    std::vector<model::Dog> dogs_;

};

} //namespace collision_detector