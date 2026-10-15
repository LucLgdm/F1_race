/* ****************************************** */
/*   test_basic.cpp                           */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/15 15:37:35 by Luc      */
/*   Updated: 2026/10/15 15:37:35 by Luc      */
/* ****************************************** */

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Premier test") {
    REQUIRE(2 + 2 == 4);
}