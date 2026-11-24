#pragma once

#include "ML/Agent.hpp"
#include <optional>
#include <span>

// Follow the active race leader. Keep the current target on equal positions,
// and retain the last view when no agent remains active (e.g. a finished replay).
[[nodiscard]] inline std::optional<std::size_t> selectTrainingCameraTarget(
	std::span<const Agent> agents, std::optional<std::size_t> current = {}) {
	std::optional<std::size_t> leader;
	if (current && *current < agents.size() && agents[*current].isActive())
		leader = current;
	for (std::size_t i = 0; i < agents.size(); ++i) {
		if (agents[i].isActive() && (!leader || agents[i].racePosition() > agents[*leader].racePosition()))
			leader = i;
	}
	if (leader)
		return leader;
	if (current && *current < agents.size())
		return current;
	return agents.empty() ? std::nullopt : std::optional<std::size_t>{0};
}
