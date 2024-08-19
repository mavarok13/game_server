#include "collision_detector.h"

namespace collision_detector {

CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
    const double u_x = c.x - a.x;
    const double u_y = c.y - a.y;
    const double v_x = b.x - a.x;
    const double v_y = b.y - a.y;
    const double u_dot_v = u_x * v_x + u_y * v_y;
    const double u_len2 = u_x * u_x + u_y * u_y;
    const double v_len2 = v_x * v_x + v_y * v_y;
    const double proj_ratio = u_dot_v / v_len2;
    const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

    return CollectionResult(sq_distance, proj_ratio);
}

bool CompareGatheringEvents(GatheringEvent e1, GatheringEvent e2) {
    return e1.time < e2.time;
}

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider & provider) {
    std::vector<GatheringEvent> events;

    for (int gi = 0; gi < provider.GatherersCount(); ++gi) {
        Gatherer gatherer = provider.GetGatherer(gi);

        if (gatherer.start_pos.x != gatherer.end_pos.x || gatherer.start_pos.y != gatherer.end_pos.y) {
            for (int ii = 0; ii < provider.ItemsCount(); ++ii) {
                Item item = provider.GetItem(ii);

                CollectionResult collection_result = TryCollectPoint(gatherer.start_pos, gatherer.end_pos, item.position);

                if (collection_result.IsCollected(gatherer.width/2+item.width/2)) {
                    events.emplace_back(ii, gi, collection_result.sq_distance, collection_result.proj_ratio);
                }
            }
        }
    }
    
    std::sort(events.begin(), events.end(), CompareGatheringEvents);

    return events;
}

}  // namespace collision_detector