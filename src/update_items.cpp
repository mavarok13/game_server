#include <algorithm>

#include "update_items.h"
#include "game_item_player_provider.h"
#include "game_office_player_provider.h"
#include "app.h"

namespace collision_detector { 
void UpdateSessionItems(model::GameSession & session, int interval) {
//  Detect items collision
    collision_detector::GameItemPlayerProvider items_provider(session.GetItems(), session.GetDogs());
    std::vector<collision_detector::GatheringEvent> items_events = collision_detector::FindGatherEvents(items_provider);

//  Detect offices collision
    collision_detector::GameOfficePlayerProvider offices_provider(session.GetMap()->GetOffices(), session.GetDogs());
    std::vector<collision_detector::GatheringEvent> offices_events = collision_detector::FindGatherEvents(offices_provider);

//  Convert local time to global
    for (collision_detector::GatheringEvent & event : items_events) {
        model::Dog dog = session.GetDogs()[event.gatherer_id];

        model::Vector2 move_vector{dog.GetPosition().x - dog.GetPrevPosition().x, dog.GetPosition().y - dog.GetPrevPosition().y};

        double distance = std::sqrt(move_vector.x*move_vector.x + move_vector.y*move_vector.y);

        event.time *= distance/(session.GetMap()->GetDogSpeed()*interval);
    }

    for (collision_detector::GatheringEvent & event : offices_events) {
        model::Dog dog = session.GetDogs()[event.gatherer_id];

        model::Vector2 move_vector{dog.GetPosition().x - dog.GetPrevPosition().x, dog.GetPosition().y - dog.GetPrevPosition().y};

        double distance = std::sqrt(move_vector.x*move_vector.x + move_vector.y*move_vector.y);

        event.time *= distance/(session.GetMap()->GetDogSpeed()*interval);
    }

    std::sort(items_events.begin(), items_events.end(), collision_detector::CompareGatheringEvents);
    std::sort(offices_events.begin(), offices_events.end(), collision_detector::CompareGatheringEvents);

//  Detection handler
    std::vector<int> removed_items;

    for (auto item_event = items_events.begin(), office_event = offices_events.begin(); item_event != items_events.end() || office_event != offices_events.end();) {
        if (office_event == offices_events.end() || item_event != items_events.end() && item_event->time <= office_event->time) {
            if (session.GetDogs()[item_event->gatherer_id].GetItemsCount() >= session.GetMap()->GetInventorySize()) {
                ++item_event;
                continue;
            }

            if (std::find(removed_items.begin(), removed_items.end(), item_event->item_id) == std::end(removed_items)) {
                session.GetDogs()[item_event->gatherer_id].AddItem(session.GetItems()[item_event->item_id]);

                removed_items.emplace_back(item_event->item_id);
            }

            ++item_event;
        } else {
            for (app::Player & player : app::PlayersManager::Instance().GetPlayers()) {
                if (player.GetSession() == &session && player.GetPlayerId() == office_event->gatherer_id) {
                    for (model::Item & item : (session.GetDogs()[office_event->gatherer_id]).GetItems()) {
                        player.AddScores(item.GetType().GetCost());
                    }

                    break;
                }
            }

            session.GetDogs()[office_event->gatherer_id].PutItems();

            ++office_event;
        }
    }

    while (removed_items.size() != 0) {
        session.RemoveItemById((session.GetItems()[*removed_items.begin()]).GetId());
        removed_items.erase(removed_items.begin());
    }
}
} //namespace collision_detector