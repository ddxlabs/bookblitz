#include "Engine.hpp"

void Engine::run(
    int64_t n_events, const int8_t *actions, const int8_t *sides,
    const int64_t *prices, const int64_t *sizes, const uint64_t *order_ids
) {

    for (int64_t i = 0; i < n_events; i++) {
        trading::Event e = {
            static_cast<trading::Action>(actions[i]),
            static_cast<trading::Side>(sides[i]),
            prices[i], sizes[i], order_ids[i]
        };

        book_.process_event(e);
    }
}
