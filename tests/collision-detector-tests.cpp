#define _USE_MATH_DEFINES

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_predicate.hpp"
#include "catch2/matchers/catch_matchers_floating_point.hpp"
#include "catch2/matchers/catch_matchers_container_properties.hpp"

#include "collision-detector-tests.h"
#include "../src/collision_detector.h"
#include "../src/geom.h"

using Catch::Matchers::Predicate;
using Catch::Matchers::WithinRel;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::SizeIs;

EventOpt GetGatheringEventByOrder(Events events, size_t gatherer_id, unsigned int order) {
    for (GatheringEvent & event : events) {
        if (event.gatherer_id == gatherer_id) {
            if (order == 0) {
                return EventOpt {event};
            }
            --order;
        }
    }

    return std::nullopt;
}

void CheckEvent(GatheringEvent event, size_t gatherer_id, size_t item_id, double sq_distance, double time) {
    REQUIRE(event.gatherer_id == gatherer_id);
    REQUIRE(event.item_id == item_id);
    CHECK_THAT(event.sq_distance, WithinRel(sq_distance, 1e-4));
    CHECK_THAT(event.time, WithinRel(time, 1e-4));
}

SCENARIO("Test find gather event function") {
    GIVEN("A gatherer") {
        std::vector<Gatherer> gatherers{Gatherer{geom::Point2D{0, 0}, geom::Point2D{10, 10}, 0.5}};

        TestItemGathererProvider provider;
        provider.SetGatherers(gatherers);

        WHEN("No items anywhere") {
            CHECK_THAT(FindGatherEvents(provider), SizeIs(0));
        }

        WHEN("Two items are created which gatherer can pick") {

            std::vector items{
                Item{geom::Point2D{5, 5}, 0.5},
                Item{geom::Point2D{7, 7.5}, 0.5}
            };

            AND_WHEN("One more item are added that a gatherer can't get") {
                items.emplace_back(geom::Point2D{10, 0}, 0.5);

                provider.SetItems(items);

                Events events = FindGatherEvents(provider);

                REQUIRE_THAT(events, SizeIs(2));

                EventOpt e1 = GetGatheringEventByOrder(events, 0, 0);
                EventOpt e2 = GetGatheringEventByOrder(events, 0, 1);

                REQUIRE(e1);
                REQUIRE(e2);

                CheckEvent(*e1, 0, 0, 0, 0.5);
                CheckEvent(*e2, 0, 1, 0.125, 0.72499);
            }

            AND_WHEN("Add more gatherer and two items which they can pick") {
                gatherers.emplace_back(geom::Point2D{7, 0}, geom::Point2D{7, 10}, 0.6);
                items.emplace_back(geom::Point2D{7.5, 0}, 0.4);
                items.emplace_back(geom::Point2D{7, 10}, 0.3);
                items.emplace_back(geom::Point2D{10, 0}, 0.5);

                provider.SetGatherers(gatherers);
                provider.SetItems(items);

                Events events = FindGatherEvents(provider);

                REQUIRE_THAT(events, SizeIs(5));

                EventOpt e1_1 = GetGatheringEventByOrder(events, 0, 0);
                EventOpt e1_2 = GetGatheringEventByOrder(events, 0, 1);
                
                EventOpt e2_1 = GetGatheringEventByOrder(events, 1, 0);
                EventOpt e2_2 = GetGatheringEventByOrder(events, 1, 1);
                EventOpt e2_3 = GetGatheringEventByOrder(events, 1, 2);

                REQUIRE(e1_1);
                REQUIRE(e1_2);
                REQUIRE(e2_1);
                REQUIRE(e2_2);
                REQUIRE(e2_3);

// *   *   *   *    (5, 5)
                CheckEvent(*e1_1, 0, 0, 0, 0.5);      
// *   *   *   *    (7, 7.5)            
                CheckEvent(*e1_2, 0, 1, 0.125, 0.72499);

// *   *   *   *    (7.8, 0)            
                CheckEvent(*e2_1, 1, 2, 0.25, 0);
// *   *   *   *    (7, 7.5)
                CheckEvent(*e2_2, 1, 1, 0, 0.75);
// *   *   *   *    (7, 10)
                CheckEvent(*e2_3, 1, 3, 0, 1);

            }
        }
        
    }

}