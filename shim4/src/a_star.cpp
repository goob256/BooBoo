#include "shim4/a_star.h"
#include "shim4/shim.h"
#include "shim4/tilemap.h"

using namespace noo;

namespace noo {

namespace util {

A_Star::Node::Node() :
	parent(NULL),
	cost_from_start(-1),
	cost_to_goal(-1),
	total_cost(-1)
{
}

A_Star::A_Star(gfx::Tilemap *tilemap, std::vector< util::Rectangle<int> > entity_solids) :
	tilemap(tilemap),
	entity_solids(entity_solids),
	final_node(0)
{
}

A_Star::~A_Star()
{
	delete final_node;
	destroy_nodes(open);
	destroy_nodes(closed);
}

std::list<A_Star::Node> A_Star::find_path(Point<int> start, Point<int> goal, bool check_solids, bool check_goal, bool allow_out_of_bounds)
{
	if (shim::get_way_points != nullptr) {
		way_points = shim::get_way_points(start);
	}

	bool done_click_swaps = false;
	for (auto wp : way_points) {
		for (auto p : wp.click_swaps) {
			if (p.second == goal) {
				goal = p.first;
				done_click_swaps = true;
				break;
			}
		}
		if (done_click_swaps) {
			break;
		}
	}

	std::vector< util::Point<int> > by;
	std::vector<bool> by_as_is;
	bool only_by = false;

	for (size_t i = 0; i < way_points.size(); i++) {
		Way_Point &wp = way_points[i];
		for (size_t j = 0; j < wp.to.size(); j++) {
			if (wp.to[j] == goal) {
				by = wp.by;
				by_as_is = wp.by_as_is;
				only_by = wp.only_by;
				break;
			}
		}
		if (by.size() > 0) {
			break;
		}
	}

	if (by.size() == 0) {
		return real_find_path(start, goal, check_solids, check_goal, allow_out_of_bounds);
	}
	else {
		util::Point<int> curr = start;
		std::list<Node> path;
		for (size_t i = 0; i < by.size(); i++) {
			if (by_as_is.size() > i && by_as_is[i]) {
				Node n;
				n.position = by[i];
				path.push_back(n);
			}
			else {
				std::list<Node> p2 = real_find_path(curr, by[i], check_solids, check_goal, allow_out_of_bounds);
				if (p2.size() == 0) {
					return p2;
				}
				path.insert(path.end(), p2.begin(), p2.end());
			}
			curr = by[i];
		}
		if (only_by == false) {
			std::list<Node> p2 = real_find_path(curr, goal, check_solids, check_goal, allow_out_of_bounds);
			if (p2.size() == 0) {
				return p2;
			}
			path.insert(path.end(), p2.begin(), p2.end());
		}
		return path;
	}
}

std::list<A_Star::Node> A_Star::real_find_path(Point<int> start, Point<int> goal, bool check_solids, bool check_goal, bool allow_out_of_bounds)
{
	delete final_node;
	final_node = 0;
	destroy_nodes(open);
	destroy_nodes(closed);
	
	// This allows clicking tiles that are solid but change to unsolid when walking on a different tile
	// (they wouldn't be put in Way_Point::to if you couldn't walk to them)
	if (check_goal) {
		for (size_t i = 0; i < way_points.size(); i++) {
			Way_Point &wp = way_points[i];
			for (size_t j = 0; j < wp.to.size(); j++) {
				if (wp.to[j] == goal) {
					check_goal = false;
					break;
				}
			}
			if (check_goal == false) {
				break;
			}
		}
	}
	if (check_goal) {
		for (size_t i = 0; i < way_points.size(); i++) {
			Way_Point &wp = way_points[i];
			for (size_t j = 0; j < wp.by.size(); j++) {
				if (wp.by[j] == goal) {
					check_goal = false;
					break;
				}
			}
			if (check_goal == false) {
				break;
			}
		}
	}

	if (check_goal && check_solids && checkcoll(goal, allow_out_of_bounds)) {
		// No path since the goal is solid
		return std::list<Node>();
	}

	Node *start_node = new Node();
	start_node->parent = 0;
	start_node->position = start;
	start_node->cost_from_start = 0;
	start_node->cost_to_goal = heuristic(start, goal);
	start_node->total_cost = start_node->cost_from_start + start_node->cost_to_goal;
	open.push_back(start_node);

	while (open.size() > 0) {
		Node *node = open.front();
		open.pop_front();

		if (node->position == goal) {
			final_node = node;
			std::list<Node> path;
			while (node->parent != 0) {
				Node n;
				n.parent = node->parent;
				n.position = node->position;
				n.cost_from_start = node->cost_from_start;
				n.cost_to_goal = node->cost_to_goal;
				n.total_cost = node->total_cost;
				path.push_front(n);
				node = node->parent;
			}
			return path;
		}
		else {
			branch(node, Point<int>(0, -1), goal, check_solids, check_goal, allow_out_of_bounds);
			branch(node, Point<int>(-1, 0), goal, check_solids, check_goal, allow_out_of_bounds);
			branch(node, Point<int>(1, 0), goal, check_solids, check_goal, allow_out_of_bounds);
			branch(node, Point<int>(0, 1), goal, check_solids, check_goal, allow_out_of_bounds);
		}
		closed.push_back(node);
	}

	return std::list<Node>(); // failed
}

A_Star::Node *A_Star::find_in_list(Point<int> position, std::list<Node *> &list)
{
	for (std::list<Node *>::iterator it = list.begin(); it != list.end(); it++) {
		Node *n = *it;
		if (n->position == position) {
			return n;
		}
	}

	return 0;
}

void A_Star::remove_from_list(Node *node, std::list<Node *> &list)
{
	std::list<Node *>::iterator it = std::find(list.begin(), list.end(), node);
	if (it != list.end()) {
		list.erase(it);
	}
}

void A_Star::add_to_open(Node *node)
{
	std::list<Node *>::iterator it;
	for (it = open.begin(); it != open.end(); it++) {
		Node *n = *it;
		if (n->total_cost >= node->total_cost) {
			break;
		}
	}
	open.insert(it, node);
}

void A_Star::destroy_nodes(std::list<Node *> &list)
{
	for (std::list<Node *>::iterator it = list.begin(); it != list.end();) {
		delete *it;
		it = list.erase(it);
	}
}

void A_Star::branch(Node *node, Point<int> offset, Point<int> goal, bool check_solids, bool check_goal, bool allow_out_of_bounds)
{
	Point<int> new_position = node->position + offset;
	if ((check_goal || new_position != goal) && check_solids && checkcoll(new_position, allow_out_of_bounds)) {
		return;
	}

	int new_cost = node->cost_from_start + 1;
	bool in_open = false;
	bool in_closed = false;
	Node *new_node = find_in_list(new_position, open);
	if (new_node == 0) {
		in_open = false;
		new_node = find_in_list(new_position, closed);
		if (new_node == 0) {
			in_closed = false;
		}
		else {
			in_closed = true;
		}
	}
	else {
		in_open = true;
		in_closed = find_in_list(new_position, closed) != 0;
	}
	if (new_node == 0 || new_node->cost_from_start > new_cost) {
		if (new_node == 0) {
			new_node = new Node();
			new_node->position = new_position;
		}
		new_node->parent = node;
		new_node->cost_from_start = new_cost;
		new_node->cost_to_goal = heuristic(new_node->position, goal);
		new_node->total_cost = new_node->cost_from_start + new_node->cost_to_goal;
		if (in_closed) {
			remove_from_list(new_node, closed);
		}
		else if (in_open) {
			remove_from_list(new_node, open);
			add_to_open(new_node);
		}
		else {
			add_to_open(new_node);
		}
	}
}

int A_Star::heuristic(Point<int> start, Point<int> end)
{
	return abs(start.x - end.x) + abs(start.y - end.y);
}

bool A_Star::checkcoll(util::Point<int> pos, bool allow_out_of_bounds)
{
	util::Size<int> size = tilemap->get_size();
	if (allow_out_of_bounds && (pos.x < 0 || pos.y < 0 || pos.x >= size.w || pos.y >= size.h)) {
		return false;
	}

	if (tilemap->is_solid(-1, pos)) {
		return true;
	}

	for (size_t i = 0; i < entity_solids.size(); i++) {
		if (entity_solids[i].contains(pos)) {
			return true;
		}
	}

	return false;
}

} // End namespace util

} // End namespace noo
