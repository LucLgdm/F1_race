#include "Track/CarCollision.hpp"
#include "Exception.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace {
float dot(Vec2 a, Vec2 b) { return a.x*b.x + a.y*b.y; }
Vec2 sub(Vec2 a, Vec2 b) { return {a.x-b.x, a.y-b.y}; }
Vec2 axis(float h) { return {std::cos(h), std::sin(h)}; }
float angleDelta(float a, float b) { return std::remainder(b-a, 2.0f*std::numbers::pi_v<float>); }
void validate(CarFootprint s) {
    if (!std::isfinite(s.halfLength) || !std::isfinite(s.halfWidth) || s.halfLength<=0 || s.halfWidth<=0)
        throw carError("Invalid vehicle footprint");
}
float separation(const CarSnapshot& a, const CarSnapshot& b, CarFootprint s) {
    const Vec2 ax=axis(a.heading), ay{-ax.y,ax.x}, bx=axis(b.heading), by{-bx.y,bx.x};
    const Vec2 d=sub(b.position,a.position);
    float gap=-1e30f;
    for (Vec2 n : {ax,ay,bx,by}) {
        const float ra=s.halfLength*std::abs(dot(ax,n))+s.halfWidth*std::abs(dot(ay,n));
        const float rb=s.halfLength*std::abs(dot(bx,n))+s.halfWidth*std::abs(dot(by,n));
        gap=std::max(gap,std::abs(dot(d,n))-ra-rb);
    }
    return gap;
}
CarSnapshot interpolate(const CarSnapshot& a, const CarSnapshot& b, float t) {
    return {{a.position.x+(b.position.x-a.position.x)*t,
             a.position.y+(b.position.y-a.position.y)*t},a.heading+angleDelta(a.heading,b.heading)*t,{}};
}
}
CarSnapshot snapshotCar(const Car& car) noexcept {
    return {car.position(),car.heading(),{std::cos(car.heading())*car.speed(),std::sin(car.heading())*car.speed()}};
}
std::array<Vec2,4> footprintCorners(const CarSnapshot& c, CarFootprint s) {
    validate(s);
    const Vec2 x=axis(c.heading), y{-x.y,x.x};
    std::array<Vec2,4> result{};
    constexpr float signs[4][2]={{-1,-1},{1,-1},{1,1},{-1,1}};
    for (std::size_t i=0;i<4;++i)
        result[i]={c.position.x+signs[i][0]*s.halfLength*x.x+signs[i][1]*s.halfWidth*y.x,
                   c.position.y+signs[i][0]*s.halfLength*x.y+signs[i][1]*s.halfWidth*y.y};
    return result;
}
bool carsOverlap(const CarSnapshot& a, const CarSnapshot& b, CarFootprint s) {
    validate(s); return separation(a,b,s)<=0.0f;
}
std::optional<float> carContact(const CarSnapshot& a0, const CarSnapshot& a1,
    const CarSnapshot& b0, const CarSnapshot& b1, CarFootprint s) {
    validate(s);
    const Vec2 da=sub(a1.position,a0.position), db=sub(b1.position,b0.position), relative=sub(da,db);
    const float bound=std::hypot(relative.x,relative.y)+std::hypot(s.halfLength,s.halfWidth)*
        (std::abs(angleDelta(a0.heading,a1.heading))+std::abs(angleDelta(b0.heading,b1.heading)));
    if (std::abs(angleDelta(a0.heading,a1.heading))<1e-7f &&
        std::abs(angleDelta(b0.heading,b1.heading))<1e-7f) {
        // Exact swept SAT for translation. In particular, a narrow lateral gap
        // stays clear even with large longitudinal relative motion.
        const Vec2 ax=axis(a0.heading), ay{-ax.y,ax.x}, bx=axis(b0.heading), by{-bx.y,bx.x};
        const Vec2 d=sub(b0.position,a0.position), v=sub(db,da);
        float enter=0.0f, leave=1.0f;
        for (Vec2 n : {ax,ay,bx,by}) {
            const float radius=s.halfLength*(std::abs(dot(ax,n))+std::abs(dot(bx,n)))+
                               s.halfWidth*(std::abs(dot(ay,n))+std::abs(dot(by,n)));
            const float centre=dot(d,n), speed=dot(v,n);
            if (std::abs(speed)<1e-8f) {
                if (std::abs(centre)>radius) return {};
            } else {
                float t0=(-radius-centre)/speed, t1=(radius-centre)/speed;
                if (t0>t1) std::swap(t0,t1);
                enter=std::max(enter,t0); leave=std::min(leave,t1);
                if (enter>leave) return {};
            }
        }
        return enter;
    }
    float t=0.0f;
    for (int i=0;i<4096;++i) {
        const float gap=separation(interpolate(a0,a1,t),interpolate(b0,b1,t),s);
        if (gap<=0.0001f) return t;
        if (bound<=0.0f) return {};
        const float advance=gap/bound;
        if (t+advance>1.0f) return {};
        const float next=t+advance;
        if (next<=t) return t; // numerical contact at float precision
        t=next;
    }
    // Conservative fallback: never allow an unresolved swept contact through.
    return t;
}
