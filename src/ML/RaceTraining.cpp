#include "ML/RaceTraining.hpp"
#include "ML/RaceSession.hpp"
#include <algorithm>
#include <cmath>

namespace {
Network raceNetwork() {
    return Network{{Layer{23,32,Activation::Tanh},Layer{32,32,Activation::Tanh},Layer{32,3,Activation::Tanh}}};
}
Vec2 offsetPoint(const Track& track, float coordinate, float lateral) {
    const auto p=track.pointAt(coordinate);
    const float h=track.headingAt(coordinate);
    return {p.x-std::sin(h)*lateral,p.y+std::cos(h)*lateral};
}
}
std::array<RaceScenario,5> raceScenarios(const Track& track) {
    const float lateral=std::max(0.0f,std::min(1.5f,track.roadWidth()/2.0f-2.75f));
    const float solo=std::min(120.0f,track.length()/3.0f);
    return {{{0,true,18,8,-lateral,solo},
             {0,true,24,12,lateral,solo},
             {0,true,16,10,0,solo},
             {track.length()*0.25f,true,22,8,-lateral,solo},
             {0,false,0,0,0,solo}}};
}
bool betterRaceScore(const RaceScore& a, const RaceScore& b) {
    if (a.successes!=b.successes) return a.successes>b.successes;
    if (a.startFitness!=b.startFitness) return a.startFitness>b.startFitness;
    if (a.progress!=b.progress) return a.progress>b.progress;
    return a.time<b.time;
}
RaceEpisode::RaceEpisode(const Track& track, RaceScenario scenario)
    : _track(track),_scenario(scenario),_simulation(track,track.pointAt(scenario.start)),
      _progress(track.length(),_simulation.location().coordinate) {
    if (scenario.traffic && carsOverlap(snapshotCar(car()),opponentAt(0)))
        throw carError("Race scenario starts with overlapping cars");
}
CarSnapshot RaceEpisode::opponentAt(float time) const {
    const float coordinate=_scenario.start+_scenario.gap+_scenario.opponentSpeed*time;
    const Vec2 p=offsetPoint(_track,coordinate,_scenario.lateralOffset);
    constexpr float epsilon=0.001f;
    const auto next=offsetPoint(_track,coordinate+_scenario.opponentSpeed*epsilon,_scenario.lateralOffset);
    return {p,_track.headingAt(coordinate),{(next.x-p.x)/epsilon,(next.y-p.y)/epsilon}};
}
std::optional<CarSnapshot> RaceEpisode::opponent() const {
    if (!_scenario.traffic) return {};
    return opponentAt(_elapsed);
}
CarControls RaceEpisode::decide(const Network& network) const {
    if (_end!=RaceEnd::Running) return {};
    const auto self=snapshotCar(car());
    const auto other=opponentAt(_elapsed);
    const auto inputs=observeTraffic(_simulation.observe(),self,
        _scenario.traffic ? std::span<const CarSnapshot>{&other,1} : std::span<const CarSnapshot>{});
    std::array<float,32> a{},b{};
    std::array<float,3> outputs{};
    network.forwardInto(inputs,outputs,a,b);
    return {std::clamp(outputs[1],0.0f,1.0f),std::clamp(outputs[2],0.0f,1.0f),std::clamp(outputs[0],-1.0f,1.0f)};
}
void RaceEpisode::advance(const CarControls& controls, float dt) {
    if (!std::isfinite(dt) || dt<=0.0f || dt>1.0f/120.0f+1e-6f)
        throw carError("Race episode requires a positive timestep no greater than 1/120 s");
    if (_end!=RaceEnd::Running) return;
    _lastControls=controls;
    const auto before=snapshotCar(car());
    Simulation predicted=_simulation;
    const bool wall=predicted.step(controls,dt);
    if (_scenario.traffic && carContact(before,snapshotCar(predicted.car()),opponentAt(_elapsed),opponentAt(_elapsed+dt))) {
        // Keep BOTH poses at the previous contact-free instant. Do not commit lap/progress state.
        _end=RaceEnd::CarContact;
        return;
    }
    _simulation.acceptState(predicted);
    _elapsed+=dt;
    _progress.update(_simulation.location().coordinate,dt);
    if (wall) { _end=RaceEnd::WallContact; return; }
    if (_scenario.traffic) {
        const float lead=_progress.progress()-(_scenario.gap+_scenario.opponentSpeed*_elapsed);
        _aheadTime=lead>=6.0f ? _aheadTime+dt : 0.0f;
        if (_aheadTime>=1.0f) _end=RaceEnd::Success;
    } else if (_progress.progress()>=_scenario.soloDistance) _end=RaceEnd::Success;
    if (_end==RaceEnd::Running) {
        if (_elapsed>=30.0f) _end=RaceEnd::Timeout;
        else if (_progress.hasStalled(4.0f)) _end=RaceEnd::Stalled;
    }
}
float RaceEpisode::progress() const {
    if (_end==RaceEnd::Success) return 1.0f;
    const float goal=_scenario.traffic ? _scenario.gap+_scenario.opponentSpeed*_elapsed+6.0f : _scenario.soloDistance;
    return std::clamp(_progress.bestProgress()/goal,0.0f,0.999f);
}
RaceTraining::RaceTraining(const Track& track, const Network* seed, std::size_t population)
    : _track(track),_scenarios(raceScenarios(track)) {
    if (population==0) throw neuralError("Race population must be positive");
    if (track.roadWidth()<8.0f) throw trackError("Overtaking training requires a track at least 8 metres wide");
    const auto initial=seed ? seed->expandedDrivingNetwork() : raceNetwork();
    _networks.reserve(population);
    for (std::size_t i=0;i<population;++i) _networks.push_back(seed ? initial : raceNetwork());
    _controls.resize(population);
    resetEpisodes();
}
void RaceTraining::resetEpisodes() {
    _episodes.clear();
    _scenarioIndices.assign(size(),0);
    _scores.assign(size(),{});
    for (std::size_t i=0;i<size();++i) _episodes.push_back(std::make_unique<RaceEpisode>(_track,_scenarios[0]));
}
bool RaceTraining::finished() const {
    return std::all_of(_scenarioIndices.begin(),_scenarioIndices.end(),[&](auto i){return i==_scenarios.size();});
}
std::size_t RaceTraining::activeCount() const {
    return std::count_if(_scenarioIndices.begin(),_scenarioIndices.end(),[&](auto i){return i<_scenarios.size();});
}
std::size_t RaceTraining::bestIndex() const {
    std::size_t best=0;
    for (std::size_t i=1;i<size();++i) if (betterRaceScore(_scores[i],_scores[best])) best=i;
    return best;
}
void RaceTraining::step(float dt) {
    if (!std::isfinite(dt) || dt<=0.0f || dt>1.0f) throw carError("Invalid race timestep");
    const int count=std::max(1,static_cast<int>(std::ceil(dt*120.0f)));
    const float step=dt/count;
    for (int tick=0;tick<count;++tick) {
        // Read-only decision phase. Each entry can become an independent CPU job / GPU lane.
        for (std::size_t i=0;i<size();++i) if (_scenarioIndices[i]<_scenarios.size())
            _controls[i]=_episodes[i]->decide(_networks[i]);
        // Independent world updates; no candidate can hit another candidate.
        for (std::size_t i=0;i<size();++i) if (_scenarioIndices[i]<_scenarios.size())
            _episodes[i]->advance(_controls[i],step);
        // Barrier: reduction and scenario resets only after all world updates.
        for (std::size_t i=0;i<size();++i) {
            if (_scenarioIndices[i]>=_scenarios.size() || _episodes[i]->end()==RaceEnd::Running) continue;
            const auto& episode=*_episodes[i];
            auto& score=_scores[i];
            score.successes+=episode.end()==RaceEnd::Success;
            score.carContacts+=episode.end()==RaceEnd::CarContact;
            score.wallContacts+=episode.end()==RaceEnd::WallContact;
            score.progress+=episode.progress(); score.time+=episode.elapsed();
            if (++_scenarioIndices[i]<_scenarios.size())
                _episodes[i]=std::make_unique<RaceEpisode>(_track,_scenarios[_scenarioIndices[i]]);
        }
    }
}
void RaceTraining::evaluateGridStarts(std::size_t cars) {
    if (!finished()) throw neuralError("Evaluate grid starts after the five exercises");
    RaceSettings settings;
    settings.cars=cars;
    for (std::size_t i=0;i<size();++i)
        if (_scores[i].successes==scenarioCount())
            _scores[i].startFitness=RaceSession{_track,_networks[i],settings}.startFitness();
}
void RaceTraining::evolve(std::mt19937& rng) {
    if (!finished()) throw neuralError("Cannot evolve before the race evaluation barrier");
    const Network parent=_networks[bestIndex()];
    for (std::size_t i=0;i<size();++i) {
        _networks[i]=parent;
        if (i>0) {
            const float p=i<=6 ? 0.05f : (i<=12 ? 0.1f : 0.2f);
            const float strength=i<=6 ? 0.03f : (i<=12 ? 0.1f : 0.3f);
            _networks[i].mutate(p,strength,rng);
        }
    }
    resetEpisodes();
}

std::size_t RaceTraining::displayIndex() const {
    for (std::size_t i=0;i<size();++i) if (_scenarioIndices[i]<_scenarios.size()) return i;
    return bestIndex();
}
