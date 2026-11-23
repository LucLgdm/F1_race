#include "Track/Bvh2D.hpp"
#include <algorithm>
#include <numeric>

namespace {
Vec2 center(const Aabb2D& bounds) {
	return {bounds.min.x + (bounds.max.x - bounds.min.x) * 0.5f,
			bounds.min.y + (bounds.max.y - bounds.min.y) * 0.5f};
}
} // namespace

void Bvh2D::clear() noexcept {
	_nodes.clear();
	_indices.clear();
}

void Bvh2D::build(std::span<const Aabb2D> bounds) {
	clear();
	if (bounds.empty())
		return;
	_indices.resize(bounds.size());
	std::iota(_indices.begin(), _indices.end(), std::size_t{0});
	_nodes.reserve(2 * bounds.size());
	buildNode(bounds, 0, bounds.size());
}

void Bvh2D::buildNode(std::span<const Aabb2D> bounds, std::size_t first, std::size_t count) {
	const std::size_t index = _nodes.size();
	_nodes.emplace_back();
	Aabb2D box, centers;
	for (std::size_t i = first; i < first + count; ++i) {
		const auto& primitive = bounds[_indices[i]];
		box.expand(primitive.min);
		box.expand(primitive.max);
		centers.expand(center(primitive));
	}
	_nodes[index].bounds = box;
	if (count <= 4) {
		_nodes[index].first = first;
		_nodes[index].count = count;
	} else {
		const bool splitX = centers.max.x - centers.min.x >= centers.max.y - centers.min.y;
		const std::size_t middle = first + count / 2;
		std::nth_element(_indices.begin() + first, _indices.begin() + middle,
						 _indices.begin() + first + count, [&](std::size_t a, std::size_t b) {
			const auto ca = center(bounds[a]), cb = center(bounds[b]);
			const float x = splitX ? ca.x : ca.y, y = splitX ? cb.x : cb.y;
			return x < y || (x == y && a < b);
		});
		buildNode(bounds, first, middle - first);
		buildNode(bounds, middle, first + count - middle);
	}
	// Use an index rather than a reference across recursive vector growth.
	_nodes[index].escape = _nodes.size();
}
