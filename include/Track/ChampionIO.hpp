/* ****************************************** */
/*   ChampionIO.hpp                           */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/20 14:50:18 by Luc      */
/*   Updated: 2026/10/20 14:50:18 by Luc      */
/* ****************************************** */

#pragma once

#include "Track/Track.hpp"
#include "ML/Network.hpp"
#include "Exception.hpp"

#include <iosfwd>
#include <cmath>
#include <iomanip>
#include <istream>
#include <limits>
#include <locale>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

struct SavedChampion {
	Track track;
	Network network;
	float lapTime;
};

class ChampionIO {
	public:
		static void write(std::ostream& output, const Track& track, const Network& network, float lapTime);
		[[nodiscard]] static SavedChampion read(std::istream& input);
};
