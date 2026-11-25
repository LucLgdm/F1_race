#include <catch2/catch_test_macros.hpp>
#include "ML/TrainingCheckpoint.hpp"
#include "ML/RaceObservation.hpp"
#include "ML/Agent.hpp"
#include <fstream>
#include <sstream>
#include <chrono>
#include <numbers>

namespace {
struct TemporaryCircuit {
    std::filesystem::path directory;
    TemporaryCircuit() {
        directory=std::filesystem::temp_directory_path()/
            ("f1_checkpoint_"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(directory);
    }
    ~TemporaryCircuit() { std::filesystem::remove_all(directory); }
};
Network policy(float throttleBias, std::size_t inputs=8, std::size_t width=16) {
    std::vector<Layer> layers;
    for (int layer=0;layer<3;++layer) {
        const auto count=layer==2 ? 3 : width;
        const auto previous=layer==0 ? inputs : width;
        std::vector<Neuron> neurons;
        for (std::size_t n=0;n<count;++n)
            neurons.emplace_back(std::vector<float>(previous,0),layer==2 && n==1 ? throttleBias : 0,Activation::Tanh);
        layers.emplace_back(std::move(neurons));
    }
    return Network{std::move(layers)};
}
void save(const std::filesystem::path& path,const Track& track,const Network& network,DrivingMode mode) {
    std::ofstream output{path}; ChampionIO::write(output,track,network,25,mode);
}
std::string contents(const std::filesystem::path& path) {
    std::ifstream input{path}; std::stringstream data; data<<input.rdbuf(); return data.str();
}
CarControls controls(const std::vector<float>& output) {
    return {std::clamp(output[1],0.0f,1.0f),std::clamp(output[2],0.0f,1.0f),std::clamp(output[0],-1.0f,1.0f)};
}
}
TEST_CASE("An explicit chrono transfer ignores an old slow race checkpoint and preserves both files", "[ml][champion]") {
    TemporaryCircuit files; const Track track{800,500,20};
    const auto chrono=policy(1),slow=policy(0,23,32);
    const auto chronoFile=files.directory/"Le_Mans.f1",raceFile=files.directory/"Le_Mans_race.f1";
    save(chronoFile,track,chrono,DrivingMode::Chrono); save(raceFile,track,slow,DrivingMode::Race);
    const auto original=contents(chronoFile),oldRace=contents(raceFile);
    const auto prepared=prepareTrainingCheckpoint(track,files.directory/"Le_Mans.track",DrivingMode::Race,&chrono,chronoFile,true);
    REQUIRE(prepared.initial.has_value()); REQUIRE_FALSE(prepared.record.has_value());
    REQUIRE(prepared.transferredChrono); REQUIRE(prepared.origin==chronoFile);
    REQUIRE(prepared.destination.filename()=="Le_Mans_race_2.f1");
    REQUIRE(prepared.initial->topology()==std::vector<std::size_t>{23,32,32,3});
    std::array<float,23> input{};
    REQUIRE(prepared.initial->forward(input)[1]>0.7f);
    REQUIRE(slow.forward(input)[1]==0);
    REQUIRE(contents(chronoFile)==original); REQUIRE(contents(raceFile)==oldRace);
    REQUIRE_FALSE(std::filesystem::exists(prepared.destination));
}
TEST_CASE("Resuming overtaking explicitly keeps the selected race checkpoint and its record", "[ml][champion]") {
    TemporaryCircuit files; const Track track{800,500,20}; const auto chrono=policy(1),race=policy(.2f,23,32);
    save(files.directory/"Le_Mans_race.f1",track,race,DrivingMode::Race);
    const auto prepared=prepareTrainingCheckpoint(track,files.directory/"Le_Mans.track",DrivingMode::Race,&chrono);
    REQUIRE(prepared.record.has_value()); REQUIRE(prepared.recordTime==25);
    REQUIRE_FALSE(prepared.transferredChrono);
    REQUIRE(prepared.initial->parameters()==race.parameters());
    REQUIRE(prepared.origin==prepared.destination);
}
TEST_CASE("A first explicit chrono transfer uses the standard race destination without creating a file", "[ml][champion]") {
    TemporaryCircuit files; const Track track{800,500,20}; const auto chrono=policy(1);
    const auto prepared=prepareTrainingCheckpoint(track,files.directory/"Le_Mans.track",DrivingMode::Race,&chrono,files.directory/"Le_Mans.f1",true);
    REQUIRE(prepared.destination.filename()=="Le_Mans_race.f1");
    REQUIRE_FALSE(std::filesystem::exists(prepared.destination));
    REQUIRE_THROWS_AS(prepareTrainingCheckpoint(track,files.directory/"Le_Mans.track",DrivingMode::Race,nullptr,{},true),neuralError);
}
TEST_CASE("Transferring 16 to 32 neurons preserves physical speed and every driving command", "[ml][champion]") {
    TemporaryCircuit files; const Track track{800,500,20}; const auto source=policy(1);
    const auto prepared=prepareTrainingCheckpoint(track,files.directory/"Le_Mans.track",DrivingMode::Race,&source,{},true);
    Simulation chrono{track,track.startPosition()},race{track,track.startPosition()};
    const std::array<CarSnapshot,1> traffic{{{{50,5},0,{8,0}}}};
    for (int tick=0;tick<240;++tick) {
        const auto expected=source.forward(chrono.observe().normalized(40,50));
        const auto input=observeTraffic(race.observe(),snapshotCar(race.car()),traffic);
        const auto actual=prepared.initial->forward(input);
        REQUIRE(actual==expected);
        REQUIRE_FALSE(chrono.step(controls(expected),1.0f/120));
        REQUIRE_FALSE(race.step(controls(actual),1.0f/120));
        REQUIRE(race.car().speed()==chrono.car().speed());
        REQUIRE(race.car().position().x==chrono.car().position().x);
        REQUIRE(race.car().position().y==chrono.car().position().y);
    }
    REQUIRE(race.car().speed()>10);
}
TEST_CASE("Chrono training stops after the first validated full lap", "[ml][training]") {
    std::vector<TrackSegment> pieces;
    TrackSegment segment{{0,0},0,1,true,30,-1};
    for (int i=0;i<4;++i) {
        pieces.push_back(segment);
        segment.start=Track::segmentPoint(segment,Track::segmentLength(segment));
        segment.heading-=std::numbers::pi_v<float>/2;
    }
    const Track track{pieces,12,15}; Agent agent{track,track.pointAt(-6)};
    for (int tick=0;tick<12000 && agent.isActive();++tick) {
        const auto& car=agent.car();
        const auto target=track.pointAt(track.progressAt(car.position())+6);
        const float dx=target.x-car.position().x,dy=target.y-car.position().y;
        const float error=std::remainder(std::atan2(dy,dx)-car.heading(),2*std::numbers::pi_v<float>);
        const float steer=std::clamp(-std::atan2(4.8f*std::sin(error),std::hypot(dx,dy))/.45f,-1.0f,1.0f);
        agent.advance({car.speed()<8 ? .5f : .125f,0,steer},1.0f/120);
    }
    REQUIRE_FALSE(agent.isActive()); REQUIRE(agent.lapCount()==1);
    REQUIRE(agent.lastLapTime()>0);
}
