#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ML/RaceSession.hpp"
#include <limits>
#include <numbers>

namespace {
Network quietPolicy(std::size_t inputs=23) {
    std::vector<Neuron> neurons;
    for (int i=0;i<3;++i) neurons.emplace_back(std::vector<float>(inputs,0.0f),0.0f,Activation::Tanh);
    return Network{{Layer{std::move(neurons)}}};
}
RaceSettings settings(std::size_t cars=2) {
    RaceSettings result; result.cars=cars; result.laps=2; return result;
}
Track roundedCircuit() {
    std::vector<TrackSegment> pieces;
    TrackSegment segment{{0,0},0,60};
    for (int i=0;i<4;++i) {
        segment.rounded=false; segment.length=60; pieces.push_back(segment);
        segment.start=Track::segmentPoint(segment,60);
        segment.rounded=true; segment.radius=30; segment.turn=-1; pieces.push_back(segment);
        segment.start=Track::segmentPoint(segment,Track::segmentLength(segment));
        segment.heading-=std::numbers::pi_v<float>/2;
    }
    return Track{pieces,20,30};
}
CarControls followLane(const Track& track,const Car& car,float lateral) {
    const auto location=track.locate(car.position());
    const float ahead=location.coordinate+6.0f;
    const float heading=track.headingAt(ahead);
    auto target=track.pointAt(ahead);
    target.x-=std::sin(heading)*lateral; target.y+=std::cos(heading)*lateral;
    const float dx=target.x-car.position().x,dy=target.y-car.position().y;
    const float error=std::remainder(std::atan2(dy,dx)-car.heading(),2*std::numbers::pi_v<float>);
    return {car.speed()<8 ? .5f : .125f,0,
        std::clamp(-std::atan2(4.8f*std::sin(error),std::hypot(dx,dy))/.45f,-1.0f,1.0f)};
}
}
TEST_CASE("A full race starts behind the line with separate traffic-aware cars", "[ml][race_session]") {
    const Track track{800,500,20}; const auto policy=quietPolicy();
    RaceSession race{track,policy};
    REQUIRE(race.size()==8); REQUIRE(race.activeCount()==8); REQUIRE_FALSE(race.finished());
    REQUIRE(race.targetLaps()==1);
    REQUIRE(race.standings().front()==0); REQUIRE(race.cameraTarget()==0);
    for (std::size_t i=0;i<race.size();++i) {
        REQUIRE(race.racePosition(i)<0); REQUIRE(race.lapCount(i)==0);
        for (std::size_t j=i+1;j<race.size();++j)
            REQUIRE_FALSE(carsOverlap(snapshotCar(race.car(i)),snapshotCar(race.car(j))));
    }
    const auto input=race.observation(0);
    REQUIRE(input[8]==1); REQUIRE(input[13]==1); REQUIRE(input[18]==1);
    REQUIRE(std::abs(input[9])>0); // self is excluded from neighbour slots
    const auto parameters=policy.parameters(); race.step(.05f);
    REQUIRE(race.policy(0).parameters()==parameters); REQUIRE(race.policy(7).parameters()==parameters);
}
TEST_CASE("A shared-world contact stops both entrants without overlap or advancing their poses", "[ml][race_session]") {
    const Track track{800,500,20}; const auto network=quietPolicy();
    RaceSession race{track,std::vector<Network>(2,network),settings(),{{-6,0},{-14,0}}};
    const std::array<CarControls,2> controls{{{0,0,0},{1,0,0}}};
    Vec2 a{},b{};
    for (int tick=0;tick<1200 && !race.finished();++tick) {
        a=race.car(0).position(); b=race.car(1).position(); race.stepWithControls(controls,1.0f/120);
    }
    REQUIRE(race.finished()); REQUIRE(race.contacts()==1);
    REQUIRE(race.status(0)==DriverStatus::CarContact); REQUIRE(race.status(1)==DriverStatus::CarContact);
    REQUIRE(race.car(0).position().x==a.x); REQUIRE(race.car(1).position().x==b.x);
    REQUIRE(race.car(0).speed()==0); REQUIRE(race.car(1).speed()==0);
    REQUIRE_FALSE(carsOverlap(snapshotCar(race.car(0)),snapshotCar(race.car(1))));
}
TEST_CASE("Rollback collision resolution also protects a third following car", "[ml][race_session]") {
    const Track track{800,500,20}; const auto network=quietPolicy();
    RaceSession race{track,std::vector<Network>(3,network),settings(3),{{-6,0},{-12,0},{-16.01f,0}}};
    const std::array<CarControls,3> controls{{{0,0,0},{1,0,0},{1,0,0}}};
    for (int tick=0;tick<1200 && !race.finished();++tick) race.stepWithControls(controls,1.0f/120);
    REQUIRE(race.finished()); REQUIRE(race.contacts()==2);
    for (std::size_t i=0;i<3;++i) {
        REQUIRE(race.status(i)==DriverStatus::CarContact);
        for (std::size_t j=i+1;j<3;++j)
            REQUIRE_FALSE(carsOverlap(snapshotCar(race.car(i)),snapshotCar(race.car(j))));
    }
}
TEST_CASE("Reordering entrants cannot change simultaneous shared-world collisions", "[ml][race_session]") {
    const Track track{800,500,20}; const auto network=quietPolicy();
    RaceSession first{track,std::vector<Network>(3,network),settings(3),{{-6,0},{-12,0},{-16.01f,0}}};
    RaceSession reversed{track,std::vector<Network>(3,network),settings(3),{{-16.01f,0},{-12,0},{-6,0}}};
    const std::array<CarControls,3> a{{{0,0,0},{1,0,0},{1,0,0}}},b{{{1,0,0},{1,0,0},{0,0,0}}};
    for (int tick=0;tick<1200 && !first.finished();++tick) {
        first.stepWithControls(a,1.0f/120); reversed.stepWithControls(b,1.0f/120);
    }
    REQUIRE(first.finished()); REQUIRE(reversed.finished()); REQUIRE(first.contacts()==reversed.contacts());
    for (std::size_t i=0;i<3;++i) {
        REQUIRE(first.status(i)==reversed.status(2-i));
        REQUIRE(first.car(i).position().x==reversed.car(2-i).position().x);
        REQUIRE(first.car(i).position().y==reversed.car(2-i).position().y);
    }
}
TEST_CASE("Retired cars remain visible to traffic observations and the camera follows an active car", "[ml][race_session]") {
    const Track track{800,500,20}; const auto network=quietPolicy();
    RaceSession race{track,std::vector<Network>(3,network),settings(3),{{-6,0},{-14,0},{-20,4}}};
    const std::array<CarControls,3> controls{{{0,0,0},{1,0,0},{0,0,0}}};
    for (int tick=0;tick<1200 && race.status(0)==DriverStatus::Racing;++tick) race.stepWithControls(controls,1.0f/120);
    REQUIRE(race.status(0)==DriverStatus::CarContact); REQUIRE(race.status(1)==DriverStatus::CarContact);
    REQUIRE(race.status(2)==DriverStatus::Racing); REQUIRE(race.cameraTarget(0)==2);
    const auto input=race.observation(2);
    REQUIRE(input[8]==1); REQUIRE(input[13]==1); REQUIRE(input[18]==0);
    REQUIRE(input[11]==0); REQUIRE(input[16]==0);
}
TEST_CASE("A shared race validates full laps, orders finishers and never evolves policies", "[ml][race_session]") {
    const auto track=roundedCircuit(); const auto network=quietPolicy();
    RaceSession race{track,std::vector<Network>(2,network),settings(),{{-6,3},{-12,-3}}};
    for (int tick=0;tick<30000 && !race.finished();++tick) {
        const std::array<CarControls,2> controls{{followLane(track,race.car(0),3),followLane(track,race.car(1),-3)}};
        race.stepWithControls(controls,1.0f/120);
        REQUIRE_FALSE(carsOverlap(snapshotCar(race.car(0)),snapshotCar(race.car(1))));
    }
    REQUIRE(race.finished()); REQUIRE(race.contacts()==0);
    for (std::size_t i=0;i<2;++i) {
        REQUIRE(race.status(i)==DriverStatus::Finished); REQUIRE(race.lapCount(i)==2);
        REQUIRE(race.finishTime(i)>track.length()*2/50);
        REQUIRE_FALSE(race.onTrack(i)); REQUIRE(race.policy(i).parameters()==network.parameters());
        const auto input=race.observation(i); REQUIRE(input[8]==0);
    }
    const auto order=race.standings();
    REQUIRE(race.finishTime(order[0])<=race.finishTime(order[1]));
    REQUIRE(race.cameraTarget()==order[0]);
    const auto time=race.elapsed(); race.step(.1f); REQUIRE(race.elapsed()==time);
}
TEST_CASE("Race caps stop inactive and overlong sessions without fake finishes", "[ml][race_session]") {
    const Track track{800,500,20}; const auto network=quietPolicy();
    auto config=settings(); config.stalledAfter=.05f;
    RaceSession stalled{track,network,config}; stalled.step(.1f);
    REQUIRE(stalled.finished()); REQUIRE(stalled.status(0)==DriverStatus::Stalled); REQUIRE(stalled.lapCount(0)==0);
    config.maxDuration=.05f; config.stalledAfter=10;
    RaceSession timeout{track,network,config}; timeout.step(.1f);
    REQUIRE(timeout.finished()); REQUIRE(timeout.status(0)==DriverStatus::Timeout);
}
TEST_CASE("Invalid full-race policies, grids, settings and commands are rejected", "[ml][race_session]") {
    const Track track{800,500,20}; const auto network=quietPolicy();
    REQUIRE_THROWS_AS(RaceSession(track,quietPolicy(8),settings()),neuralError);
    auto invalid=settings(); invalid.cars=0; REQUIRE_THROWS_AS(RaceSession(track,network,invalid),carError);
    invalid=settings(); invalid.laps=0; REQUIRE_THROWS_AS(RaceSession(track,network,invalid),carError);
    REQUIRE_THROWS_AS(RaceSession(track,std::vector<Network>(2,network),settings(),{{-6,0},{-6,0}}),carError);
    RaceSession race{track,network,settings()};
    REQUIRE_THROWS_AS(race.step(0),carError); REQUIRE_THROWS_AS(race.step(2),carError);
    REQUIRE_THROWS_AS(race.observation(race.size()),std::out_of_range);
    RaceSession many{track,network,settings(20)};
    REQUIRE(many.observation(0)[18]==1);
    REQUIRE_THROWS_AS(many.observation(20),std::out_of_range);
    REQUIRE_THROWS_AS(race.stepWithControls({},.01f),carError);
    const std::array<CarControls,2> bad{{{std::numeric_limits<float>::quiet_NaN(),0,0},{}}};
    REQUIRE_THROWS_AS(race.stepWithControls(bad,.01f),carError);
}

TEST_CASE("A clean shared-world overtake updates standings and the active leader", "[ml][race_session]") {
    const Track track{800,500,20}; const auto network=quietPolicy();
    RaceSession race{track,std::vector<Network>(2,network),settings(),{{-6,3},{-18,-3}}};
    const std::array<CarControls,2> controls{{{.2f,0,0},{1,0,0}}};
    REQUIRE(race.standings().front()==0);
    for (int tick=0;tick<1200 && race.racePosition(1)<race.racePosition(0)+6;++tick)
        race.stepWithControls(controls,1.0f/120);
    REQUIRE(race.status(0)==DriverStatus::Racing); REQUIRE(race.status(1)==DriverStatus::Racing);
    REQUIRE(race.racePosition(1)>race.racePosition(0));
    REQUIRE(race.standings().front()==1); REQUIRE(race.cameraTarget(0)==1);
    REQUIRE(race.contacts()==0);
    REQUIRE_FALSE(carsOverlap(snapshotCar(race.car(0)),snapshotCar(race.car(1))));
}
