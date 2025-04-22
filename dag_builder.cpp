// File: dag_builder.cpp (V3 - Ignores Control Flow)
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <set>
#include <list>
#include <memory>
#include <vector>
#include <tuple>
#include <algorithm> // For sort, replace
#include <cctype> // For isdigit

// --- Node Structure (same) ---
struct DagNode {
    std::string op; // Operation or initial identifier/literal
    std::shared_ptr<DagNode> left = nullptr;
    std::shared_ptr<DagNode> right = nullptr;
    std::list<std::string> labels; // Variables currently holding this node's value
    bool is_leaf = false;
    int node_id = -1; // Unique ID for DOT output

    // Constructor for leaves (variables/literals)
    DagNode(std::string initial_label_or_op) : op(std::move(initial_label_or_op)), is_leaf(true) {
        // If it's not a temporary/label, add it as an initial label
        if (!op.empty() && !(op[0] == 't' && op.length() > 1 && std::isdigit(op[1])) && !(op[0] == 'L' && op.length() > 1 && std::isdigit(op[1]))) {
             labels.push_back(op);
        }
    }
    // Constructor for internal nodes (operations/calls)
    DagNode(std::string o, std::shared_ptr<DagNode> l, std::shared_ptr<DagNode> r) : op(std::move(o)), left(std::move(l)), right(std::move(r)), is_leaf(false) {}

    // Comparison not strictly needed for vector storage but good practice
    bool operator<(const DagNode& other) const {
        // Simple comparison based on operation and children addresses for ordering if needed
        if (op != other.op) return op < other.op;
        if (left.get() != other.left.get()) return left.get() < other.left.get();
        return right.get() < other.right.get();
    }
};


// --- Function to read variable names (same) ---
std::set<std::string> readVariableNames(const std::string& filename) {
    std::set<std::string> vars;
    std::ifstream infile(filename);
    std::string line;
    if (!infile) { /* warning */ }
    else {
        while (std::getline(infile, line)) {
            if (line.empty() || line[0] == '#') continue;
            const std::string whitespace = " \t\n\r\f\v";
            size_t first = line.find_first_not_of(whitespace);
            if (first == std::string::npos) continue;
            size_t last = line.find_last_not_of(whitespace);
             if (last >= first) { vars.insert(line.substr(first, (last - first + 1))); }
        }
    }
    return vars;
}

// --- Function to read 3AC instructions (same) ---
std::vector<std::string> read3AC(const std::string& filename) {
    std::vector<std::string> code;
     std::ifstream infile(filename);
     std::string line;
      if (!infile) { /* error */ }
      else {
        while (std::getline(infile, line)) {
            if (line.empty() || line[0] == '#') continue; // Skip comments/empty
            const std::string whitespace = " \t\n\r\f\v";
            size_t first = line.find_first_not_of(whitespace);
            if (first == std::string::npos) continue;
            size_t last = line.find_last_not_of(whitespace);
             if (last >= first) { code.push_back(line.substr(first, (last - first + 1))); }
        }
    }
    return code;
}

// --- Function to build DAG and generate DOT output ---
void buildAndGenerateDot(const std::vector<std::string>& three_addr_code,
                         const std::set<std::string>& initial_variables,
                         std::vector<std::string>& dot_output)
{
    dot_output.push_back("digraph G {");
    dot_output.push_back("  rankdir=TB;");
    dot_output.push_back("  node [shape=box, fontname=Consolas, fontsize=10];");
    dot_output.push_back("  edge [fontname=Consolas, fontsize=9];");
    dot_output.push_back("");

    // Tracks the node representing the most recent value for each variable/temporary
    std::map<std::string, std::shared_ptr<DagNode>> current_node_map;
    // Stores all unique nodes created to avoid duplicates
    std::vector<std::shared_ptr<DagNode>> dag_nodes;
    // Map to find existing internal nodes: <op, left_child_ptr, right_child_ptr> -> node_index_in_dag_nodes
    std::map<std::tuple<std::string, std::shared_ptr<DagNode>, std::shared_ptr<DagNode>>, size_t> existing_op_nodes;
    int next_node_id = 0; // For DOT N# identifiers

    // Helper to get or create a leaf node (for variables or literals)
    auto get_or_create_leaf_node = [&](const std::string& name) -> std::shared_ptr<DagNode> {
        // If we already know the current node for this name, return it
        if (current_node_map.count(name)) {
            return current_node_map[name];
        }
        // Check if a leaf node with this name already exists anywhere in the DAG
        for (const auto& existing_node : dag_nodes) {
            if (existing_node->is_leaf && existing_node->op == name) {
                current_node_map[name] = existing_node; // Update map
                return existing_node;
            }
        }
        // Create a new leaf node
        auto new_node = std::make_shared<DagNode>(name);
        new_node->node_id = next_node_id++;
        dag_nodes.push_back(new_node);
        current_node_map[name] = new_node;
        return new_node;
    };

    // Initialize leaf nodes for all known variables from the start
    for (const auto& var : initial_variables) {
        get_or_create_leaf_node(var);
    }

    // Process 3AC instructions
    for (const std::string& instruction : three_addr_code) {
        std::stringstream ss(instruction);
        std::string part1, part2, part3, part4, part5;
        ss >> part1;

        // --- Skip Control Flow and Informational Instructions ---
        if (part1 == "ifFalse" || part1 == "goto" || part1 == "func" || part1.back() == ':') {
             // Labels end with ':', func begin/end are informational
             continue;
        }
        // Param is informational for DAG data flow, handle optionally later maybe
        if (part1 == "param") {
            // Ensure the parameter variable/temp exists as a node
             if(ss >> part2) get_or_create_leaf_node(part2);
            continue;
        }
        // Handle read/write placeholders (ensure variable exists)
        if (part1 == "read" || part1 == "write") {
             if(ss >> part2) get_or_create_leaf_node(part2);
            continue; // No data dependency edges created for these simple placeholders
        }

        // --- Process Computational and Assignment Instructions ---
        ss >> part2; // Should be '=' for assignments, or the value for return
        bool p3 = static_cast<bool>(ss >> part3); // op1 or function name
        bool p4 = static_cast<bool>(ss >> part4); // operator or param count
        bool p5 = static_cast<bool>(ss >> part5); // op2

        std::shared_ptr<DagNode> result_node = nullptr;
        std::string lhs = ""; // Variable being defined (if any)

        // 1. Return statement: return VALUE
        if (part1 == "return") {
            if (!part2.empty()) { // return t1, return 1 etc.
                result_node = get_or_create_leaf_node(part2); // Node already exists or is created
                // Optionally add a special 'return' label/node? Skipping for simplicity.
            } else {
                // return; (void return) - no node created/modified
            }
            continue; // Returns don't create new nodes in this simplified DAG model
        }
        // 2. Assignment statement: LHS = ...
        else if (!part1.empty() && part2 == "=") {
            lhs = part1; // The variable being assigned to

            // Case 2a: Assignment from function call: lhs = call func, N
            if (p3 && part3 == "call") {
                std::string func_name = p4 ? part4 : "unknown_func";
                // Calls always create a new node (side effects)
                result_node = std::make_shared<DagNode>("call " + func_name, nullptr, nullptr); // Treat call like an op node
                result_node->node_id = next_node_id++;
                dag_nodes.push_back(result_node);
                // We could try and find the preceding 'param' instructions and add dotted edges here
            }
            // Case 2b: Assignment with binary operation: lhs = op1 OP op2
            else if (p3 && p4 && p5) {
                std::string op1_name = part3;
                std::string op = part4;
                std::string op2_name = part5;

                std::shared_ptr<DagNode> node1 = get_or_create_leaf_node(op1_name);
                std::shared_ptr<DagNode> node2 = get_or_create_leaf_node(op2_name);

                // Check if this exact operation node already exists
                auto key = std::make_tuple(op, node1, node2);
                auto it = existing_op_nodes.find(key);
                if (it != existing_op_nodes.end()) {
                    result_node = dag_nodes[it->second]; // Reuse existing node
                } else {
                    // Create a new operation node
                    result_node = std::make_shared<DagNode>(op, node1, node2);
                    result_node->node_id = next_node_id++;
                    dag_nodes.push_back(result_node);
                    existing_op_nodes[key] = dag_nodes.size() - 1; // Store index
                }
            }
            // Case 2c: Simple assignment: lhs = op1
            else if (p3 && !p4) {
                 std::string op1_name = part3;
                 result_node = get_or_create_leaf_node(op1_name); // Just point to the existing node for op1
            }
            // Case 2d: Unhandled assignment form
            else {
                 std::cerr << "DAG Warning: Unhandled assignment form: " << instruction << std::endl;
                 continue;
            }
        }
        // 3. Unhandled Instruction Format
        else {
             std::cerr << "DAG Warning: Skipping unparsed 3AC instruction: " << instruction << std::endl;
             continue;
        }

        // --- Update Labels and Map ---
        if (!lhs.empty() && result_node != nullptr) {
            // Remove 'lhs' label from any node that currently has it
             if(current_node_map.count(lhs)) {
                 current_node_map[lhs]->labels.remove(lhs);
             }
            // Add 'lhs' label to the new result node (if not already present)
            bool found = false;
            for(const auto& lbl : result_node->labels) if(lbl == lhs) found=true;
            if (!found) result_node->labels.push_back(lhs);

            // Update the map: 'lhs' now points to this result node
            current_node_map[lhs] = result_node;
        }
    } // End processing 3AC

    // --- Generate DOT Output ---
    dot_output.push_back("  // Nodes");
    std::set<int> defined_node_ids; // Keep track of nodes already defined in DOT

    for (const auto& node : dag_nodes) {
         if(node->node_id < 0) continue; // Skip nodes that weren't properly assigned an ID (shouldn't happen)
         if(defined_node_ids.count(node->node_id)) continue; // Already defined

        std::stringstream node_def_ss;
        std::string label_str = node->op; // Start with the operation/leaf name

        // Sort and add variable labels associated with this node
        if (!node->labels.empty()) {
            label_str += "\\n["; // Newline before labels
            node->labels.sort(); // Consistent output order
            bool first_label = true;
            for (const auto& label : node->labels) {
                if (!first_label) label_str += ",";
                label_str += label; first_label = false;
            }
            label_str += "]";
        }
        // Escape quotes in the final label string for DOT
        std::replace(label_str.begin(), label_str.end(), '"', '\'');

        node_def_ss << "  N" << node->node_id << " [label=\"" << label_str << "\"];";
        dot_output.push_back(node_def_ss.str());
        defined_node_ids.insert(node->node_id);
    }

    dot_output.push_back("");
    dot_output.push_back("  // Edges");
    std::set<std::pair<int, int>> defined_edges; // Avoid duplicate edges

    for (const auto& node : dag_nodes) {
         if(node->node_id < 0) continue;

        // Add edges from children to parent (internal operation nodes)
        if (node->left && node->left->node_id >= 0) {
            auto edge = std::make_pair(node->left->node_id, node->node_id);
            if (defined_edges.find(edge) == defined_edges.end()) {
                dot_output.push_back("  N" + std::to_string(node->left->node_id) + " -> N" + std::to_string(node->node_id) + ";");
                defined_edges.insert(edge);
            }
        }
         if (node->right && node->right->node_id >= 0) {
             auto edge = std::make_pair(node->right->node_id, node->node_id);
             if (defined_edges.find(edge) == defined_edges.end()) {
                dot_output.push_back("  N" + std::to_string(node->right->node_id) + " -> N" + std::to_string(node->node_id) + ";");
                 defined_edges.insert(edge);
             }
         }
    }

    dot_output.push_back("}"); // End DOT graph definition
}

// --- Main Function ---
int main(int argc, char* argv[]) {
     if (argc != 3) { std::cerr << "Usage: dag_builder <3ac_input_file> <vars_input_file>\n"; return 1; }
    std::string tac_input_file = argv[1];
    std::string dag_vars_file = argv[2];
    std::string dag_output_file = "dag.dot"; // Output DOT format

    std::set<std::string> initial_vars = readVariableNames(dag_vars_file);
    std::vector<std::string> three_addr_code = read3AC(tac_input_file);
    if (three_addr_code.empty() && !std::ifstream(tac_input_file)) { std::cerr << "DAG Error: 3AC input file not found or empty.\n"; return 1; }
    else if (three_addr_code.empty()) { std::cout << "DAG Warning: 3AC input file is empty.\n"; }

    std::cout << "DAG: Building DAG from 3AC and variables..." << std::endl;
    std::vector<std::string> dot_representation;
    buildAndGenerateDot(three_addr_code, initial_vars, dot_representation);

    std::ofstream outfile(dag_output_file);
    if (!outfile) { std::cerr << "Error: Cannot open DAG output file: " << dag_output_file << std::endl; return 1; }
    if (dot_representation.size() <= 2) { // Only contains boilerplate
        outfile << "digraph G {}\n"; // Empty graph if no nodes/edges generated
        std::cout << "DAG Warning: Generated empty DAG (likely due to empty/unprocessed 3AC)." << std::endl;
    }
    else {
        for (const auto& line : dot_representation) outfile << line << std::endl;
        std::cout << "DAG: DOT generation complete. Written to " << dag_output_file << std::endl;
    }
    outfile.close();

    return 0;
}