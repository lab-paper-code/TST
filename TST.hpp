#ifndef TST_H_
#define TST_H_

#include <string>
#include <stack>
#include <tuple>
#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "s2/s2loop.h"
#include "s2/s2region_term_indexer.h"


namespace TST {

static const int POINTER_NULL_INT = -1;

/* Node Definition */
class NodeBase {
public:
    
};

class Node_T : public NodeBase {
public:
	int child[2];

	Node_T() {
		child[0] = child[1]	= POINTER_NULL_INT;
	}
};

class Linked_Node : public NodeBase { // The leaf of the temporal trie
public:
	unsigned int ENCODED_TIME;
	int child[8];

	struct {
		int prev; // past
		int next; // future
	};

	Linked_Node() : ENCODED_TIME(0) {
        prev = next = POINTER_NULL_INT;
		for (int i = 0; i < 8; ++i) {
            child[i] = POINTER_NULL_INT;
        }
    }

	bool operator<(const Linked_Node& other) const {
        return ENCODED_TIME < other.ENCODED_TIME;
    }
};

class Node_S : public NodeBase {
public:
	int child[4];

	Node_S() {
		child[0] = child[1] = child[2] = child[3] = POINTER_NULL_INT;
	}
};

template<class DATA>
class Data_Node : public NodeBase {
public:
	unsigned int ENCODED_TIME;
	unsigned long long S2_ID;
	std::vector<DATA>* data_vector_ptr;

	struct {
		int prev; // past
		int next; // future
	};

	Data_Node() {
		prev = next = POINTER_NULL_INT;
		data_vector_ptr = new std::vector<DATA>();
	}

	bool operator<(const Data_Node<DATA>& other) const {
        // Compare by ENCODED_TIME first
        if (ENCODED_TIME != other.ENCODED_TIME) {
            return ENCODED_TIME < other.ENCODED_TIME;
        }
        // If ENCODED_TIME is equal, compare by S2_ID
        return S2_ID < other.S2_ID;
    }

	 bool operator>(const Data_Node<DATA>& other) const {
        return other < *this;
    }

	/* For Data Vector */
	void insert_data(const DATA& data) {
        data_vector_ptr->push_back(data);
    }

	void get_data(std::vector<DATA>& retrieved_data_vector) {
		if (data_vector_ptr) {
            // Collect the queried data
            retrieved_data_vector.insert(
                retrieved_data_vector.end(),
                data_vector_ptr->begin(),
                data_vector_ptr->end()
            );
        }
        return;
    }

	size_t size() const {
		return data_vector_ptr ? data_vector_ptr->size() : 0;
	}
};

/* Tree Definition */
template<class DATA>
class TST {
private:
	static const int REF_YEAR = 2000;
	static const int ROOT_IDX = 0;
	int MAXCELL = 10000;
	int  D_TEMP_LEAF = 0, D_SPAT_LEAF = 0, D_INTER = 0;

	int temp_len;
	int spat_len;
	int total_len;
	int s2_level; // selected S2 level
	unsigned TEMP_INTER_IDX, TEMP_LEAF_IDX, SPAT_INTER_IDX, SPAT_LEAF_IDX;
	
	enum{LEFT_CHILD, RIGHT_CHILD}; // temporal child index
	enum{CHILD_ZERO, CHILD_ONE, CHILD_TWO, CHILD_THIRD, // spatial child index
		  CHILD_FOURTH, CHILD_FIFTH, CHILD_SIXTH, CHILD_SEVENTH};

	std::vector<Node_T> temp_internal;
	std::vector<Linked_Node> temp_leaf;
	std::vector<Node_S> spat_internal;
	std::vector<Data_Node<DATA>> spat_leaf;

	int trav_temp(unsigned int);
	void trav_spat(std::map<int, std::vector<unsigned long long>>, int, std::vector<DATA>&);

public:
	TST(); 
	TST(int, const std::string&);
	
	template<typename... Args>
	unsigned int time_encoder(Args...);
	unsigned long long space_encoder(double, double);
	void Insert(unsigned int, unsigned long long, DATA);
	void Delete(unsigned int, unsigned long long, DATA);

	std::map<int, std::vector<unsigned long long>> REC_S2_FINDER(std::vector<double>&, std::vector<double>&);
	void range_search(std::map<int, std::vector<unsigned long long>>&, 
									unsigned int, unsigned int, std::vector<DATA>& res);

	void setMaxCells(int); // Setter for max # of S2 cells
	int getInter_NodeCount() const; // Getter for # of Internal Nodes
	int getLeaf_NodeCount() const; // Getter for # of Leaf Nodes
	int getTotal_NodeCount() const; // Getter for # of Tree Nodes
	int getTemp_len() const; // Getter for Encoded Temporal Bit length
	int getSpat_len() const; // Getter for Encoded Spatial Bit length
	int getTotal_len() const; // Getter for Encoded Total Bit length
	int get_DataCount() const; // Getter for Total Data Count
	double get_size() const; // Getter for Index size
};


template<class DATA> // Minimum S2 Level is 1 and Time resolution is year
TST<DATA>::TST() : temp_len(6), spat_len(6), total_len(12), s2_level(1),
					TEMP_INTER_IDX(ROOT_IDX + 1), TEMP_LEAF_IDX(0), SPAT_INTER_IDX(0), SPAT_LEAF_IDX(0) {
	temp_internal.emplace_back(Node_T()); // Add ROOT Node
}

template<class DATA>
TST<DATA>::TST(int s2_res, const std::string& t_res){
	if (s2_res < 1 || s2_res > 30) {
        throw std::invalid_argument("Invalid spatial resolution (first argument). Must be between 1 and 30.");
    }
	
	s2_level = s2_res;
    spat_len = s2_level * 2 + 4;

    // Validate and assign temporal resolution using switch-like logic
    if (t_res == "year") {
        temp_len = 6;
    } else if (t_res == "month") {
        temp_len = 10;
    } else if (t_res == "day") {
        temp_len = 15;
    } else if (t_res == "hour") {
        temp_len = 20;
    } else if (t_res == "minute") {
        temp_len = 26;
    } else if (t_res == "second") {
        temp_len = 32;
    } else {
        throw std::invalid_argument("Invalid temporal resolution (second argument). Must be one of: year, month, day, hour, minute, second.");
    }
	total_len = temp_len + spat_len;

	temp_internal.emplace_back(Node_T()); // Add ROOT Node
	TEMP_INTER_IDX = ROOT_IDX + 1;
	TEMP_LEAF_IDX = SPAT_INTER_IDX = SPAT_LEAF_IDX = 0;
}

template<class DATA>
template<typename... Args>
unsigned int TST<DATA>::time_encoder(Args... args) {
    int expected_args = 0;
    const std::string arg_list[] = {"year", "month", "day", "hour", "minute", "second"};
	const int bit_lengths[] = {6, 4, 5, 5, 6, 6};

    switch (temp_len) {
        case 6:  expected_args = 1; break; // year
        case 10: expected_args = 2; break; // year, month
        case 15: expected_args = 3; break; // year, month, day
        case 20: expected_args = 4; break; // year, month, day, hour
        case 26: expected_args = 5; break; // year, month, day, hour, minute
        case 32: expected_args = 6; break; // year, month, day, hour, minute, second
        default:
            throw std::invalid_argument("Invalid temporal resolution. Cannot determine required arguments.");
    }

    // Validate the number of temporal arguments
    if (sizeof...(args) != expected_args) {
		std::string required_args;
        for (int i = 0; i < expected_args; ++i) {
            if (!required_args.empty()) {
                required_args += ", ";
            }
            required_args += arg_list[i];
        }

        throw std::invalid_argument(
            "Expected " + std::to_string(expected_args + 2) + " arguments (longitude, latitude, " + 
            required_args + "), but got " + std::to_string(sizeof...(args) + 2) + "."
        );
    }

	/* Temporal Encoding */
	unsigned int encoded_temporal = 0;
	int acc_length = 0;
    int idx = 0;

	([&](const auto& arg) {
        uint8_t bit_length = bit_lengths[idx];
        uint8_t value = static_cast<uint8_t>(arg);
		if(idx == 0) value = value - REF_YEAR; //  the reference year is subtracted
		acc_length += bit_length;

		// Convert value to binary string and keep only the required bits (temp_len bits)
		encoded_temporal += (value << (temp_len - acc_length));
        ++idx;
    }(args), ...);

	/* Return Encoded Temporal Information */
    return encoded_temporal;
}

template<class DATA>
unsigned long long TST<DATA>::space_encoder(double lat, double lng) {
	/* Spatial Encoding: S2Geometry */
	S2LatLng latlng = S2LatLng::FromDegrees(lat, lng);
	S2CellId cell_id = S2CellId(latlng);
	S2CellId selected_cell_id = cell_id.parent(s2_level);

	// Convert value to binary string and keep only the required bits (spat_len bits)
	unsigned long long encoded_spatial = selected_cell_id.id() >> (64 - spat_len);
	
	return encoded_spatial;
}

template<class DATA>
void TST<DATA>::Insert(unsigned int encoded_temp, unsigned long long encoded_spat, DATA data) {
	/* Temporal Node Insertion */
	int i, LAST_ITER, LAST_BIT, bit = 0;
	unsigned LAST_IDX, u = ROOT_IDX;

	// 1 - Search for the existence of a path in the trie with a time prefix.
	for(i = 1; i <= temp_len; i++){
		bit = (encoded_temp >> (temp_len - i)) & 1;
		if(temp_internal[u].child[bit] == POINTER_NULL_INT){
			LAST_ITER = i;
			LAST_BIT = bit;
			LAST_IDX = u;
			break;
		}
		u = temp_internal[u].child[bit];
	}

	// 2 - add path to time prefix
	if(i != temp_len+1){
		for(; i <= temp_len; i++){
			bit = (encoded_temp >> (temp_len - i)) & 1;
			if(i != temp_len){
				temp_internal[u].child[bit] = TEMP_INTER_IDX;
				temp_internal.emplace_back(Node_T());
				TEMP_INTER_IDX++;
			}
			else{
				temp_internal[u].child[bit] = TEMP_LEAF_IDX;
				temp_leaf.emplace_back(Linked_Node());
				TEMP_LEAF_IDX++;
			}
			u = temp_internal[u].child[bit];
		}

		temp_leaf[u].ENCODED_TIME = encoded_temp;

		// 3 - Update the node into the doubly linked list based on ENCODED_TIME
		switch (temp_leaf.size() - D_TEMP_LEAF) {
			case 1:
				break;
			case 2: {
				int FIRST_IDX = 0;
				if (temp_leaf[FIRST_IDX] < temp_leaf[u]) {
					temp_leaf[FIRST_IDX].next = u;
					temp_leaf[u].prev = FIRST_IDX;
				} else {
					temp_leaf[FIRST_IDX].prev = u;
					temp_leaf[u].next = FIRST_IDX;
				}
				break;
			}
			default: { // More than 2 nodes
				int PREV_IDX = POINTER_NULL_INT;
				int NEXT_IDX = POINTER_NULL_INT;
				unsigned v = u;

				if(LAST_BIT == 0){
					unsigned left_most_idx = temp_internal[LAST_IDX].child[1];
					for(int j = LAST_ITER + 1; j <= temp_len; j++){
						if(temp_internal[left_most_idx].child[0] != POINTER_NULL_INT)
							left_most_idx = temp_internal[left_most_idx].child[0];
						else
							left_most_idx = temp_internal[left_most_idx].child[1];
					}
					NEXT_IDX = left_most_idx;
					PREV_IDX = temp_leaf[NEXT_IDX].prev;
				}
				else{
					unsigned right_most_idx = temp_internal[LAST_IDX].child[0];
					for(int j = LAST_ITER + 1; j <= temp_len; j++){
						if(temp_internal[right_most_idx].child[1] != POINTER_NULL_INT)
							right_most_idx = temp_internal[right_most_idx].child[1];
						else
							right_most_idx = temp_internal[right_most_idx].child[0];
					}
					PREV_IDX = right_most_idx;
					NEXT_IDX = temp_leaf[PREV_IDX].next;
				}

				temp_leaf[v].prev = PREV_IDX;
				temp_leaf[v].next = NEXT_IDX;

				// Update prev node
				if(PREV_IDX != POINTER_NULL_INT){
					temp_leaf[PREV_IDX].next = v;
				}

				// Update next node
				if(NEXT_IDX != POINTER_NULL_INT){
					temp_leaf[NEXT_IDX].prev = v;
				}
				break;
			}
		}
	}

	// 4 - Search for the existence of a path in the trie with a space suffix.
	// 4-1 - Check the leading 3 bits.
	int lead_3bits = (encoded_spat >> (spat_len - 3)) & 0b111;
	LAST_ITER = -1;
	if(temp_leaf[u].child[lead_3bits] == POINTER_NULL_INT) {
		temp_leaf[u].child[lead_3bits] = SPAT_INTER_IDX;
		spat_internal.emplace_back(Node_S());
		SPAT_INTER_IDX++;
	}
	u = temp_leaf[u].child[lead_3bits];

	// 4-2 - Check in 2-bit increments
	for(i = 1; i <= s2_level; i++){
		bit = (encoded_spat >> (spat_len - 3 - 2*i)) & 0b11;
		if(spat_internal[u].child[bit] == POINTER_NULL_INT){
			if(i >= 2){
				LAST_ITER = i;
				LAST_BIT = bit;
				LAST_IDX = u;
			}
			break;
		}
		u = spat_internal[u].child[bit];
	}

	if(i != s2_level+1){
		for(; i <= s2_level; i++){
			bit = (encoded_spat >> (spat_len - 3 - 2*i)) & 0b11;
			if(i != s2_level){
				spat_internal[u].child[bit] = SPAT_INTER_IDX;
				spat_internal.emplace_back(Node_S());
				SPAT_INTER_IDX++;
				u = spat_internal[u].child[bit];
			}
			else{ // Leaf Node
				spat_internal[u].child[bit] = SPAT_LEAF_IDX;
				spat_leaf.emplace_back(Data_Node<DATA>());
				SPAT_LEAF_IDX++;
				u = spat_internal[u].child[bit];
			}
		}
		spat_leaf[u].ENCODED_TIME = encoded_temp;
		spat_leaf[u].S2_ID = encoded_spat;

		// 6 - Update the node into the doubly linked list based on TSC_ID
		switch (spat_leaf.size() - D_SPAT_LEAF) {
			case 1:
				break;
			case 2: {
				int FIRST_IDX = 0;
				if (spat_leaf[FIRST_IDX] < spat_leaf[u]) {
					spat_leaf[FIRST_IDX].next = u;
					spat_leaf[u].prev = FIRST_IDX;
				} else {
					spat_leaf[FIRST_IDX].prev = u;
					spat_leaf[u].next = FIRST_IDX;
				}
				break;
			}
			default: { // More than 2 nodes
				int PREV_IDX = POINTER_NULL_INT;
				int NEXT_IDX = POINTER_NULL_INT;
				int PIVOT = SPAT_LEAF_IDX - 2;
				unsigned v = u;

				if(LAST_ITER >= 2){					
					if(LAST_BIT == CHILD_ZERO){
						unsigned left_most_idx = LAST_IDX;
						if(spat_internal[left_most_idx].child[CHILD_ONE] != POINTER_NULL_INT)
							left_most_idx = spat_internal[left_most_idx].child[CHILD_ONE];
						else if(spat_internal[left_most_idx].child[CHILD_TWO] != POINTER_NULL_INT)
							left_most_idx = spat_internal[left_most_idx].child[CHILD_TWO];
						else
							left_most_idx = spat_internal[left_most_idx].child[CHILD_THIRD];

						// Check the Left-most node's idx on sub-trie
						for(int j = LAST_ITER + 1; j <= s2_level; j++){
							if(spat_internal[left_most_idx].child[CHILD_ZERO] != POINTER_NULL_INT)
								left_most_idx = spat_internal[left_most_idx].child[CHILD_ZERO];
							else if(spat_internal[left_most_idx].child[CHILD_ONE] != POINTER_NULL_INT)
								left_most_idx = spat_internal[left_most_idx].child[CHILD_ONE];
							else if(spat_internal[left_most_idx].child[CHILD_TWO] != POINTER_NULL_INT)
								left_most_idx = spat_internal[left_most_idx].child[CHILD_TWO];
							else
								left_most_idx = spat_internal[left_most_idx].child[CHILD_THIRD];
						}
						NEXT_IDX = left_most_idx;
						PREV_IDX = spat_leaf[NEXT_IDX].prev;
					}
					else if(LAST_BIT == CHILD_THIRD){
						unsigned right_most_idx = LAST_IDX;
						if(spat_internal[right_most_idx].child[CHILD_TWO] != POINTER_NULL_INT)
							right_most_idx = spat_internal[right_most_idx].child[CHILD_TWO];
						else if(spat_internal[right_most_idx].child[CHILD_ONE] != POINTER_NULL_INT)
							right_most_idx = spat_internal[right_most_idx].child[CHILD_ONE];
						else
							right_most_idx = spat_internal[right_most_idx].child[CHILD_ZERO];

						// Check the right-most node's idx on sub-trie
						for(int j = LAST_ITER + 1; j <= s2_level; j++){
							if(spat_internal[right_most_idx].child[CHILD_THIRD] != POINTER_NULL_INT)
								right_most_idx = spat_internal[right_most_idx].child[CHILD_THIRD];
							else if(spat_internal[right_most_idx].child[CHILD_TWO] != POINTER_NULL_INT)
								right_most_idx = spat_internal[right_most_idx].child[CHILD_TWO];
							else if(spat_internal[right_most_idx].child[CHILD_ONE] != POINTER_NULL_INT)
								right_most_idx = spat_internal[right_most_idx].child[CHILD_ONE];
							else
								right_most_idx = spat_internal[right_most_idx].child[CHILD_ZERO];
						}
						PREV_IDX = right_most_idx;
						NEXT_IDX = spat_leaf[PREV_IDX].next;
					}
					else{ /* CHILD_ONE, CHILD_TWO */
						// check the right side
						int way_flag = -1;
						unsigned left_most_idx = LAST_IDX;
						unsigned right_most_idx = LAST_IDX;
						for(int j = LAST_BIT + 1; j <= CHILD_THIRD; j++){
							if(spat_internal[left_most_idx].child[j] != POINTER_NULL_INT){
								left_most_idx = spat_internal[left_most_idx].child[j];
								way_flag = 1;
								break;
							}
						}

						if(way_flag > 0){
							for(int j = LAST_ITER + 1; j <= s2_level; j++){
								if(spat_internal[left_most_idx].child[CHILD_ZERO] != POINTER_NULL_INT)
									left_most_idx = spat_internal[left_most_idx].child[CHILD_ZERO];
								else if(spat_internal[left_most_idx].child[CHILD_ONE] != POINTER_NULL_INT)
									left_most_idx = spat_internal[left_most_idx].child[CHILD_ONE];
								else if(spat_internal[left_most_idx].child[CHILD_TWO] != POINTER_NULL_INT)
									left_most_idx = spat_internal[left_most_idx].child[CHILD_TWO];
								else
									left_most_idx = spat_internal[left_most_idx].child[CHILD_THIRD];
							}
							NEXT_IDX = left_most_idx;
							PREV_IDX = spat_leaf[NEXT_IDX].prev;
						}
						else {
							for(int j = LAST_BIT - 1; j >= CHILD_ZERO; j--){
								if(spat_internal[right_most_idx].child[j] != POINTER_NULL_INT){
									right_most_idx = spat_internal[right_most_idx].child[j];
									break;
								}
							}
							
							for(int j = LAST_ITER + 1; j <= s2_level; j++){
								if(spat_internal[right_most_idx].child[CHILD_THIRD] != POINTER_NULL_INT)
									right_most_idx = spat_internal[right_most_idx].child[CHILD_THIRD];
								else if(spat_internal[right_most_idx].child[CHILD_TWO] != POINTER_NULL_INT)
									right_most_idx = spat_internal[right_most_idx].child[CHILD_TWO];
								else if(spat_internal[right_most_idx].child[CHILD_ONE] != POINTER_NULL_INT)
									right_most_idx = spat_internal[right_most_idx].child[CHILD_ONE];
								else
									right_most_idx = spat_internal[right_most_idx].child[CHILD_ZERO];
							}
							PREV_IDX = right_most_idx;
							NEXT_IDX = spat_leaf[PREV_IDX].next;
						}
					}
				}
				else{
					// Find insertion point: Using LINKED-LIST
					if (spat_leaf[PIVOT] < spat_leaf[v]) {
						PREV_IDX = PIVOT;			
						while (PREV_IDX != POINTER_NULL_INT && spat_leaf[PREV_IDX] < spat_leaf[v]) {
							NEXT_IDX = spat_leaf[PREV_IDX].next;
							if (NEXT_IDX == POINTER_NULL_INT || spat_leaf[NEXT_IDX] > spat_leaf[v]) {
								break;
							}
							PREV_IDX = NEXT_IDX;
						}
					} else {
						NEXT_IDX = PIVOT;
						while (NEXT_IDX != POINTER_NULL_INT && spat_leaf[NEXT_IDX] > spat_leaf[v]) {
							PREV_IDX = spat_leaf[NEXT_IDX].prev;
							if (PREV_IDX == POINTER_NULL_INT || spat_leaf[PREV_IDX] < spat_leaf[v]) {
								break;
							}
							NEXT_IDX = PREV_IDX;
						}
					}
				}

				spat_leaf[v].prev = PREV_IDX;
				spat_leaf[v].next = NEXT_IDX;

				// Update previous node pointer
				if (PREV_IDX != POINTER_NULL_INT) {
					spat_leaf[PREV_IDX].next = v;
				}

				// Update next node pointer
				if (NEXT_IDX != POINTER_NULL_INT) {
					spat_leaf[NEXT_IDX].prev = v;
				}
				break;
			}
		}
	}

	// Data Pointing (Insert into data vector)
	spat_leaf[u].insert_data(data);

	return;
}

template<class DATA>
void TST<DATA>::Delete(unsigned int encoded_temp, unsigned long long encoded_spat, DATA data) {
	int i, bit;
	unsigned u = ROOT_IDX;
	std::stack<unsigned> path_idx;
	path_idx.push(ROOT_IDX);

	// 1 - Search for the existence of a path in the trie with a time prefix.
	for(i = 1; i <= temp_len; i++){
		bit = (encoded_temp >> (temp_len - i)) & 1;
		if(temp_internal[u].child[bit] == POINTER_NULL_INT){
			std::cerr << "[Warning] Does not exist in the temporal trie. Deletion skipped." << std::endl;
			return;
		}
		u = temp_internal[u].child[bit];
		path_idx.push(u);
	}
	

	// 2 - Search for the existence of a path in the trie with a space suffix.
	// 2-1 - Check the leading 3 bits.
	int lead_3bits = (encoded_spat >> (spat_len - 3)) & 0b111;
	if(temp_leaf[u].child[lead_3bits] == POINTER_NULL_INT) {
		std::cerr << "[Warning] Does not exist in the spatial trie. Deletion skipped." << std::endl;
		return;
	}
	u = temp_leaf[u].child[lead_3bits];
	path_idx.push(u);

	// 2-2 - Check in 2-bit increments
	for(i = 1; i <= s2_level; i++){
		bit = (encoded_spat >> (spat_len - 3 - 2*i)) & 0b11;
		if(spat_internal[u].child[bit] == POINTER_NULL_INT){
			std::cerr << "[Warning] Does not exist in the spatial trie. Deletion skipped." << std::endl;
			return;
		}
		u = spat_internal[u].child[bit];
		path_idx.push(u);
	}

	/* 3 -  Delete the actual data referenced by the node. */ 
	auto& vec = *(spat_leaf[u].data_vector_ptr);
	auto it = std::find(vec.begin(), vec.end(), data);
	if(it != vec.end()){
		vec.erase(it);
	}
	else{
		std::cerr << "[Warning] Leaf node does not reference a valid data. "
          			<< "Possible missing or null data. Deletion skipped." << std::endl;
		return;
	}

	// Idx Checker
	if(u != path_idx.top()){
		std::cerr << "[Warning] The final index does not match." << std::endl;
	}
	path_idx.pop();

	// 3-1 - Disable the spatial leaf node
	if(vec.empty()){
		int PREV_IDX = spat_leaf[u].prev;
		int NEXT_IDX = spat_leaf[u].next;

		// Detach target node from the doubly linked list
		spat_leaf[u].prev = POINTER_NULL_INT;
		spat_leaf[u].next = POINTER_NULL_INT;

		// Spatial Trie Leaf Node
		switch (spat_leaf.size() - D_SPAT_LEAF) {
			case 1:
				D_SPAT_LEAF++;
				break; // ROOT Free
			case 2: {
				D_SPAT_LEAF++;
				unsigned other = (PREV_IDX != POINTER_NULL_INT) ? PREV_IDX : NEXT_IDX;
				spat_leaf[other].prev = POINTER_NULL_INT;
            	spat_leaf[other].next = POINTER_NULL_INT;
				break;
			}
			default: {
				D_SPAT_LEAF++;
				if(PREV_IDX != POINTER_NULL_INT)
					spat_leaf[PREV_IDX].next = NEXT_IDX;
				if(NEXT_IDX != POINTER_NULL_INT)
					spat_leaf[NEXT_IDX].prev = PREV_IDX;
				break;
			}
		}
	}
	else // Do not need to deactivate the node
		return;


	// 3-2 - Check whether the spatial internal node (2-bits) should be disabled
	u = path_idx.top();
	for(i = s2_level; i >= 1; i--){
		bit = (encoded_spat >> (spat_len - 3 - 2*i)) & 0b11;
		spat_internal[u].child[bit] = POINTER_NULL_INT;

		for(int j = CHILD_ZERO; j <= CHILD_THIRD; j++){
			if(spat_internal[u].child[j] != POINTER_NULL_INT)
				return; // There are child nodes more than one 
		}
		D_INTER++;

		// If there is no more child node, then deactivate
		path_idx.pop();
		u = path_idx.top();
	}

	// 3-3 - Check whether the temporal leaf node should be disabled
	temp_leaf[u].child[lead_3bits] = POINTER_NULL_INT;
	for(int j = CHILD_ZERO; j <= CHILD_SEVENTH; j++){
		if(temp_leaf[u].child[j] != POINTER_NULL_INT)
			return;
	}

	// Detach temporal leaf node from the doubly linked list
	int PREV_IDX = temp_leaf[u].prev;
	int NEXT_IDX = temp_leaf[u].next;
	temp_leaf[u].prev = POINTER_NULL_INT;
	temp_leaf[u].next = POINTER_NULL_INT;

	switch (temp_leaf.size() - D_TEMP_LEAF) {
		case 1:
			D_TEMP_LEAF++;
			break; // ROOT Free
		case 2: {
			D_TEMP_LEAF++;
			unsigned other = (PREV_IDX != POINTER_NULL_INT) ? PREV_IDX : NEXT_IDX;
			temp_leaf[other].prev = POINTER_NULL_INT;
			temp_leaf[other].next = POINTER_NULL_INT;
			break;
		}
		default: {
			D_TEMP_LEAF++;
			if(PREV_IDX != POINTER_NULL_INT)
				temp_leaf[PREV_IDX].next = NEXT_IDX;
			if(NEXT_IDX != POINTER_NULL_INT)
				temp_leaf[NEXT_IDX].prev = PREV_IDX;
			break;
		}
	}

	// 3-4 - Check whether the temporal internal node (1-bit) should be disabled
	D_INTER--; // prevent duplicate calculation (D_TEMP_LEAF)
	for(i = temp_len; i >= 1; i--){
		path_idx.pop();
		u = path_idx.top();

		bit = (encoded_temp >> (temp_len - i)) & 1;
		temp_internal[u].child[bit] = POINTER_NULL_INT;
		
		if(temp_internal[u].child[1-bit] != POINTER_NULL_INT)
			return;
		D_INTER++;
	}

	return;
}

template<class DATA>
std::map<int, std::vector<unsigned long long>> TST<DATA>::REC_S2_FINDER(std::vector<double>& left_bottom, std::vector<double>& right_upper) {
	S2RegionCoverer::Options options;
	options.set_max_level(s2_level);
	options.set_max_cells(MAXCELL);
	S2RegionCoverer coverer(options);

	S2LatLngRect rect = S2LatLngRect::FromPointPair(S2LatLng::FromDegrees(left_bottom[0], left_bottom[1]), S2LatLng::FromDegrees(right_upper[0], right_upper[1]));
	S2CellUnion covering = coverer.GetCovering(rect);

	/* Identify the S2 Cells within the queried space */
	unsigned long long number_of_cells = covering.size();
	std::map<int, std::vector<unsigned long long>> levelMap; // S2 Cell level map

	for(unsigned long long i = 0; i < number_of_cells; i++){
		unsigned long long s2_search_id = covering.cell_id(i).id();
		int level = covering.cell_id(i).level();

		if (levelMap.find(level) == levelMap.end()) {
			levelMap[level] = std::vector<unsigned long long>{s2_search_id};
		} else {
			levelMap[level].push_back(s2_search_id);
		}
	}
	
	return levelMap;
}

template<class DATA>
void TST<DATA>::range_search(std::map<int, std::vector<unsigned long long>>& S2_LEVEL_MAP, 
									unsigned int encoded_start_time, unsigned int encoded_end_time, std::vector<DATA>& res) {
	// Finds the time node closest to the starting point
	int TIME_IDX = trav_temp(encoded_start_time);
	while(TIME_IDX != POINTER_NULL_INT && temp_leaf[TIME_IDX].ENCODED_TIME < encoded_end_time){
		trav_spat(S2_LEVEL_MAP, TIME_IDX, res);
		TIME_IDX = temp_leaf[TIME_IDX].next;
	}
	return;
}

template<class DATA>
int TST<DATA>::trav_temp(unsigned int encoded_start_time) { // Traverse on Temporal Trie
	int i, bit = 0;
	int u = ROOT_IDX;

	for(i = 1; i <= temp_len; i++){
		bit = (encoded_start_time >> (temp_len-i)) & 1;
		if(temp_internal[u].child[bit] == POINTER_NULL_INT) break;
		u = temp_internal[u].child[bit];
	}
	if(i == temp_len+1) return u;

	for(; i <= temp_len; i++){
		if(temp_internal[u].child[RIGHT_CHILD] != POINTER_NULL_INT)
			u = temp_internal[u].child[1];
		else
			u = temp_internal[u].child[0];
	}

	// reach to the doubly linked list
	int v = u;
    while (v != POINTER_NULL_INT) {
        if (temp_leaf[v].ENCODED_TIME > encoded_start_time) {
            int closest = v;
            while (temp_leaf[v].prev != POINTER_NULL_INT && temp_leaf[temp_leaf[v].prev].ENCODED_TIME > encoded_start_time) {
				v = temp_leaf[v].prev;
                closest = v;
            }
            return closest;
        }
		else{
			if(temp_leaf[v].next == POINTER_NULL_INT) break;
			else v = temp_leaf[v].next;
		}
    }

	return v;
}

template<class DATA>
void TST<DATA>::trav_spat(std::map<int, std::vector<unsigned long long>> LEVEL_MAP, int TIME_IDX, std::vector<DATA>& res) { // Traverse on Spatial Trie
	int j, bit, lead_3bits;
	unsigned long long s2;
	int u;

	for(const auto& LEVEL_S2_PAIR : LEVEL_MAP){
		int level = LEVEL_S2_PAIR.first;
		const std::vector<unsigned long long>& s2_values = LEVEL_S2_PAIR.second;

		for(unsigned i = 0; i < s2_values.size(); i++){
			s2 = s2_values[i] >> (64 - spat_len);
			u = TIME_IDX;
			lead_3bits = s2 >> (spat_len - 3) & 0b111;
			
			if(temp_leaf[u].child[lead_3bits] == POINTER_NULL_INT) continue; // NOT EXIST
			else u = temp_leaf[u].child[lead_3bits];

			for(j = 1; j <= level; j++){
				bit = (s2 >> (spat_len - 3 - 2*j)) & 0b11;
				if(spat_internal[u].child[bit] == POINTER_NULL_INT) break;
				u = spat_internal[u].child[bit];
			}
			
			if(j == level+1){ // Result Exist
				if(level != s2_level){
					unsigned left_most = u;
					for(int k=1; k <= s2_level-level; k++){ // Search the left most leaf node
						if(spat_internal[left_most].child[CHILD_ZERO] != POINTER_NULL_INT)
							left_most = spat_internal[left_most].child[CHILD_ZERO];
						else if(spat_internal[left_most].child[CHILD_ONE] != POINTER_NULL_INT)
							left_most = spat_internal[left_most].child[CHILD_ONE];
						else if(spat_internal[left_most].child[CHILD_TWO] != POINTER_NULL_INT)
							left_most = spat_internal[left_most].child[CHILD_TWO];
						else
							left_most = spat_internal[left_most].child[CHILD_THIRD];
					}
					unsigned right_most = u;
					for(int k=1; k <= s2_level-level; k++){ // Search the right most leaf node
						if(spat_internal[right_most].child[CHILD_THIRD] != POINTER_NULL_INT)
							right_most = spat_internal[right_most].child[CHILD_THIRD];
						else if(spat_internal[right_most].child[CHILD_TWO] != POINTER_NULL_INT)
							right_most = spat_internal[right_most].child[CHILD_TWO];
						else if(spat_internal[right_most].child[CHILD_ONE] != POINTER_NULL_INT)
							right_most = spat_internal[right_most].child[CHILD_ONE];
						else
							right_most = spat_internal[right_most].child[CHILD_ZERO];
					}

					if(left_most == right_most) spat_leaf[left_most].get_data(res);
					else{
						int trav = left_most;
						do {
							spat_leaf[trav].get_data(res);
							trav = spat_leaf[trav].next;
						} while(trav != spat_leaf[right_most].next);
					}
				}
				else{
					spat_leaf[u].get_data(res);
				}			
			}
		}
	}
}

template<class DATA>
void TST<DATA>::setMaxCells(int new_max) {
	// You can set the maximum number of S2 cells to search within the queried spatial range.
	MAXCELL = new_max;
	return;
}

template<class DATA>
int TST<DATA>::getInter_NodeCount() const {
	return temp_internal.size() + temp_leaf.size() +
			spat_internal.size() - D_TEMP_LEAF - D_INTER;
}

template<class DATA>
int TST<DATA>::getLeaf_NodeCount() const {
	return spat_leaf.size() - D_SPAT_LEAF;
}

template<class DATA>
int TST<DATA>::getTotal_NodeCount() const {
	return getInter_NodeCount() + getLeaf_NodeCount();
}

template<class DATA>
int TST<DATA>::get_DataCount() const {
	int total_size = 0;
    for(unsigned i = 0; i < spat_leaf.size(); i++){
        auto data = spat_leaf[i];
        total_size += data.size();
    }
	return total_size;
}

template<class DATA>
double TST<DATA>::get_size() const {
    size_t temp_internal_bytes = temp_internal.size() * sizeof(Node_T);
    size_t temp_leaf_bytes     = temp_leaf.size() * sizeof(Linked_Node);
    size_t spat_internal_bytes = spat_internal.size() * sizeof(Node_S);
    size_t spat_leaf_bytes     = spat_leaf.size() * sizeof(Data_Node<DATA>);

    size_t total_bytes = temp_internal_bytes + temp_leaf_bytes + 
                         spat_internal_bytes + spat_leaf_bytes;

    return total_bytes / (1024.0 * 1024.0);  // Convert to MB
}

template<class DATA>
int TST<DATA>::getTemp_len() const {
	return temp_len;
}

template<class DATA>
int TST<DATA>::getSpat_len() const {
	return spat_len;
}

template<class DATA>
int TST<DATA>::getTotal_len() const {
	return total_len;
}



}

#endif