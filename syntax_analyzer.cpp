#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <algorithm>

struct Token {
    std::string type_str;
    std::string lexeme;

    Token(std::string t = "", std::string l = "") : type_str(std::move(t)), lexeme(std::move(l)) {}
};

std::string indentStr(int level) {
    return std::string(level * 2, ' ');
}

std::vector<std::string> generateSimulatedAst(const std::vector<Token>& tokens) {
    std::vector<std::string> ast_output;
    ast_output.push_back("Simplified AST Representation:");
    ast_output.push_back("----------------------------");

    if (tokens.empty()) {
        ast_output.push_back("(No tokens found in input file)");
        return ast_output;
    }

    int indent_level = 0;
    size_t i = 0;

    while (i < tokens.size()) {
        std::string current_indent = indentStr(indent_level);
        const Token& token = tokens[i];
        std::stringstream ss;

        auto is_safe = [&](size_t offset) { return (i + offset) < tokens.size(); };

        if (is_safe(3) &&
            token.lexeme == "using" &&
            tokens[i+1].lexeme == "namespace" &&
            tokens[i+2].lexeme == "std" &&
            tokens[i+3].lexeme == ";")
        {
            i += 3;
        }

        else if (token.lexeme == "}")
        {
            indent_level = std::max(0, indent_level - 1);
            current_indent = indentStr(indent_level);
            ss << current_indent << "}";
            ast_output.push_back(ss.str());
        }

        else if (is_safe(0) && token.lexeme == "return")
        {
             ss << current_indent << "- Return: ";
             std::string expr_str;
             size_t j = i + 1;
             while(j < tokens.size() && tokens[j].lexeme != ";") {
                 expr_str += tokens[j].lexeme + " ";
                 j++;
             }
             ss << (expr_str.empty() ? "(void)" : "(expression)");
             ast_output.push_back(ss.str());

             if (j < tokens.size() && tokens[j].lexeme == ";") { i = j; }
             else { i = j - 1; ast_output.push_back(current_indent + "  (Warning: Missing semicolon after return?)"); }
        }

        else if (is_safe(3) &&
                 (token.type_str == "KEYWORD" || token.type_str == "IDENTIFIER") &&
                 tokens[i+1].type_str == "IDENTIFIER" &&
                 tokens[i+2].lexeme == "(")
        {
            size_t j = i + 3; int paren_level = 1;
             while (j < tokens.size()) { /* ... find matching ')' ... */ if(tokens[j].lexeme == "(") paren_level++; else if (tokens[j].lexeme == ")") paren_level--; if (paren_level == 0) break; j++; }

            if (j < tokens.size() && is_safe(j-i+1) && tokens[j].lexeme == ")" && tokens[j+1].lexeme == "{") {
                ss << current_indent << "- FunctionDef: " << token.lexeme << " " << tokens[i+1].lexeme << "(...)";
                ast_output.push_back(ss.str());
                ss.str(""); ss << current_indent << "  - Body:";
                ast_output.push_back(ss.str());
                i = j + 1;
                continue;
            } else {
                 i++; continue;
            }
        }

        else if (token.lexeme == "{")
        {
            ss << current_indent << "{";
            ast_output.push_back(ss.str());
            indent_level += 1;
        }

        else if (is_safe(1) &&
                 token.type_str == "KEYWORD" &&
                 tokens[i+1].type_str == "IDENTIFIER")
        {
            ss << current_indent << "- VariableDecl: " << token.lexeme << " " << tokens[i+1].lexeme;
            ast_output.push_back(ss.str());
            ss.str("");

            size_t j = i + 2;
            bool has_init = false;
            if (is_safe(j-i) && tokens[j].lexeme == "=") {
                 has_init = true;
                 ss << current_indent << "  - Initializer: (expression)";
                 ast_output.push_back(ss.str());
                 while (j < tokens.size() && tokens[j].lexeme != ";") { j++; }
            }
             else if (is_safe(j-i) && tokens[j].lexeme == ",") { i = j; }
            else { while (j < tokens.size() && tokens[j].lexeme != ";") { j++; } }

            if (j < tokens.size() && (tokens[j].lexeme == ";" || tokens[j].lexeme == ",")) { i = j; }
            else { i = i + 1; ast_output.push_back(current_indent + "  (Warning: Malformed declaration?)"); }
        }

        else if (is_safe(2) &&
                 token.type_str == "IDENTIFIER" &&
                 tokens[i+1].lexeme == "=")
        {
             ss << current_indent << "- Assignment: Variable(" << token.lexeme << ") = (expression)";
             ast_output.push_back(ss.str());
             size_t j = i + 2;
             while(j < tokens.size() && tokens[j].lexeme != ";") { j++; }
             if (j < tokens.size() && tokens[j].lexeme == ";") { i = j; }
             else { i = j - 1; ast_output.push_back(current_indent + "  (Warning: Missing semicolon after assignment?)"); }
        }

        else if (is_safe(1) &&
                 token.type_str == "IDENTIFIER" && (token.lexeme == "cin" || token.lexeme == "cout") &&
                 tokens[i+1].type_str == "OPERATOR")
         {
             ss << current_indent << "- IO_Statement: " << token.lexeme << " (expression)";
             ast_output.push_back(ss.str());
             size_t j = i + 1;
             while(j < tokens.size() && tokens[j].lexeme != ";") { j++; }
             if (j < tokens.size() && tokens[j].lexeme == ";") { i = j; }
             else { i = j -1; ast_output.push_back(current_indent + "  (Warning: Missing semicolon after IO?)"); }
         }

        else if (is_safe(1) &&
                 token.type_str == "IDENTIFIER" &&
                 tokens[i+1].lexeme == "(")
        {
             size_t j = i + 2; int paren_level = 1;
             while (j < tokens.size()) { /* ... find matching ')' ... */ if(tokens[j].lexeme == "(") paren_level++; else if (tokens[j].lexeme == ")") paren_level--; if (paren_level == 0) break; j++; }

             if (j < tokens.size() && is_safe(j-i+1) && tokens[j].lexeme == ")" && tokens[j+1].lexeme == ";") {
                 ss << current_indent << "- FunctionCall: " << token.lexeme << "(...)";
                 ast_output.push_back(ss.str());
                 i = j + 1;
             } 
        }

        else if (is_safe(1) && token.lexeme == "if" && tokens[i+1].lexeme == "(")
        {
            ss << current_indent << "- IfStmt:";
            ast_output.push_back(ss.str()); ss.str("");

            size_t j = i + 2; int paren_level = 1;
            while(j < tokens.size()) { /* ... find matching ')' ... */ if (tokens[j].lexeme == "(") paren_level++; else if (tokens[j].lexeme == ")") paren_level--; if (paren_level == 0) break; j++; }

            ss << current_indent << "  - Condition: (expression)";
            ast_output.push_back(ss.str()); ss.str("");

            if (j < tokens.size()) {
                i = j;

                ss << current_indent << "  - Then:";
                ast_output.push_back(ss.str());
                if (is_safe(1) && tokens[i+1].lexeme == "{") {
                    // Let the '{' rule handle the indent increase in the next iteration
                } else {
                    // Assume single statement follows - print marker
                    ast_output.push_back(current_indent + "    (single statement follows)");
                    // The next iteration will handle the single statement
                }
            } else {
                 ast_output.push_back(current_indent + "  (Warning: Malformed 'if' condition?)");
            }
        }

        // Rule: Skip Semicolons if not handled by other rules
        else if (token.lexeme == ";") { /* Skip standalone semicolons */ }
        // Rule: Skip Preprocessor directives
        else if (token.type_str == "PREPROCESSOR") { /* Skip */ }
        // Rule: Unhandled Token - Skip silently
        else { /* Skip */ }

        i++; // Move to the next token for the next iteration

    } // End while loop

    if (indent_level != 0) {
         ast_output.push_back("Warning: Final indentation level is not zero (" + std::to_string(indent_level) + "), potential brace mismatch.");
    }
    return ast_output;
}


// --- Function to Parse the Lexer Output File (Revised V3 - With Trim Fix) ---
std::vector<Token> parseLexerOutputFile(const std::string& filename) {
    std::vector<Token> tokens;
    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "Error: Cannot open lexer output file: " << filename << std::endl;
        return tokens;
    }

    std::string line;
    // Skip header/separator lines
    std::getline(infile, line);
    std::getline(infile, line);

    int line_num = 3; // Start counting from after header

    // Define the trim lambda function HERE, inside the function scope
    auto trim = [](const std::string& str) -> std::string {
        const std::string whitespace = " \t\n\r\f\v"; // Define whitespace characters
        size_t first = str.find_first_not_of(whitespace);
        if (first == std::string::npos) {
            return ""; // String is only whitespace
        }
        size_t last = str.find_last_not_of(whitespace);
        return str.substr(first, (last - first + 1));
    };


    while (std::getline(infile, line)) {
         if (line.empty() || line.find_first_not_of(" \t\n\r") == std::string::npos) {
             line_num++;
             continue;
         }

        size_t first_pipe = line.find('|');
        size_t second_pipe = (first_pipe == std::string::npos) ? std::string::npos : line.find('|', first_pipe + 1);

        if (first_pipe != std::string::npos && second_pipe != std::string::npos) {

            std::string type_part = line.substr(first_pipe + 1, second_pipe - first_pipe - 1);
            std::string lexeme_part = line.substr(second_pipe + 1);

            // Now use the defined trim lambda
            std::string type_str = trim(type_part);
            std::string lexeme_str = trim(lexeme_part);

            if (type_str == "END_OF_FILE") {
                 break; // Stop reading on EOF line
            } else if (!type_str.empty()) {
                 tokens.emplace_back(type_str, lexeme_str);
            } else {
                 std::cerr << "Warning: Skipping line " << line_num << " with empty type in " << filename << ": " << line << std::endl;
            }
        } else {
            // Don't print warning for EOF line if it was missing pipes somehow
             if (line.find("END_OF_FILE") == std::string::npos) {
                 std::cerr << "Warning: Skipping malformed line " << line_num << " (missing '|') in " << filename << ": " << line << std::endl;
             }
        }
        line_num++;
    }
    return tokens;
}
// --- Main Function (remains the same) ---
int main(int argc, char* argv[]) {
    if (argc != 2) { /* ... usage error ... */ return 1; }
    std::string lexer_output_file = argv[1];
    std::string ast_output_file = "ast_output.txt";
    // ... (rest of main is the same - parse tokens, generate ast, write file) ...
    std::cout << "Parsing token file: " << lexer_output_file << std::endl;
    std::vector<Token> tokens = parseLexerOutputFile(lexer_output_file);
    if (tokens.empty()) { /* ... */ }
    std::cout << "Generating simulated AST..." << std::endl;
    std::vector<std::string> ast_representation = generateSimulatedAst(tokens);
    std::ofstream outfile(ast_output_file);
    if (!outfile) { /* ... */ return 1; }
    for (const auto& line : ast_representation) { outfile << line << std::endl; }
    outfile.close();
    std::cout << "Syntax analysis simulation complete. AST written to " << ast_output_file << std::endl;
    return 0;
}