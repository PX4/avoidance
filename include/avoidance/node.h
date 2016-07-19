#ifndef GLOBAL_PLANNER_NODE
#define GLOBAL_PLANNER_NODE

#include <string>

#include "avoidance/cell.h"
#include "avoidance/common.h"

namespace avoidance {
  
class Node {
 public:
  Node() = default;
  Node(const Cell & cell, const Cell & parent) : cell_(cell), parent_(parent) {}

  virtual bool isEqual(const Node & other) const;
  virtual bool isSmaller(const Node & other) const;
  virtual std::size_t hash() const;
  virtual std::shared_ptr<Node> nextNode(const Cell & nextCell) const;
  virtual std::vector<std::shared_ptr<Node> > getNeighbors() const;
  virtual double getRotation(const Node & other) const;
  virtual double getXYRotation(const Node & other) const;
  std::string asString() const;

  Cell cell_;
  Cell parent_;
};

bool operator==(const Node & lhs, const Node & rhs) {return lhs.isEqual(rhs);}
bool operator< (const Node & lhs, const Node & rhs) {return lhs.isSmaller(rhs);}
bool operator!=(const Node & lhs, const Node & rhs) {return !operator==(lhs, rhs);}
bool operator> (const Node & lhs, const Node & rhs) {return  operator< (rhs, lhs);}
bool operator<=(const Node & lhs, const Node & rhs) {return !operator> (lhs, rhs);}
bool operator>=(const Node & lhs, const Node & rhs) {return !operator< (lhs, rhs);}

typedef std::shared_ptr<Node> NodePtr;
typedef std::pair<Node, double> NodeDistancePair;
typedef std::pair<NodePtr, double> PointerNodeDistancePair;

class CompareDist {
 public:
  bool operator()(const CellDistancePair & n1, const CellDistancePair & n2) {
    return n1.second > n2.second;
  }
  bool operator()(const NodeDistancePair & n1, const NodeDistancePair & n2) {
    return n1.second > n2.second;
  }
  bool operator()(const PointerNodeDistancePair & n1, const PointerNodeDistancePair & n2) {
    return n1.second > n2.second;
  }
};

class NodeWithoutSmooth : public Node {
 public:
  NodeWithoutSmooth() = default;
  NodeWithoutSmooth(const Cell & cell, const Cell & parent) : Node(cell, parent) {}

  bool isEqual(const Node & other) const {
    return cell_ == other.cell_;
  }

  std::size_t hash() const {
    return std::hash<avoidance::Cell>()(cell_);
  }

  NodePtr nextNode(const Cell & nextCell) const {
    return NodePtr(new NodeWithoutSmooth(nextCell, cell_));
  }

  double getRotation(const Node & other) const {
    return 0.0;
  }
};

struct HashNodePtr {
    std::size_t operator()(const std::shared_ptr<Node> & node_ptr) const
    {
        return node_ptr->hash();
    }
};

struct EqualsNodePtr {
    std::size_t operator()(const std::shared_ptr<Node> & node_ptr1, const std::shared_ptr<Node> & node_ptr2) const
    {
        return node_ptr1->isEqual(*node_ptr2);
    }
};

} // namespace avoidance

namespace std {

template <>
struct hash<avoidance::Node> {
  std::size_t operator()(const avoidance::Node & node) const {
    return node.hash();
  }
};

template <>
struct hash<avoidance::NodeWithoutSmooth> {
  std::size_t operator()(const avoidance::NodeWithoutSmooth & node) const {
    return node.hash();
  }
};

} // namespace std

#endif // GLOBAL_PLANNER_NODE
