#pragma once

#include <optional>

#include "../src/collision_detector.h"

using namespace collision_detector;

using Events = std::vector<GatheringEvent>;
using EventOpt = std::optional<GatheringEvent>;

class TestItemGathererProvider : public ItemGathererProvider {
public:
    TestItemGathererProvider() {}
    TestItemGathererProvider(std::vector<Item> items, std::vector<Gatherer> gatherers) : items_(items), gatherers_(gatherers) {}

    size_t ItemsCount() const override {
        return items_.size();
    }

    Item GetItem(size_t idx) const override {
        return items_[idx];
    }

    void SetItems(std::vector<Item> items) {
        items_ = items;
    }

    size_t GatherersCount() const override {
        return gatherers_.size();
    }

    Gatherer GetGatherer(size_t idx) const override {
        return gatherers_[idx];
    }

    void SetGatherers(std::vector<Gatherer> gatherers) {
        gatherers_ = gatherers;
    }

private:
    std::vector<Item> items_;
    std::vector<Gatherer> gatherers_;
};

EventOpt GetGatheringEventByOrder(Events events, size_t gatherer_id, unsigned int order);

void CheckEvent(GatheringEvent event, size_t gatherer_id, size_t item_id, double sq_distance, double time);