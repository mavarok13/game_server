#include "catch2/catch_test_macros.hpp"

#include "../src/http_utils.h"

SCENARIO("Url decoding tests") {
    using namespace std::literals;

    CHECK(http_utils::UrlDecode(""sv) == ""s);
    CHECK(http_utils::UrlDecode("+"sv) == " "s);
    CHECK(http_utils::UrlDecode("HIHIHI"sv) == "HIHIHI"s);
    CHECK(http_utils::UrlDecode("Hello+chel"sv) == "Hello chel"s);
    CHECK(http_utils::UrlDecode("%4A%4F%4B%45"sv) == "JOKE"s);
    CHECK(http_utils::UrlDecode("I+love%20B%6F%6FST"sv) == "I love BooST"s);
    CHECK_THROWS_AS(http_utils::UrlDecode("%4A%4F%4B%4"sv), std::invalid_argument);
    CHECK_THROWS_AS(http_utils::UrlDecode("I+love%20B%6F%6ST"sv), std::invalid_argument);
}

SCENARIO("Token formatting tests") {
    using namespace std::literals;

    CHECK(http_utils::FormatToken("Bearer 00000000ffffffff00000000ffffffff\n\n"sv) == "00000000ffffffff00000000ffffffff"s);
    CHECK(http_utils::FormatToken("Bearer 00000000ffffffff00000000ffffffff"sv) == "00000000ffffffff00000000ffffffff"s);
    CHECK_THROWS_AS(http_utils::FormatToken("Bear 00000000"sv), std::invalid_argument);
}