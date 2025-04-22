#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <set>
#include <algorithm>
#include <stdexcept>

// --- Token Struct (same) ---
struct Token {
    std::string type_str;
    std::string lexeme;
    int line_num = 0;
    Token(std::string t = "", std::string l = "", int ln = 0) : type_str(std::move(t)), lexeme(std::move(l)), line_num(ln) {}
};

// --- Function to Parse Lexer Output File (same) ---
std::vector<Token> parseLexerOutputFileWithLines(const std::string& filename) {
    std::vector<Token> tokens;
    std::ifstream infile(filename);
    if (!infile) { /* error */ return tokens; }
    std::string line;
    std::getline(infile, line); std::getline(infile, line);
    int physical_line_counter = 3;
    auto trim = [](const std::string& str) -> std::string { /* ... same trim ... */
        const std::string whitespace = " \t\n\r\f\v";
        size_t first = str.find_first_not_of(whitespace);
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(whitespace);
        if (last >= first) { return str.substr(first, (last - first + 1)); }
        else { return ""; }
    };
    while (std::getline(infile, line)) {
        if (line.empty() || line.find_first_not_of(" \t\n\r") == std::string::npos) { physical_line_counter++; continue; }
        size_t first_pipe = line.find('|');
        size_t second_pipe = (first_pipe == std::string::npos) ? std::string::npos : line.find('|', first_pipe + 1);
        if (first_pipe != std::string::npos && second_pipe != std::string::npos) {
            std::string num_part_str = line.substr(0, first_pipe);
            std::string type_str = trim(line.substr(first_pipe + 1, second_pipe - first_pipe - 1));
            std::string lexeme_str = trim(line.substr(second_pipe + 1));
            int token_line_num = 0; try { if(!trim(num_part_str).empty()) token_line_num = std::stoi(trim(num_part_str)); } catch(...) {}
            if (type_str == "END_OF_FILE") { break; }
            else if (!type_str.empty()) { tokens.emplace_back(type_str, lexeme_str, token_line_num); }
            else { /* warning */ }
        } else { /* warning */ }
        physical_line_counter++;
    }
    return tokens;
}

// --- 3AC Generator State (same) ---
int temp_count = 0;
int label_count = 0;
std::string newTemp() { return "t" + std::to_string(temp_count++); }
std::string newLabel() { return "L" + std::to_string(label_count++); }

// --- Helper to find end of a simple statement (ends with ;) or block ({}) ---
size_t findEndOfStatementOrBlock(const std::vector<Token>& tokens, size_t start_index) {
     if (start_index >= tokens.size()) return start_index;

     if (tokens[start_index].lexeme == "{") {
         int brace_level = 1;
         size_t current = start_index + 1;
         while (current < tokens.size()) {
             if (tokens[current].lexeme == "{") brace_level++;
             else if (tokens[current].lexeme == "}") brace_level--;
             if (brace_level == 0) return current; // Return index of closing brace
             current++;
         }
         return tokens.size() -1; // Error case: closing brace not found
     } else {
         // Find next semicolon
         size_t current = start_index;
         while (current < tokens.size()) {
             if (tokens[current].lexeme == ";") return current;
              // Stop early if we hit constructs that clearly aren't part of the simple statement
              if (tokens[current].lexeme == "{" || tokens[current].lexeme == "}") break;
             current++;
         }
         return current-1; // Return index before the terminating token (or last token if ';' not found)
     }
 }


// --- Forward Declaration ---

size_t generate3ACRecursive(const std::vector<Token>& tokens, size_t i, std::vector<std::string>& three_addr_code, std::set<std::string>& variables); // <-- No bool here


// --- Process a sequence of tokens ---
// Returns the index *after* the last processed token in the sequence
// --- Process a sequence of tokens ---
// (Function signature might still have bool, that's ok for now if unused)
size_t processTokenSequence(const std::vector<Token>& tokens, size_t start_idx, size_t end_idx, std::vector<std::string>& three_addr_code, std::set<std::string>& variables, bool inside_if_else = false) {
    size_t current_idx = start_idx;
    while (current_idx <= end_idx && current_idx < tokens.size()) {
        // Call generate3ACRecursive WITHOUT the boolean argument
        current_idx = generate3ACRecursive(tokens, current_idx, three_addr_code, variables);
    }
    return current_idx;
}

// --- Main 3AC Generator Function (V9) ---
// Returns the index of the *next* token to process after handling the current construct
size_t generate3ACRecursive(const std::vector<Token>& tokens, size_t i, std::vector<std::string>& three_addr_code, std::set<std::string>& variables) {
    if (i >= tokens.size()) return tokens.size();

    const Token& token = tokens[i];
    auto is_safe = [&](size_t offset) { return (i + offset) < tokens.size(); };
    // std::cerr << "generate3ACRecursive processing index " << i << " ('" << token.lexeme << "')" << std::endl; // Debug

    // --- START: Explicit Preamble Skipping ---
    // Skip common directives/keywords FIRST before checking for functions etc.
    // using namespace std ;
    if (token.lexeme == "using" && is_safe(3) && tokens[i+1].lexeme == "namespace" && tokens[i+2].lexeme == "std" && tokens[i+3].lexeme == ";") {
         // std::cerr << "  Skipping: using namespace std;" << std::endl; // Debug
        return i + 4;
    }
    // Skip preprocessor lines
     else if (token.type_str == "PREPROCESSOR") {
         // std::cerr << "  Skipping: Preprocessor " << token.lexeme << std::endl; // Debug
         size_t pp_end = i + 1;
         int start_tok_num = token.line_num; // Assuming line_num is token number/index
         // This heuristic might be flawed if line_num isn't reliable
         while (pp_end < tokens.size() && tokens[pp_end].line_num == start_tok_num && tokens[pp_end].lexeme != ";") {
             pp_end++;
         }
         // Handle case where preprocessor ends line without ;
         if (pp_end < tokens.size() && tokens[pp_end].line_num != start_tok_num && tokens[pp_end-1].lexeme != ";") {
            return pp_end; // Return index of token on next line
         }
         return pp_end + (pp_end < tokens.size() && tokens[pp_end].lexeme == ";" ? 1 : 0); // Skip past EOL or ;
     }
     // Skip standalone braces, commas, semicolons if they somehow appear at top level
     else if (token.lexeme == "{" || token.lexeme == "}" || token.lexeme == "," || token.lexeme == ";") {
         // std::cerr << "  Skipping: punctuation " << token.lexeme << std::endl; // Debug
        return i + 1;
     }
    // --- END: Explicit Preamble Skipping ---


    // --- Function Definition --- Check AFTER skipping preamble
    if (is_safe(3) && (token.type_str == "KEYWORD" || token.type_str == "IDENTIFIER") && tokens[i + 1].type_str == "IDENTIFIER" && tokens[i + 2].lexeme == "(") {
        // ... (Function Definition logic - SAME AS V8) ...
        // std::cerr << "  Matched: Function Definition" << std::endl; // Debug
        std::string func_name = tokens[i + 1].lexeme;
        variables.insert(func_name);
        three_addr_code.push_back("");
        three_addr_code.push_back("func begin " + func_name);
        size_t body_start_idx = i + 2; // Start search for '{' from '('
        int paren_level = 0;
        size_t params_end_idx = body_start_idx;
        while(params_end_idx < tokens.size()) {
             if(tokens[params_end_idx].lexeme == "(") paren_level++;
             else if(tokens[params_end_idx].lexeme == ")") paren_level--;
             if(paren_level == 1 && tokens[params_end_idx].type_str == "IDENTIFIER" && tokens[params_end_idx-1].lexeme != "(" && tokens[params_end_idx-1].lexeme != ",") {
                 variables.insert(tokens[params_end_idx].lexeme);
             }
             if (paren_level == 0 && tokens[params_end_idx].lexeme == ")") break;
              if (paren_level < 0) break;
             params_end_idx++;
        }
        body_start_idx = params_end_idx + 1;
        while (body_start_idx < tokens.size() && tokens[body_start_idx].lexeme != "{") body_start_idx++;

        if (is_safe(body_start_idx - i) && tokens[body_start_idx].lexeme == "{") {
            size_t body_end_idx = findEndOfStatementOrBlock(tokens, body_start_idx);
            if (body_end_idx > body_start_idx) {
                 processTokenSequence(tokens, body_start_idx + 1, body_end_idx - 1, three_addr_code, variables);
            }
            three_addr_code.push_back("func end " + func_name);
            return body_end_idx + 1;
        } else { return i + 1; } // Malformed
    }

    // --- If Statement --- Pattern: if ( ID OP LIT/ID ) ...
    else if (token.lexeme == "if" && is_safe(5) && tokens[i+1].lexeme == "(" && tokens[i+2].type_str == "IDENTIFIER" && tokens[i+3].type_str == "OPERATOR" && (tokens[i+4].type_str.find("LITERAL") != std::string::npos || tokens[i+4].type_str == "IDENTIFIER") && tokens[i+5].lexeme == ")")
    {
        // ... (If Statement logic - SAME AS V8) ...
         // std::cerr << "  Matched: If Statement" << std::endl; // Debug
        std::string op1 = tokens[i+2].lexeme; variables.insert(op1);
        std::string op = tokens[i+3].lexeme;
        std::string op2 = tokens[i+4].lexeme;
        if (tokens[i+4].type_str == "IDENTIFIER") variables.insert(op2);
        std::string cond_temp = newTemp();
        three_addr_code.push_back(cond_temp + " = " + op1 + " " + op + " " + op2);
        std::string label_else = newLabel();
        std::string label_endif = newLabel();
        three_addr_code.push_back("ifFalse " + cond_temp + " goto " + label_else);
        size_t then_start_idx = i + 6;
        size_t then_end_idx = findEndOfStatementOrBlock(tokens, then_start_idx);
        size_t next_idx_after_then = processTokenSequence(tokens, then_start_idx, then_end_idx, three_addr_code, variables);
        three_addr_code.push_back("goto " + label_endif);
        three_addr_code.push_back(label_else + ":");
        size_t else_start_idx = next_idx_after_then;
        size_t else_end_idx = findEndOfStatementOrBlock(tokens, else_start_idx);
         size_t next_idx_after_else = processTokenSequence(tokens, else_start_idx, else_end_idx, three_addr_code, variables);
        three_addr_code.push_back(label_endif + ":");
        return next_idx_after_else;
    }

    // --- Return Statement ---
    else if (token.lexeme == "return") {
        // ... (Return Statement logic - SAME AS V8) ...
        // std::cerr << "  Matched: Return Statement" << std::endl; // Debug
        size_t expr_start_idx = i + 1;
        size_t expr_end_idx = findEndOfStatementOrBlock(tokens, expr_start_idx); // Find ';'
        if (is_safe(expr_start_idx - i + 8) && tokens[expr_start_idx].type_str == "IDENTIFIER" && tokens[expr_start_idx + 1].lexeme == "*" && tokens[expr_start_idx + 2].type_str == "IDENTIFIER" && tokens[expr_start_idx + 3].lexeme == "(" && tokens[expr_start_idx + 4].type_str == "IDENTIFIER" && tokens[expr_start_idx + 5].lexeme == "-" && tokens[expr_start_idx + 6].type_str.find("LITERAL") != std::string::npos && tokens[expr_start_idx + 7].lexeme == ")" && expr_end_idx >= expr_start_idx + 8 && tokens[expr_end_idx].lexeme == ";") {
             std::string ret_op1 = tokens[expr_start_idx].lexeme; variables.insert(ret_op1); std::string ret_op = tokens[expr_start_idx + 1].lexeme; std::string ret_func = tokens[expr_start_idx + 2].lexeme; variables.insert(ret_func); std::string p_op1 = tokens[expr_start_idx + 4].lexeme; variables.insert(p_op1); std::string p_op = tokens[expr_start_idx + 5].lexeme; std::string p_op2_lit = tokens[expr_start_idx + 6].lexeme;
             std::string param_temp = newTemp(); three_addr_code.push_back(param_temp + " = " + p_op1 + " " + p_op + " " + p_op2_lit);
             three_addr_code.push_back("param " + param_temp);
             std::string call_res = newTemp(); three_addr_code.push_back(call_res + " = call " + ret_func + ", 1");
             std::string final_res = newTemp(); three_addr_code.push_back(final_res + " = " + ret_op1 + " " + ret_op + " " + call_res);
             three_addr_code.push_back("return " + final_res);
             return expr_end_idx + 1;
         } else if (expr_start_idx <= expr_end_idx && (expr_start_idx == expr_end_idx) && (tokens[expr_start_idx].type_str == "IDENTIFIER" || tokens[expr_start_idx].type_str.find("LITERAL") != std::string::npos)) {
            std::string ret_val = tokens[expr_start_idx].lexeme; three_addr_code.push_back("return " + ret_val); if (tokens[expr_start_idx].type_str == "IDENTIFIER") variables.insert(ret_val);
             return expr_end_idx + 1;
        } else if (expr_start_idx > expr_end_idx) {
            three_addr_code.push_back("return"); return expr_end_idx + 1;
        } else {
            std::string expr_placeholder = ""; for(size_t k=expr_start_idx; k<=expr_end_idx; ++k) { if(k < tokens.size()) expr_placeholder += tokens[k].lexeme + " "; } if (!expr_placeholder.empty()) expr_placeholder.pop_back();
             three_addr_code.push_back("return (" + expr_placeholder + ")"); for(size_t k = expr_start_idx; k <= expr_end_idx; ++k) { if(k < tokens.size() && tokens[k].type_str == "IDENTIFIER") variables.insert(tokens[k].lexeme); }
              return expr_end_idx + 1;
        }
    }

    // --- Assignment --- Pattern: [TYPE] ID = ...
    else if ( (tokens[i].type_str=="KEYWORD" && is_safe(2) && tokens[i+1].type_str == "IDENTIFIER" && tokens[i+2].lexeme == "=" ) /* int c = ... */ ||
              (tokens[i].type_str=="IDENTIFIER" && is_safe(1) && tokens[i+1].lexeme == "=") /* c = ... */ )
    {
        // ... (Assignment logic - SAME AS V8) ...
        size_t eq_idx = (tokens[i].type_str=="KEYWORD") ? i+2 : i+1; size_t lhs_idx = (tokens[i].type_str=="KEYWORD") ? i+1 : i;
        std::string lhs = tokens[lhs_idx].lexeme; variables.insert(lhs);
        size_t rhs_start = eq_idx + 1;
        if (is_safe(rhs_start - i + 8) && tokens[rhs_start].type_str == "IDENTIFIER" && tokens[rhs_start + 1].lexeme == "(" && tokens[rhs_start + 2].type_str == "IDENTIFIER" && tokens[rhs_start + 3].lexeme == ")" && tokens[rhs_start + 4].lexeme == "*" && tokens[rhs_start + 5].type_str == "IDENTIFIER" && tokens[rhs_start + 6].lexeme == "(" && tokens[rhs_start + 7].type_str == "IDENTIFIER" && tokens[rhs_start + 8].lexeme == ")") {
             size_t pattern_end_idx = rhs_start + 8; if (is_safe(pattern_end_idx -i) && tokens[pattern_end_idx].lexeme == ";") {
                  std::string func1 = tokens[rhs_start].lexeme; variables.insert(func1); std::string arg1 = tokens[rhs_start + 2].lexeme; variables.insert(arg1); std::string op = tokens[rhs_start + 4].lexeme; std::string func2 = tokens[rhs_start + 5].lexeme; variables.insert(func2); std::string arg2 = tokens[rhs_start + 7].lexeme; variables.insert(arg2);
                  std::string temp1 = newTemp(); three_addr_code.push_back("param " + arg1); three_addr_code.push_back(temp1 + " = call " + func1 + ", 1");
                  std::string temp2 = newTemp(); three_addr_code.push_back("param " + arg2); three_addr_code.push_back(temp2 + " = call " + func2 + ", 1");
                  std::string temp3 = newTemp(); three_addr_code.push_back(temp3 + " = " + temp1 + " " + op + " " + temp2); three_addr_code.push_back(lhs + " = " + temp3);
                  return pattern_end_idx + 1;
             }
         } else if (is_safe(rhs_start -i + 1) && (tokens[rhs_start].type_str == "IDENTIFIER" || tokens[rhs_start].type_str.find("LITERAL") != std::string::npos) && tokens[rhs_start + 1].lexeme == ";") {
              std::string rhs = tokens[rhs_start].lexeme; if (tokens[rhs_start].type_str == "IDENTIFIER") variables.insert(rhs); three_addr_code.push_back(lhs + " = " + rhs); return rhs_start + 2;
         } else { size_t assign_end = findEndOfStatementOrBlock(tokens, i); return assign_end + 1; } // Skip unhandled assignment
    }

    // --- I/O Statements ---
    else if (token.lexeme == "cin" && is_safe(4) && tokens[i+1].lexeme == ">>" && tokens[i+2].type_str == "IDENTIFIER" && tokens[i+3].lexeme == ">>" && tokens[i+4].type_str == "IDENTIFIER") {
        // ... (Cin logic - SAME AS V8) ...
        size_t stmt_end = findEndOfStatementOrBlock(tokens, i); if(stmt_end < tokens.size() && tokens[stmt_end].lexeme == ";"){ std::string var1 = tokens[i+2].lexeme; variables.insert(var1); std::string var2 = tokens[i+4].lexeme; variables.insert(var2); three_addr_code.push_back("read " + var1); three_addr_code.push_back("read " + var2); return stmt_end + 1; }
    }
    else if (token.lexeme == "cout" && is_safe(3) && tokens[i+1].lexeme == "<<" && tokens[i+2].type_str == "IDENTIFIER") {
         // ... (Cout logic - SAME AS V8) ...
         size_t stmt_end = findEndOfStatementOrBlock(tokens, i); if(stmt_end < tokens.size() && tokens[stmt_end].lexeme == ";"){ std::string var1 = tokens[i+2].lexeme; variables.insert(var1); three_addr_code.push_back("write " + var1); return stmt_end + 1; }
    }

    // --- Variable Declaration (just skip and track names) ---
    else if ( (token.type_str == "KEYWORD" && (token.lexeme == "int" || token.lexeme == "float" /* etc */) ) ) {
        // ... (Declaration logic - SAME AS V8) ...
        bool assignment_found = false; size_t check_idx = i + 1;
        while(check_idx < tokens.size() && tokens[check_idx].lexeme != ";") { if(tokens[check_idx].lexeme == "=") { assignment_found = true; break; } check_idx++; }
        if (!assignment_found) {
             size_t decl_end = i + 1; while (decl_end < tokens.size() && tokens[decl_end].lexeme != ";") { if (tokens[decl_end].type_str == "IDENTIFIER" && decl_end > 0 && tokens[decl_end-1].lexeme != "(") { variables.insert(tokens[decl_end].lexeme); } decl_end++; } return decl_end + 1;
        } // else: let assignment rule handle it if possible by falling through
    }


    // --- Fallback: Unhandled token ---
    // std::cerr << "ICG Debug: Default skip for unhandled token [" << i << "]: '" << token.lexeme << "' (" << token.type_str << ")" << std::endl; // Debug
    if (token.type_str == "IDENTIFIER") { // Track potentially used identifiers
        variables.insert(token.lexeme);
    }
    return i + 1; // CRITICAL: Ensure we always advance index if no pattern matches

} // --- End of generate3ACRecursive function (V9) ---
// --- Main Function (V5 - Use Top-Level Loop - No changes needed here) ---
int main(int argc, char* argv[]) {
    if (argc != 2) { std::cerr << "Usage: intermediate_gen <lexer_output_filename>\n"; return 1; }
    std::string lexer_output_file = argv[1];
    std::string tac_output_file = "3ac_output.txt";
    std::string dag_input_vars_file = "dag_vars.txt";

    std::cout << "ICG: Parsing token file: " << lexer_output_file << std::endl;
    std::vector<Token> tokens = parseLexerOutputFileWithLines(lexer_output_file);
    if (tokens.empty() && !std::ifstream(lexer_output_file)) { std::cerr << "ICG: Input token file not found or empty...\n"; return 1; }
    else if (tokens.empty()) { std::cout << "ICG: Token file parsed, but no valid tokens found...\n"; }

    std::cout << "ICG: Generating 3AC..." << std::endl;
    std::vector<std::string> three_addr_code;
    std::set<std::string> variable_names;
    temp_count = 0; label_count = 0;

    size_t current_token_index = 0;
    size_t last_processed_index = -1; // Use -1 to ensure first iteration works

    while (current_token_index < tokens.size()) {
        current_token_index = generate3ACRecursive(tokens, current_token_index, three_addr_code, variable_names);

        if (current_token_index <= last_processed_index && current_token_index < tokens.size() && tokens[current_token_index].type_str != "END_OF_FILE" ) {
             std::cerr << "ICG Warning: No progress made at token index " << current_token_index << " ('" << tokens[current_token_index].lexeme << "'). Stopping." << std::endl;
              three_addr_code.push_back("# WARNING: Generation stopped due to lack of progress.");
             break;
        }
        last_processed_index = current_token_index;

        if (current_token_index < tokens.size() && tokens[current_token_index].type_str == "END_OF_FILE") break;
    }


    std::ofstream tac_outfile(tac_output_file);
    if (!tac_outfile) { std::cerr << "Error: Cannot open 3AC output file...\n"; return 1; }
    tac_outfile << "# Three-Address Code (Simulated - V6)" << std::endl; // Update version marker
    if (three_addr_code.empty()) tac_outfile << "# (No 3AC generated)\n";
    else for (const auto& line : three_addr_code) tac_outfile << line << std::endl;
    tac_outfile.close();

    std::ofstream dagvars_outfile(dag_input_vars_file);
     if (!dagvars_outfile) { std::cerr << "Error: Cannot open DAG variables file...\n"; }
     else {
        dagvars_outfile << "# Variables for DAG input (Approximation)" << std::endl;
        if (variable_names.empty()) dagvars_outfile << "# (No variables tracked)\n";
        else {
             for(const auto& var : variable_names) {
                 // Filter temps and labels
                 if (var.length() > 0 && !(var[0] == 't' && std::isdigit(var[1])) && !(var[0] == 'L' && std::isdigit(var[1])) ) {
                     dagvars_outfile << var << std::endl;
                 }
             }
        }
        dagvars_outfile.close();
    }

    std::cout << "ICG: 3AC generation complete. Written to " << tac_output_file << std::endl;
    std::cout << "ICG: Variable list written to " << dag_input_vars_file << std::endl;
    return 0;
}