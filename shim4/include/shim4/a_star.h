#ifndef NOO_A_STAR_H
#define NOO_A_STAR_H

#include "shim4/main.h"

namespace noo {

namespace gfx {
	class Tilemap;
}

namespace util {

class SHIM4_EXPORT A_Star {
public:
	struct Node {
		Node();

		Node *parent;
		Point<int> position;
		int cost_from_start;
		int cost_to_goal;
		int total_cost;
	};

	struct Way_Point {
		std::vector< util::Point<int> > to; // if clicking any of these
		std::vector< util::Point<int> > by; // go through these points first
		std::vector<bool> by_as_is; // just add by[x] to path, don't find path to it (can be empty)
		bool only_by;
		std::vector<std::pair<util::Point<int>, util::Point<int> > > click_swaps; // if this vector has anything in it, the above is ignored. 'first' is destination to change goal to if 'second' is the destination (goal)
	};

	A_Star(gfx::Tilemap *tilemap, std::vector< util::Rectangle<int> > entity_solids);
	~A_Star();

	std::list<Node> find_path(Point<int> start, Point<int> goal, bool check_solids = true, bool check_goal = true, bool allow_out_of_bounds = false);

private:
	std::list<Node> real_find_path(Point<int> start, Point<int> goal, bool check_solids = true, bool check_goal = true, bool allow_out_of_bounds = false);

	bool checkcoll(Point<int> pos, bool allow_out_of_bounds = false);
	Node *find_in_list(Point<int> position, std::list<Node *> &list);
	void remove_from_list(Node *node, std::list<Node *> &list);
	void add_to_open(Node *node);
	void destroy_nodes(std::list<Node *> &list);
	void branch(Node *node, Point<int> offset, Point<int> goal, bool check_solids = true, bool check_goal = true, bool allow_out_of_bounds = false);
	int heuristic(Point<int> start, Point<int> end);

	gfx::Tilemap *tilemap;
	std::vector< util::Rectangle<int> > entity_solids;

	Node *final_node;

	std::list<Node *> open;
	std::list<Node *> closed;

	std::vector<Way_Point> way_points;
};

} // End namespace util

} // End namespace noo

#endif // NOO_A_STAR_H
