#pragma once

#include "Track/Aabb2D.hpp"
#include <cstddef>
#include <span>
#include <vector>

// A static index of primitive bounds. Leaves retain the original primitive
// indices, so building the tree never reorders the circuit's geometry.
class Bvh2D {
  public:
	// Primitive bounds must be finite and non-empty; an empty span clears the tree.
	void build(std::span<const Aabb2D> bounds);
	void clear() noexcept;

	// Overlapping leaves provide candidates. The predicate performs the exact
	// geometry test and stops traversal by returning true.
	template <class Predicate>
	[[nodiscard]] bool anyOverlapping(const Aabb2D& region, Predicate predicate) const {
		std::size_t index = 0;
		while (index < _nodes.size()) {
			const auto& node = _nodes[index];
			if (!node.bounds.overlaps(region)) {
				index = node.escape;
				continue;
			}
			for (std::size_t i = 0; i < node.count; ++i)
				if (predicate(_indices[node.first + i]))
					return true;
			++index;
		}
		return false;
	}

	template <class Visitor>
	void forEachOverlapping(const Aabb2D& region, Visitor visitor) const {
		(void)anyOverlapping(region, [&](std::size_t index) {
			visitor(index);
			return false;
		});
	}

  private:
	struct Node {
		Aabb2D bounds;
		std::size_t first = 0;
		std::size_t count = 0; // zero for an internal node
		std::size_t escape = 0; // first node after this subtree
	};
	std::vector<Node> _nodes;
	std::vector<std::size_t> _indices;
	void buildNode(std::span<const Aabb2D> bounds, std::size_t first, std::size_t count);
};
