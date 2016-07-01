#include "avoidance/node.h"

namespace avoidance {

std::vector<Node> Node::getNeighbors() const {
  std::vector<Node> neighbors;
  neighbors.reserve(10);
	for (Cell neighborCell : cell.getNeighbors()) {
		neighbors.push_back(Node(neighborCell, cell));
	}
  return neighbors;
}

} // namespace avoidance
