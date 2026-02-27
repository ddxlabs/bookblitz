#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>
#include "OrderBook.hpp"

using namespace trading;
using Catch::Approx;

static Event ev(Action action, Side side, Price price, Volume volume, OrderId oid) {
    return Event{action, side, price, volume, oid};
}

// ============================================================================
// TEST 1: One of each basic operation
// ============================================================================
TEST_CASE("Basic order operations", "[orderbook][basic]") {
    OrderBook book;

    SECTION("Add bid") {
        book.process_event(ev(Action::Add, Side::Bid, 100, 10, 1));

        CHECK(book.get_price_at_level(true,  0) == Approx(100.0));
        CHECK(book.get_vol_at_level  (true,  0) == Approx(10.0));
        CHECK(std::isnan(book.get_price_at_level(true, 1)));
        CHECK(std::isnan(book.get_mid_price()));
        CHECK(std::isnan(book.get_spread()));
    }

    SECTION("Add ask") {
        book.process_event(ev(Action::Add, Side::Ask, 102, 5, 1));

        CHECK(book.get_price_at_level(false, 0) == Approx(102.0));
        CHECK(book.get_vol_at_level  (false, 0) == Approx(5.0));
        CHECK(std::isnan(book.get_price_at_level(false, 1)));
        CHECK(std::isnan(book.get_mid_price()));
        CHECK(std::isnan(book.get_spread()));
    }

    SECTION("Modify bid") {
        book.process_event(ev(Action::Add,    Side::Bid, 100, 10, 1));
        book.process_event(ev(Action::Modify, Side::Bid, 105, 20, 1));

        CHECK(book.get_price_at_level(true, 0) == Approx(105.0));
        CHECK(book.get_vol_at_level  (true, 0) == Approx(20.0));
        CHECK(std::isnan(book.get_price_at_level(true, 1)));
    }

    SECTION("Modify ask") {
        book.process_event(ev(Action::Add,    Side::Ask, 102, 5, 1));
        book.process_event(ev(Action::Modify, Side::Ask,  98, 3, 1));

        CHECK(book.get_price_at_level(false, 0) == Approx(98.0));
        CHECK(book.get_vol_at_level  (false, 0) == Approx(3.0));
        CHECK(std::isnan(book.get_price_at_level(false, 1)));
    }

    SECTION("Cancel bid") {
        book.process_event(ev(Action::Add,    Side::Bid, 100, 10, 1));
        book.process_event(ev(Action::Cancel, Side::Bid,   0,  0, 1));

        CHECK(std::isnan(book.get_price_at_level(true, 0)));
        CHECK(std::isnan(book.get_vol_at_level  (true, 0)));
    }

    SECTION("Cancel ask") {
        book.process_event(ev(Action::Add,    Side::Ask, 102, 5, 1));
        book.process_event(ev(Action::Cancel, Side::Ask,   0, 0, 1));

        CHECK(std::isnan(book.get_price_at_level(false, 0)));
        CHECK(std::isnan(book.get_vol_at_level  (false, 0)));
    }
}

// ============================================================================
// TEST 2: Intensive multi-order session
// ============================================================================
TEST_CASE("Intensive multi-order session", "[orderbook][intensive]") {
    OrderBook book;

    // -- Build initial book --------------------------------------------------
    // Bids: oid 1-6 (two orders share price 100)
    book.process_event(ev(Action::Add, Side::Bid, 100, 10,  1));
    book.process_event(ev(Action::Add, Side::Bid,  99, 20,  2));
    book.process_event(ev(Action::Add, Side::Bid,  98, 15,  3));
    book.process_event(ev(Action::Add, Side::Bid,  97,  5,  4));
    book.process_event(ev(Action::Add, Side::Bid,  96, 30,  5));
    book.process_event(ev(Action::Add, Side::Bid, 100,  5,  6)); // aggregates @ 100

    // Asks: oid 7-12 (two orders share price 101)
    book.process_event(ev(Action::Add, Side::Ask, 101,  8,  7));
    book.process_event(ev(Action::Add, Side::Ask, 102, 12,  8));
    book.process_event(ev(Action::Add, Side::Ask, 103, 25,  9));
    book.process_event(ev(Action::Add, Side::Ask, 104,  7, 10));
    book.process_event(ev(Action::Add, Side::Ask, 105, 18, 11));
    book.process_event(ev(Action::Add, Side::Ask, 101,  2, 12)); // aggregates @ 101

    // -- Verify initial state ------------------------------------------------
    // Bids descending: 100(15), 99(20), 98(15), 97(5), 96(30)
    CHECK(book.get_price_at_level(true, 0) == Approx(100.0));
    CHECK(book.get_vol_at_level  (true, 0) == Approx(15.0));
    CHECK(book.get_price_at_level(true, 1) == Approx(99.0));
    CHECK(book.get_vol_at_level  (true, 1) == Approx(20.0));
    CHECK(book.get_price_at_level(true, 4) == Approx(96.0));
    CHECK(std::isnan(book.get_price_at_level(true, 5)));

    // Asks ascending: 101(10), 102(12), 103(25), 104(7), 105(18)
    CHECK(book.get_price_at_level(false, 0) == Approx(101.0));
    CHECK(book.get_vol_at_level  (false, 0) == Approx(10.0));
    CHECK(book.get_price_at_level(false, 1) == Approx(102.0));
    CHECK(std::isnan(book.get_price_at_level(false, 5)));

    CHECK(book.get_mid_price() == Approx(100.5));
    CHECK(book.get_spread()    == Approx(1.0));

    // -- Modify bid oid=1: 100 -> 95 ----------------------------------------
    // bid@100 drops from 15 to 5 (oid=6 remains); new level at 95
    book.process_event(ev(Action::Modify, Side::Bid, 95, 10, 1));

    CHECK(book.get_price_at_level(true, 0) == Approx(100.0));
    CHECK(book.get_vol_at_level  (true, 0) == Approx(5.0));
    CHECK(book.get_price_at_level(true, 5) == Approx(95.0));
    CHECK(book.get_vol_at_level  (true, 5) == Approx(10.0));

    // -- Modify ask oid=7: 101 -> 106 ----------------------------------------
    // ask@101 drops from 10 to 2 (oid=12 remains); new level at 106
    book.process_event(ev(Action::Modify, Side::Ask, 106, 8, 7));

    CHECK(book.get_price_at_level(false, 0) == Approx(101.0));
    CHECK(book.get_vol_at_level  (false, 0) == Approx(2.0));
    CHECK(book.get_price_at_level(false, 5) == Approx(106.0));
    CHECK(book.get_vol_at_level  (false, 5) == Approx(8.0));

    CHECK(book.get_mid_price() == Approx(100.5));
    CHECK(book.get_spread()    == Approx(1.0));

    // -- Cancel last order at bid@100 (oid=6) --------------------------------
    book.process_event(ev(Action::Cancel, Side::Bid, 0, 0, 6));

    CHECK(book.get_price_at_level(true, 0) == Approx(99.0));
    CHECK(book.get_mid_price() == Approx(100.0));
    CHECK(book.get_spread()    == Approx(2.0));

    // -- Cancel last order at ask@101 (oid=12) --------------------------------
    book.process_event(ev(Action::Cancel, Side::Ask, 0, 0, 12));

    CHECK(book.get_price_at_level(false, 0) == Approx(102.0));
    CHECK(book.get_mid_price() == Approx(100.5));
    CHECK(book.get_spread()    == Approx(3.0));

    // -- Drain all remaining bids --------------------------------------------
    book.process_event(ev(Action::Cancel, Side::Bid, 0, 0, 2)); // 99
    book.process_event(ev(Action::Cancel, Side::Bid, 0, 0, 3)); // 98
    book.process_event(ev(Action::Cancel, Side::Bid, 0, 0, 4)); // 97
    book.process_event(ev(Action::Cancel, Side::Bid, 0, 0, 5)); // 96
    book.process_event(ev(Action::Cancel, Side::Bid, 0, 0, 1)); // 95 (after modify)

    CHECK(std::isnan(book.get_price_at_level(true, 0)));
    CHECK(std::isnan(book.get_mid_price()));
    CHECK(std::isnan(book.get_spread()));

    // -- Drain all remaining asks --------------------------------------------
    book.process_event(ev(Action::Cancel, Side::Ask, 0, 0,  8)); // 102
    book.process_event(ev(Action::Cancel, Side::Ask, 0, 0,  9)); // 103
    book.process_event(ev(Action::Cancel, Side::Ask, 0, 0, 10)); // 104
    book.process_event(ev(Action::Cancel, Side::Ask, 0, 0, 11)); // 105
    book.process_event(ev(Action::Cancel, Side::Ask, 0, 0,  7)); // 106 (after modify)

    CHECK(std::isnan(book.get_price_at_level(false, 0)));
}
