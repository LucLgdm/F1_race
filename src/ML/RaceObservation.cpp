#include "ML/RaceObservation.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

std::array<float,23> observeTraffic(const CarObservation& road,
    const CarSnapshot& self, std::span<const CarSnapshot> neighbours) {
    std::array<float,23> result{};
    const auto base=road.normalized(40.0f,50.0f);
    std::copy(base.begin(),base.end(),result.begin());
    std::array<const CarSnapshot*,3> nearest{};
    std::array<float,3> distances{};
    distances.fill(std::numeric_limits<float>::infinity());
    for (const auto& other : neighbours) {
        const float dx=other.position.x-self.position.x, dy=other.position.y-self.position.y;
        const float d=dx*dx+dy*dy;
        if (d>40.0f*40.0f) continue;
        for (std::size_t k=0;k<3;++k) if (d<distances[k]) {
            for (std::size_t j=2;j>k;--j) { distances[j]=distances[j-1]; nearest[j]=nearest[j-1]; }
            distances[k]=d; nearest[k]=&other; break;
        }
    }
    const float c=std::cos(self.heading), s=std::sin(self.heading);
    for (std::size_t k=0;k<3;++k) if (nearest[k]) {
        const auto& o=*nearest[k];
        const float dx=o.position.x-self.position.x, dy=o.position.y-self.position.y;
        const float vx=o.velocity.x-self.velocity.x, vy=o.velocity.y-self.velocity.y;
        const std::size_t i=8+5*k;
        result[i]=1.0f;
        result[i+1]=(c*dx+s*dy)/40.0f;
        result[i+2]=(-s*dx+c*dy)/40.0f;
        // Two cars can approach at up to twice max speed.
        result[i+3]=std::clamp((c*vx+s*vy)/100.0f,-1.0f,1.0f);
        result[i+4]=std::clamp((-s*vx+c*vy)/100.0f,-1.0f,1.0f);
    }
    return result;
}
