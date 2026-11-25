#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ML/RaceTraining.hpp"
#include "ML/DrivingMode.hpp"
#include "Track/ChampionIO.hpp"
#include <fstream>
#include <sstream>
#include <numbers>
#include <filesystem>
#include <type_traits>

using Catch::Approx;
namespace {
Network chronoPolicy() {
    return Network{{Layer{8,16,Activation::Tanh},Layer{16,16,Activation::Tanh},Layer{16,3,Activation::Tanh}}};
}
CarSnapshot pose(float x,float y,float h=0) { return {{x,y},h,{}}; }
}
TEST_CASE("Traffic observations use the local frame, nearest order and absent slots", "[ml][race]") {
    static_assert(std::is_trivially_copyable_v<CarSnapshot>);
    CarObservation road; road.speed=25; road.headingError=0;
    const auto self=pose(10,20,std::numbers::pi_v<float>/2);
    std::array<CarSnapshot,4> others{pose(10,30),pose(5,20),pose(10,100),pose(10,40)};
    others[0].velocity={0,10};
    const auto in=observeTraffic(road,self,others);
    REQUIRE(in[5]==Approx(.5)); REQUIRE(in[7]==Approx(1));
    REQUIRE(in[8]==1); REQUIRE(in[9]==Approx(0).margin(1e-6)); REQUIRE(in[10]==Approx(.125));
    REQUIRE(in[14]==Approx(.25)); REQUIRE(in[15]==Approx(0).margin(1e-6));
    REQUIRE(in[16]==Approx(.1));
    REQUIRE(in[19]==Approx(.5));
    const auto empty=observeTraffic(road,self,{});
    for (std::size_t i=8;i<23;++i) REQUIRE(empty[i]==0);
}
TEST_CASE("Traffic slots truncate to three and relative velocity accounts for ego motion", "[ml][race]") {
    CarObservation road;
    CarSnapshot self=pose(0,0); self.velocity={20,0};
    std::array<CarSnapshot,4> others{pose(8,0),pose(4,0),pose(12,0),pose(2,0)};
    others[3].velocity={10,0};
    const auto in=observeTraffic(road,self,others);
    REQUIRE(in[9]==Approx(.05)); REQUIRE(in[11]==Approx(-.1));
    REQUIRE(in[14]==Approx(.1)); REQUIRE(in[19]==Approx(.2));
}
TEST_CASE("Rectangular footprints allow close parallel cars and detect oriented contact", "[ml][race][collision]") {
    REQUIRE_FALSE(carsOverlap(pose(0,0),pose(0,3.01f)));
    REQUIRE(carsOverlap(pose(0,0),pose(0,3)));
    REQUIRE_FALSE(carsOverlap(pose(0,0),pose(4.01f,0)));
    REQUIRE(carsOverlap(pose(0,0),pose(3.4f,0,std::numbers::pi_v<float>/2)));
    REQUIRE_FALSE(carsOverlap(pose(0,0),pose(3.6f,0,std::numbers::pi_v<float>/2)));
    REQUIRE_THROWS_AS(carsOverlap(pose(0,0),pose(0,0),{-1,1}),carError);
}
TEST_CASE("Swept footprints catch tunnelling, opposing motion and rotation", "[ml][race][collision]") {
    const auto contact=carContact(pose(-10,0),pose(10,0),pose(0,0),pose(0,0));
    REQUIRE(contact.has_value()); REQUIRE(*contact==Approx(.3).margin(.0001));
    REQUIRE(carContact(pose(-10,0),pose(10,0),pose(10,0),pose(-10,0)));
    REQUIRE_FALSE(carContact(pose(-10,0),pose(10,0),pose(0,3.01f),pose(0,3.01f)));
    REQUIRE(carContact(pose(0,0),pose(0,0,std::numbers::pi_v<float>/2),pose(0,3.6f),pose(0,3.6f)));
    REQUIRE_FALSE(carContact(pose(0,0),pose(0,0),pose(0,4),pose(0,4)));
}
TEST_CASE("Expansion to 23-32-32-3 preserves old driving predictions", "[ml][race][network]") {
    const auto original=chronoPolicy();
    const auto expanded=original.expandedDrivingNetwork();
    REQUIRE(expanded.topology()==std::vector<std::size_t>{23,32,32,3});
    std::mt19937 rng{123}; std::uniform_real_distribution<float> random(-1,1);
    for (int trial=0;trial<40;++trial) {
        std::array<float,8> old{}; for (auto& v:old) v=random(rng);
        std::array<float,23> extended{}; for (auto& v:extended) v=random(rng);
        std::copy(old.begin(),old.end(),extended.begin());
        REQUIRE(expanded.forward(extended)==original.forward(old));
        std::array<float,32> a{},b{}; std::array<float,3> output{};
        expanded.forwardInto(extended,output,a,b);
        REQUIRE(std::vector<float>(output.begin(),output.end())==original.forward(old));
    }
    REQUIRE(expanded.expandedDrivingNetwork().parameters()==expanded.parameters());
}
TEST_CASE("Chrono wide layers and race mode roundtrip without changing track format", "[ml][race][champion]") {
    const Track track{800,500,20};
    const Network wide{{Layer{8,32,Activation::Tanh},Layer{32,32,Activation::Tanh},Layer{32,3,Activation::Tanh}}};
    std::stringstream chrono;
    ChampionIO::write(chrono,track,wide,42);
    REQUIRE(chrono.str().starts_with("F1_CHAMPION 2"));
    auto loaded=ChampionIO::read(chrono);
    REQUIRE(loaded.mode==DrivingMode::Chrono); REQUIRE(loaded.network.parameters()==wide.parameters());
    const auto race=chronoPolicy().expandedDrivingNetwork();
    std::stringstream data; ChampionIO::write(data,track,race,24,DrivingMode::Race);
    REQUIRE(data.str().starts_with("F1_CHAMPION 3"));
    const auto text=data.str();
    auto raceLoaded=ChampionIO::read(data);
    REQUIRE(raceLoaded.mode==DrivingMode::Race); REQUIRE(raceLoaded.network.parameters()==race.parameters());
    REQUIRE(raceLoaded.track.length()==track.length());
    std::stringstream invalid;
    REQUIRE_THROWS_AS(ChampionIO::write(invalid,track,race,24),IOError);
    auto wrong=text; wrong.replace(wrong.find("FOOTPRINT 2 1.5"),15,"FOOTPRINT 2 1.6");
    std::stringstream mismatch{wrong}; REQUIRE_THROWS_AS(ChampionIO::read(mismatch),IOError);
    wrong=text; wrong.replace(wrong.find("OBSERVATIONS 2"),14,"OBSERVATIONS 1");
    std::stringstream encoding{wrong}; REQUIRE_THROWS_AS(ChampionIO::read(encoding),IOError);
}
TEST_CASE("Existing repository champions remain readable and expandable", "[ml][race][champion]") {
    std::size_t count=0;
    for (const auto& entry:std::filesystem::directory_iterator(std::filesystem::path(F1_SOURCE_DIR)/"circuits")) {
        if (entry.path().extension()!=".f1") continue;
        std::ifstream input(entry.path()); const auto saved=ChampionIO::read(input);
        REQUIRE(saved.mode==DrivingMode::Chrono);
        REQUIRE(saved.network.expandedDrivingNetwork().topology()==std::vector<std::size_t>{23,32,32,3});
        ++count;
    }
    REQUIRE(count>=4);
}
TEST_CASE("Race contact rejects movement and preserves the last clear poses", "[ml][race][episode]") {
    const Track track{800,500,20};
    RaceEpisode episode{track,{0,true,18,0,0,100}};
    constexpr float dt=1.0f/120;
    Vec2 previous{};
    while (episode.end()==RaceEnd::Running) {
        previous=episode.car().position();
        episode.advance({1,0,0},dt);
    }
    REQUIRE(episode.end()==RaceEnd::CarContact);
    REQUIRE(episode.car().position().x==previous.x); REQUIRE(episode.car().position().y==previous.y);
    REQUIRE_FALSE(carsOverlap(snapshotCar(episode.car()),*episode.opponent()));
}
TEST_CASE("A controlled clean pass and a solo exercise can succeed", "[ml][race][episode]") {
    const Track track{800,500,20};
    RaceEpisode pass{track,{0,true,18,8,3.2f,100}};
    for (int i=0;i<3600 && pass.end()==RaceEnd::Running;++i) pass.advance({1,0,0},1.0f/120);
    REQUIRE(pass.end()==RaceEnd::Success); REQUIRE(pass.progress()==1);
    RaceEpisode solo{track,{0,false,0,0,0,40}};
    for (int i=0;i<3600 && solo.end()==RaceEnd::Running;++i) solo.advance({1,0,0},1.0f/120);
    REQUIRE(solo.end()==RaceEnd::Success);
}
TEST_CASE("Race scenarios are identical for each policy and preserve an elite at the barrier", "[ml][race][trainer]") {
    const Track track{800,500,20};
    const auto seed=chronoPolicy();
    RaceTraining trainer{track,&seed,2};
    std::mt19937 earlyRng{1};
    REQUIRE_THROWS_AS(trainer.evolve(earlyRng),neuralError);
    for (int i=0;i<18020 && !trainer.finished();++i) trainer.step(1.0f/120);
    REQUIRE(trainer.finished());
    REQUIRE(trainer.score(0).successes==trainer.score(1).successes);
    REQUIRE(trainer.score(0).progress==trainer.score(1).progress);
    REQUIRE(trainer.score(0).time==trainer.score(1).time);
    const auto parent=trainer.network(trainer.bestIndex()).parameters();
    std::mt19937 rng{1}; trainer.evolve(rng);
    REQUIRE_FALSE(trainer.finished()); REQUIRE(trainer.network(0).parameters()==parent);
    REQUIRE(trainer.network(1).parameters()!=parent);
    REQUIRE_THROWS_AS(trainer.step(0),carError);
}
TEST_CASE("Race ranking prioritizes successes, failed progress, then successful time", "[ml][race][selection]") {
    REQUIRE(betterRaceScore({1,0,0,1,20},{0,0,0,4,2}));
    REQUIRE(betterRaceScore({0,0,0,2,20},{0,0,0,1,2}));
    REQUIRE(betterRaceScore({5,0,0,5,20},{5,0,0,5,21}));
}

TEST_CASE("All default overtaking scenarios admit a clean controlled solution", "[ml][race][episode]") {
    const Track track{800,500,20};
    for (const auto& scenario : raceScenarios(track)) {
        RaceEpisode episode{track,scenario};
        for (int tick=0;tick<3602 && episode.end()==RaceEnd::Running;++tick) {
            const auto& car=episode.car();
            const auto location=track.locate(car.position());
            const float coordinate=location.coordinate+10.0f;
            const float h=track.headingAt(coordinate);
            const float offset=!scenario.traffic ? 0.0f : (scenario.lateralOffset<=0 ? 3.5f : -3.5f);
            auto target=track.pointAt(coordinate);
            target.x-=std::sin(h)*offset; target.y+=std::cos(h)*offset;
            const float dx=target.x-car.position().x,dy=target.y-car.position().y;
            const float error=std::remainder(std::atan2(dy,dx)-car.heading(),2*std::numbers::pi_v<float>);
            const float steering=-std::atan2(4.8f*std::sin(error),std::hypot(dx,dy))/.45f;
            episode.advance({car.speed()<20 ? .8f : .125f,0,std::clamp(steering,-1.0f,1.0f)},1.0f/120);
        }
        REQUIRE(episode.end()==RaceEnd::Success);
    }
}
