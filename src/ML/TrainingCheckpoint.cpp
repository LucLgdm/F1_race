#include "ML/TrainingCheckpoint.hpp"
#include "Track/TrackIO.hpp"
#include <fstream>
#include <sstream>

TrainingCheckpoint prepareTrainingCheckpoint(const Track& track,
    const std::filesystem::path& trackFile, DrivingMode mode, const Network* seed,
    const std::filesystem::path& seedFile, bool freshFromChrono) {
    if (freshFromChrono && (mode!=DrivingMode::Race || !seed || seed->topology().front()!=8))
        throw neuralError("A fresh overtaking session requires an explicit chrono policy");
    TrainingCheckpoint result;
    const auto stem=trackFile.stem().string()+(mode==DrivingMode::Race ? "_race" : "");
    auto path=trackFile.parent_path()/(stem+".f1");
    int suffix=1;
    while (std::filesystem::exists(path)) {
        if (!freshFromChrono) {
            try {
                std::ifstream input{path};
                auto saved=ChampionIO::read(input);
                std::ostringstream current,stored;
                TrackIO::write(current,track); TrackIO::write(stored,saved.track);
                if (saved.mode==mode && current.str()==stored.str()) {
                    result.record=std::move(saved.network); result.initial=result.record;
                    result.recordTime=saved.lapTime; result.origin=path;
                    break;
                }
            } catch (const std::exception&) { /* preserve unreadable or incompatible checkpoints */ }
        }
        path=trackFile.parent_path()/(stem+"_"+std::to_string(++suffix)+".f1");
    }
    result.destination=path;
    if (!result.initial && mode==DrivingMode::Race && seed) {
        result.transferredChrono=seed->topology().front()==8;
        result.initial=result.transferredChrono ? seed->expandedDrivingNetwork() : *seed;
        result.origin=seedFile;
    }
    return result;
}
