#include "ML/RaceSession.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace {
void validate(RaceSettings settings) {
    if (settings.cars<1 || settings.cars>20 || settings.laps<1 || settings.laps>20 ||
        !std::isfinite(settings.maxDuration) || settings.maxDuration<0 ||
        !std::isfinite(settings.stalledAfter) || settings.stalledAfter<=0)
        throw carError("Invalid race settings (1-20 cars and laps, positive durations)");
}
std::vector<Network> copies(const Network& network, RaceSettings settings) {
    validate(settings);
    return std::vector<Network>(settings.cars,network);
}
}
RaceSession::RaceSession(const Track& track, const Network& policy, RaceSettings settings)
    : RaceSession(track,copies(policy,settings),settings) {}

RaceSession::RaceSession(const Track& track, std::vector<Network> policies, RaceSettings settings,
                         std::vector<RaceGridSlot> grid)
    : _track(track),_settings(settings),_policies(std::move(policies)),_grid(std::move(grid)) {
    validate(settings);
    if (_policies.size()!=settings.cars) throw neuralError("Each race entrant requires one policy");
    if (!track.issues().empty() || track.roadWidth()<8.0f)
        throw trackError("A full race requires a playable circuit at least 8 metres wide");
    for (const auto& network : _policies) {
        const auto shape=network.topology();
        if (shape.front()!=23 || shape.back()!=3 ||
            std::any_of(shape.begin()+1,shape.end(),[](auto width){return width>32;}))
            throw neuralError("Full races require a traffic policy with 23 inputs, 3 outputs and layers <=32");
    }
    if (_settings.maxDuration==0)
        _settings.maxDuration=std::max(120.0f,track.length()*settings.laps/5.0f+60.0f);
    if (_grid.empty()) {
        if (6.0f*settings.cars+4.0f>=track.length())
            throw trackError("Starting grid does not fit: reduce the number of cars");
        const float lateral=std::min(3.0f,track.roadWidth()/2.0f-2.55f);
        for (std::size_t i=0;i<settings.cars;++i)
            _grid.push_back({-6.0f-static_cast<float>(i)*6.0f,(i%2==0 ? lateral : -lateral)});
    }
    if (_grid.size()!=settings.cars) throw carError("Each race entrant requires one grid position");
    _cars.reserve(settings.cars); _predicted.reserve(settings.cars); _progress.reserve(settings.cars);
    for (const auto& slot : _grid) {
        if (!std::isfinite(slot.coordinate) || !std::isfinite(slot.lateral)) throw carError("Invalid race grid position");
        const auto p=track.pointAt(slot.coordinate);
        const float heading=track.headingAt(slot.coordinate);
        const Vec2 position{p.x-std::sin(heading)*slot.lateral,p.y+std::cos(heading)*slot.lateral};
        _cars.emplace_back(track,position);
        _predicted.push_back(_cars.back());
        _progress.emplace_back(track.length(),_cars.back().location().coordinate);
        for (std::size_t j=0;j+1<_cars.size();++j)
            if (carContact(snapshotCar(_cars[j].car()),snapshotCar(_cars[j].car()),
                           snapshotCar(_cars.back().car()),snapshotCar(_cars.back().car())))
                throw carError("Starting grid overlaps: reduce the number of cars or move the start line");
    }
    _status.assign(size(),DriverStatus::Racing); _finishTimes.assign(size(),0.0f);
    _before.resize(size()); _after.resize(size()); _inputs.resize(size()); _controls.resize(size());
    _blocked.resize(size()); _wall.resize(size()); _contact.resize(size()); _seenPairs.resize(size()*size());
}
float RaceSession::racePosition(std::size_t i) const {
    return _grid.at(i).coordinate+_progress.at(i).progress();
}
std::size_t RaceSession::activeCount() const {
    return std::count(_status.begin(),_status.end(),DriverStatus::Racing);
}
std::array<float,23> RaceSession::observation(std::size_t i) const {
    const auto& own=_cars.at(i); // validate before filling the bounded neighbour array
    std::array<CarSnapshot,19> neighbours{};
    std::size_t count=0;
    for (std::size_t j=0;j<size();++j)
        if (j!=i && onTrack(j)) neighbours[count++]=snapshotCar(car(j));
    return observeTraffic(own.observe(),snapshotCar(own.car()),
                          std::span<const CarSnapshot>{neighbours.data(),count});
}
void RaceSession::step(float dt) { advance(dt,{}); }
void RaceSession::stepWithControls(std::span<const CarControls> controls, float dt) {
    if (controls.size()!=size()) throw carError("Each race entrant requires one command");
    for (const auto& c : controls)
        if (!std::isfinite(c.throttle) || !std::isfinite(c.brake) || !std::isfinite(c.steering))
            throw carError("Race commands must be finite");
    advance(dt,controls);
}
void RaceSession::advance(float dt, std::span<const CarControls> controls) {
    if (!std::isfinite(dt) || dt<=0 || dt>1) throw carError("Invalid race timestep");
    const int ticks=std::max(1,static_cast<int>(std::ceil(dt*120.0f)));
    const float step=dt/ticks;
    for (int t=0;t<ticks && !finished();++t) tick(step,controls);
}
void RaceSession::tick(float dt, std::span<const CarControls> controls) {
    // 1. Immutable world snapshots and observations, including stopped retirements.
    for (std::size_t i=0;i<size();++i) _before[i]=snapshotCar(car(i));
    for (std::size_t i=0;i<size();++i) if (_status[i]==DriverStatus::Racing) _inputs[i]=observation(i);
    // 2. Policy jobs. No shared workspace, random generator or rendering operations.
    for (std::size_t i=0;i<size();++i) if (_status[i]==DriverStatus::Racing) {
        if (!controls.empty()) { _controls[i]=controls[i]; continue; }
        std::array<float,32> a{},b{}; std::array<float,3> out{};
        _policies[i].forwardInto(_inputs[i],out,a,b);
        _controls[i]={std::clamp(out[1],0.0f,1.0f),std::clamp(out[2],0.0f,1.0f),std::clamp(out[0],-1.0f,1.0f)};
    }
    // 3. Prediction jobs write only their assigned simulation.
    std::fill(_blocked.begin(),_blocked.end(),0); std::fill(_wall.begin(),_wall.end(),0);
    std::fill(_contact.begin(),_contact.end(),0); std::fill(_seenPairs.begin(),_seenPairs.end(),0);
    for (std::size_t i=0;i<size();++i) {
        _predicted[i].acceptState(_cars[i]);
        if (_status[i]==DriverStatus::Racing) {
            _wall[i]=_predicted[i].step(_controls[i],dt);
            _blocked[i]=_wall[i];
        }
    }
    // 4. Central collision barrier. Recheck after rollbacks: a third car must not
    // enter the previous pose of a car whose movement has just been rejected.
    // Block flags only grow, so at most N+1 passes reach a fixed point.
    for (std::size_t pass=0;pass<=size();++pass) {
        for (std::size_t i=0;i<size();++i)
            _after[i]=_blocked[i] ? _before[i] : snapshotCar(_predicted[i].car());
        bool changed=false;
        for (std::size_t i=0;i<size();++i) if (onTrack(i))
            for (std::size_t j=i+1;j<size();++j) if (onTrack(j)) {
                if (_status[i]!=DriverStatus::Racing && _status[j]!=DriverStatus::Racing) continue;
                if (!carContact(_before[i],_after[i],_before[j],_after[j])) continue;
                if (!_seenPairs[i*size()+j]) { ++_contacts; _seenPairs[i*size()+j]=1; }
                for (const auto k : {i,j}) if (_status[k]==DriverStatus::Racing) {
                    if (!_blocked[k]) changed=true;
                    _blocked[k]=1; _contact[k]=1;
                }
            }
        if (!changed) break;
    }
    // 5. Commit only validated motions, then update the race clock and classification.
    for (std::size_t i=0;i<size();++i) if (_status[i]==DriverStatus::Racing) {
        if (_blocked[i]) {
            _cars[i].stop();
            _status[i]=_contact[i] ? DriverStatus::CarContact : DriverStatus::WallContact;
            continue;
        }
        _cars[i].acceptState(_predicted[i]);
        _progress[i].update(_cars[i].location().coordinate,dt);
        if (_cars[i].lapCount()>=_settings.laps) {
            _finishTimes[i]=_elapsed+dt-_cars[i].timePerTrack();
            _status[i]=DriverStatus::Finished;
        } else if (_progress[i].hasStalled(_settings.stalledAfter)) {
            _cars[i].stop(); _status[i]=DriverStatus::Stalled;
        }
    }
    _elapsed+=dt;
    if (_elapsed>=_settings.maxDuration)
        for (std::size_t i=0;i<size();++i) if (_status[i]==DriverStatus::Racing) {
            _cars[i].stop(); _status[i]=DriverStatus::Timeout;
        }
}
std::vector<std::size_t> RaceSession::standings() const {
    std::vector<std::size_t> order(size()); std::iota(order.begin(),order.end(),0);
    std::stable_sort(order.begin(),order.end(),[&](std::size_t a,std::size_t b) {
        const bool af=_status[a]==DriverStatus::Finished,bf=_status[b]==DriverStatus::Finished;
        if (af!=bf) return af;
        if (af) return _finishTimes[a]<_finishTimes[b];
        return racePosition(a)>racePosition(b);
    });
    return order;
}
std::optional<std::size_t> RaceSession::cameraTarget(std::optional<std::size_t> current) const {
    std::optional<std::size_t> leader;
    if (current && *current<size() && status(*current)==DriverStatus::Racing) leader=current;
    for (std::size_t i=0;i<size();++i)
        if (status(i)==DriverStatus::Racing && (!leader || racePosition(i)>racePosition(*leader))) leader=i;
    if (leader) return leader;
    return standings().front();
}
