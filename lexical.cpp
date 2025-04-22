#include <iostream>
#include <string>
#include <vector>
#include <cctype>     
#include <unordered_map>
#include <stdexcept>   
#include <sstream>       
#include <fstream>     
#include <iomanip>        

enum class TokenType {

    K_AUTO, K_BREAK, K_CASE, K_CHAR, K_CONST, K_CONTINUE, K_DEFAULT, K_DO,
    K_DOUBLE, K_ELSE, K_ENUM, K_EXTERN, K_FLOAT, K_FOR, K_GOTO, K_IF,
    K_INT, K_LONG, K_REGISTER, K_RETURN, K_SHORT, K_SIGNED, K_SIZEOF, K_STATIC,
    K_STRUCT, K_SWITCH, K_TYPEDEF, K_UNION, K_UNSIGNED, K_VOID, K_VOLATILE, K_WHILE,
    
    K_CLASS, K_PUBLIC, K_PRIVATE, K_PROTECTED, K_NEW, K_DELETE, K_THIS, K_NAMESPACE,
    K_USING, K_TRUE, K_FALSE, K_TRY, K_CATCH, K_THROW, K_CONST_CAST, K_DYNAMIC_CAST,
    K_REINTERPRET_CAST, K_STATIC_CAST, K_TEMPLATE, K_TYPENAME,

    // Identifiers
    IDENTIFIER,

    // Literals
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    CHAR_LITERAL,

    // Operators
    OPERATOR, // Covers all operators like +, -, *, /, %, =, <, >, !, &, |, ^, ~, ?, :, ., ->, ++, --, <<, >>, ::, etc.

    // Separators (previously Punctuation)
    LPAREN,      // (
    RPAREN,      // )
    LBRACE,      // {
    RBRACE,      // }
    LBRACKET,    // [
    RBRACKET,    // ]
    SEMICOLON,   // ;
    COMMA,       // ,
    COLON,       // :

    // Preprocessor
    PREPROCESSOR, // e.g., #include, #define

    // Special
    END_OF_FILE,
    UNKNOWN // For errors or unrecognized characters
};

// --- NEW: Function to get broad category string ---
std::string getBroadCategory(TokenType type) {
    switch (type) {
        // Keywords
        case TokenType::K_AUTO: case TokenType::K_BREAK: case TokenType::K_CASE: case TokenType::K_CHAR:
        case TokenType::K_CONST: case TokenType::K_CONTINUE: case TokenType::K_DEFAULT: case TokenType::K_DO:
        case TokenType::K_DOUBLE: case TokenType::K_ELSE: case TokenType::K_ENUM: case TokenType::K_EXTERN:
        case TokenType::K_FLOAT: case TokenType::K_FOR: case TokenType::K_GOTO: case TokenType::K_IF:
        case TokenType::K_INT: case TokenType::K_LONG: case TokenType::K_REGISTER: case TokenType::K_RETURN:
        case TokenType::K_SHORT: case TokenType::K_SIGNED: case TokenType::K_SIZEOF: case TokenType::K_STATIC:
        case TokenType::K_STRUCT: case TokenType::K_SWITCH: case TokenType::K_TYPEDEF: case TokenType::K_UNION:
        case TokenType::K_UNSIGNED: case TokenType::K_VOID: case TokenType::K_VOLATILE: case TokenType::K_WHILE:
        case TokenType::K_CLASS: case TokenType::K_PUBLIC: case TokenType::K_PRIVATE: case TokenType::K_PROTECTED:
        case TokenType::K_NEW: case TokenType::K_DELETE: case TokenType::K_THIS: case TokenType::K_NAMESPACE:
        case TokenType::K_USING: case TokenType::K_TRUE: case TokenType::K_FALSE: case TokenType::K_TRY:
        case TokenType::K_CATCH: case TokenType::K_THROW: case TokenType::K_CONST_CAST: case TokenType::K_DYNAMIC_CAST:
        case TokenType::K_REINTERPRET_CAST: case TokenType::K_STATIC_CAST: case TokenType::K_TEMPLATE: case TokenType::K_TYPENAME:
            return "KEYWORD";

        // Identifier
        case TokenType::IDENTIFIER:
            return "IDENTIFIER";

        // Literals
        case TokenType::INTEGER_LITERAL:
        case TokenType::FLOAT_LITERAL:
        case TokenType::STRING_LITERAL:
        case TokenType::CHAR_LITERAL:
            return "LITERAL";

        // Operator
        case TokenType::OPERATOR: // This now covers all operator symbols including :: -> etc.
            return "OPERATOR";

        // Separators
        case TokenType::LPAREN: case TokenType::RPAREN:
        case TokenType::LBRACE: case TokenType::RBRACE:
        case TokenType::LBRACKET: case TokenType::RBRACKET:
        case TokenType::SEMICOLON: case TokenType::COMMA: case TokenType::COLON:
            return "SEPARATOR";

        // Preprocessor
        case TokenType::PREPROCESSOR:
            return "PREPROCESSOR";

        // Special / Error
        case TokenType::END_OF_FILE:
            return "END_OF_FILE";
        case TokenType::UNKNOWN:
        default: // Catch any unexpected cases
            return "UNKNOWN/ERROR";
    }
}


struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    // Constructor (Same as before)
    Token(TokenType t = TokenType::UNKNOWN, std::string l = "", int ln = 0, int col = 0)
        : type(t), lexeme(std::move(l)), line(ln), column(col) {}

    // Original toString() - can be kept for debugging if needed
    std::string originalToString() const {
        std::stringstream ss;
        // Use the detailed enum name here if needed for debugging
        // ss << "Token(" << tokenTypeToString(type) << ", \"" << lexeme << "\", Line: " << line << ", Col: " << column << ")";
        // Or use the broad category
        ss << "Token(" << getBroadCategory(type) << ", \"" << lexeme << "\", Line: " << line << ", Col: " << column << ")";
        return ss.str();
    }
};

//-----------------------------------------------------------------------------
// 2. Lexer Class (Implementation is the same as before)
//    (Make sure to include the full Lexer class implementation here,
//     including constructor, getNextToken, getAllTokens, and all private
//     helper methods like skipWhitespaceAndComments, recognize..., etc.)
//-----------------------------------------------------------------------------
class Lexer {
public:
    Lexer(const std::string& source)
        : source_code(source), current_pos(0), current_line(1), current_col(1)
    {
        // Initialize keyword map (Same keywords map as before)
         keywords["auto"] = TokenType::K_AUTO;
        keywords["break"] = TokenType::K_BREAK;
        keywords["case"] = TokenType::K_CASE;
        keywords["char"] = TokenType::K_CHAR;
        keywords["const"] = TokenType::K_CONST;
        keywords["continue"] = TokenType::K_CONTINUE;
        keywords["default"] = TokenType::K_DEFAULT;
        keywords["do"] = TokenType::K_DO;
        keywords["double"] = TokenType::K_DOUBLE;
        keywords["else"] = TokenType::K_ELSE;
        keywords["enum"] = TokenType::K_ENUM;
        keywords["extern"] = TokenType::K_EXTERN;
        keywords["float"] = TokenType::K_FLOAT;
        keywords["for"] = TokenType::K_FOR;
        keywords["goto"] = TokenType::K_GOTO;
        keywords["if"] = TokenType::K_IF;
        keywords["int"] = TokenType::K_INT;
        keywords["long"] = TokenType::K_LONG;
        keywords["register"] = TokenType::K_REGISTER;
        keywords["return"] = TokenType::K_RETURN;
        keywords["short"] = TokenType::K_SHORT;
        keywords["signed"] = TokenType::K_SIGNED;
        keywords["sizeof"] = TokenType::K_SIZEOF;
        keywords["static"] = TokenType::K_STATIC;
        keywords["struct"] = TokenType::K_STRUCT;
        keywords["switch"] = TokenType::K_SWITCH;
        keywords["typedef"] = TokenType::K_TYPEDEF;
        keywords["union"] = TokenType::K_UNION;
        keywords["unsigned"] = TokenType::K_UNSIGNED;
        keywords["void"] = TokenType::K_VOID;
        keywords["volatile"] = TokenType::K_VOLATILE;
        keywords["while"] = TokenType::K_WHILE;
        keywords["class"] = TokenType::K_CLASS;
        keywords["public"] = TokenType::K_PUBLIC;
        keywords["private"] = TokenType::K_PRIVATE;
        keywords["protected"] = TokenType::K_PROTECTED;
        keywords["new"] = TokenType::K_NEW;
        keywords["delete"] = TokenType::K_DELETE;
        keywords["this"] = TokenType::K_THIS;
        keywords["namespace"] = TokenType::K_NAMESPACE;
        keywords["using"] = TokenType::K_USING;
        keywords["true"] = TokenType::K_TRUE;
        keywords["false"] = TokenType::K_FALSE;
        keywords["try"] = TokenType::K_TRY;
        keywords["catch"] = TokenType::K_CATCH;
        keywords["throw"] = TokenType::K_THROW;
        keywords["const_cast"] = TokenType::K_CONST_CAST;
        keywords["dynamic_cast"] = TokenType::K_DYNAMIC_CAST;
        keywords["reinterpret_cast"] = TokenType::K_REINTERPRET_CAST;
        keywords["static_cast"] = TokenType::K_STATIC_CAST;
        keywords["template"] = TokenType::K_TEMPLATE;
        keywords["typename"] = TokenType::K_TYPENAME;
    }

    // Get the next token from the source code (Same implementation as before)
    Token getNextToken() {
        skipWhitespaceAndComments(); // Critical: whitespace/comments are skipped here

        int token_start_line = current_line;
        int token_start_col = current_col;
        size_t token_start_pos = current_pos;

        if (isEOF()) {
            return Token(TokenType::END_OF_FILE, "", current_line, current_col);
        }

        char current_char = peek();

        if (std::isalpha(current_char) || current_char == '_') {
            return recognizeIdentifierOrKeyword(token_start_line, token_start_col);
        }
        if (std::isdigit(current_char) || (current_char == '.' && std::isdigit(peek(1)))) {
             return recognizeNumberLiteral(token_start_line, token_start_col);
        }
        if (current_char == '"') {
            return recognizeStringLiteral(token_start_line, token_start_col);
        }
        if (current_char == '\'') {
            return recognizeCharLiteral(token_start_line, token_start_col);
        }
        if (current_char == '#') {
             return recognizePreprocessor(token_start_line, token_start_col);
        }
        // Check simple separators before routing to recognizeOperator
        switch (current_char) {
            case '(': consume(); return Token(TokenType::LPAREN, "(", token_start_line, token_start_col);
            case ')': consume(); return Token(TokenType::RPAREN, ")", token_start_line, token_start_col);
            case '{': consume(); return Token(TokenType::LBRACE, "{", token_start_line, token_start_col);
            case '}': consume(); return Token(TokenType::RBRACE, "}", token_start_line, token_start_col);
            case '[': consume(); return Token(TokenType::LBRACKET, "[", token_start_line, token_start_col);
            case ']': consume(); return Token(TokenType::RBRACKET, "]", token_start_line, token_start_col);
            case ';': consume(); return Token(TokenType::SEMICOLON, ";", token_start_line, token_start_col);
            case ',': consume(); return Token(TokenType::COMMA, ",", token_start_line, token_start_col);
             // Colon and operators handled by recognizeOperator
        }

        // If not a simple separator or other handled type, try operators/colon
         return recognizeOperator(token_start_line, token_start_col);

        // Fallback for truly unrecognized characters (should be rare if recognizeOperator is robust)
        // consume();
        // return Token(TokenType::UNKNOWN, std::string(1, current_char), token_start_line, token_start_col);
    }

    // Helper to get all tokens at once (Same implementation as before)
    std::vector<Token> getAllTokens() {
        std::vector<Token> tokens;
        Token token;
        do {
            token = getNextToken();
            tokens.push_back(token);
            // Add a check to prevent infinite loops on UNKNOWN if consume() wasn't called
             if (token.type == TokenType::UNKNOWN && token.lexeme.empty() && token.line > 0) { // Check line > 0 to avoid issues with initial state
                 if (!isEOF()) {
                    // Manually consume if the recognizer failed to, to avoid looping forever
                    char problematic_char = peek();
                     if (problematic_char != '\0') { // Check if not actually EOF
                         consume();
                         // Find the UNKNOWN token we just added and update its lexeme
                        if (!tokens.empty() && tokens.back().type == TokenType::UNKNOWN) {
                             tokens.back().lexeme = std::string(1, problematic_char);
                        }
                         std::cerr << "Warning: Forcefully consumed unknown character '" << problematic_char
                                   << "' at Line: " << token.line << ", Col: " << token.column << std::endl;
                     } else {
                         // Avoid adding another EOF if we already pushed one and are at EOF
                         if (tokens.size() > 1 && tokens[tokens.size()-2].type == TokenType::END_OF_FILE) {
                             tokens.pop_back(); // Remove duplicate EOF
                         }
                         break; // Exit loop if EOF reached during error recovery
                     }

                 } else {
                     // Avoid adding another EOF if we already pushed one
                     if (tokens.size() > 1 && tokens[tokens.size()-2].type == TokenType::END_OF_FILE) {
                         tokens.pop_back(); // Remove duplicate EOF
                     }
                     break; // Exit loop if EOF reached during error recovery
                 }
            }

        } while (token.type != TokenType::END_OF_FILE);
        return tokens;
    }


private:
    std::string source_code;
    size_t current_pos;
    int current_line;
    int current_col; // Column number where the current character *starts*

    std::unordered_map<std::string, TokenType> keywords;

    // --- Helper Methods ---
    // ... PASTE ALL THE PRIVATE HELPER METHODS FROM THE PREVIOUS C++ ANSWER HERE ...
    // (isEOF, peek, consume, skipWhitespaceAndComments, recognizeIdentifierOrKeyword,
    //  recognizeNumberLiteral, recognizeStringLiteral, recognizeCharLiteral,
    //  recognizePreprocessor, recognizeOperator)

     // Check if we are at the end of the source code
    bool isEOF(size_t offset = 0) const {
        return (current_pos + offset) >= source_code.length();
    }

    // Look at the character at the current position + offset without consuming it
    char peek(size_t offset = 0) const {
        if (isEOF(offset)) {
            return '\0'; // Null character signifies EOF
        }
        return source_code[current_pos + offset];
    }

    // Consume the current character and advance the position
    char consume() {
        if (isEOF()) {
            return '\0';
        }
        char current_char = source_code[current_pos];
        if (current_char == '\n') {
            current_line++;
            current_col = 1; // Reset column on newline
        } else {
            current_col++;
        }
        current_pos++;
        return current_char;
    }

    // Skips whitespace and comments - THEY WILL NOT APPEAR AS TOKENS
    void skipWhitespaceAndComments() {
        while (!isEOF()) {
            char current_char = peek();

            // Skip whitespace
            if (std::isspace(current_char)) {
                consume();
                continue;
            }

            // Skip single-line comments (//)
            if (current_char == '/' && peek(1) == '/') {
                consume(); // Consume '/'
                consume(); // Consume '/'
                while (!isEOF() && peek() != '\n') {
                    consume(); // Consume until newline
                }
                // Don't consume the newline itself, let the main loop handle it or skip it next iteration
                continue;
            }

            // Skip multi-line comments (/* ... */)
            if (current_char == '/' && peek(1) == '*') {
                int start_line = current_line; // For error reporting if unterminated
                int start_col = current_col;
                consume(); // Consume '/'
                consume(); // Consume '*'
                bool terminated = false;
                while (!isEOF()) {
                    if (peek() == '*' && peek(1) == '/') {
                        consume(); // Consume '*'
                        consume(); // Consume '/'
                        terminated = true;
                        break; // Exit comment loop
                    }
                    consume(); // Consume character inside comment
                }
                if (!terminated) { // Check if EOF was reached before */
                   std::cerr << "Warning: Unterminated block comment starting at Line: " << start_line << ", Col: " << start_col << std::endl;
                }
                continue;
            }

            // If it's not whitespace or a comment, break the loop
            break;
        }
    }

    // Recognizes Identifiers (variable names, function names, etc.) and Keywords
    Token recognizeIdentifierOrKeyword(int start_line, int start_col) {
        std::string lexeme;
        size_t start_pos = current_pos;
        // Identifiers start with a letter or underscore
        if (std::isalpha(peek()) || peek() == '_') {
             lexeme += consume();
        } else {
             // Should not happen based on calling context, but defensive check
            return Token(TokenType::UNKNOWN, "", start_line, start_col);
        }

        // Then followed by letters, digits, or underscores
        while (!isEOF() && (std::isalnum(peek()) || peek() == '_')) {
            lexeme += consume();
        }

        // Check if the identifier is actually a keyword
        auto keyword_it = keywords.find(lexeme);
        if (keyword_it != keywords.end()) {
            return Token(keyword_it->second, lexeme, start_line, start_col);
        } else {
            return Token(TokenType::IDENTIFIER, lexeme, start_line, start_col);
        }
    }

     // Recognizes Integer or Floating Point Literals
    Token recognizeNumberLiteral(int start_line, int start_col) {
        std::string lexeme;
        bool is_float = false;

        // Handle leading decimal point (e.g., .5)
        if (peek() == '.') {
            if (std::isdigit(peek(1))) {
                lexeme += consume(); // Consume '.'
                is_float = true;
            } else {
                // It's just a '.', the member access operator. Let operator handler deal with it.
                return recognizeOperator(start_line, start_col); // Re-route
            }
        }

        // Consume digits before potential decimal
        while (!isEOF() && std::isdigit(peek())) {
            lexeme += consume();
        }

        // Check for decimal point if not already seen
        if (!is_float && peek() == '.') {
             if (std::isdigit(peek(1))) {
                lexeme += consume(); // Consume '.'
                is_float = true;
                 // Consume digits after decimal
                while (!isEOF() && std::isdigit(peek())) {
                    lexeme += consume();
                }
            } else {
                // Integer followed by '.', return the integer.
                return Token(TokenType::INTEGER_LITERAL, lexeme, start_line, start_col);
            }
        }

        // Check for exponent part (e or E)
        if (!isEOF() && (std::tolower(peek()) == 'e')) {
            if (!lexeme.empty() && (std::isdigit(lexeme.back()) || lexeme.back() == '.')) {
                 is_float = true;
                lexeme += consume(); // Consume 'e' or 'E'
                if (!isEOF() && (peek() == '+' || peek() == '-')) {
                    lexeme += consume();
                }
                if(!isEOF() && std::isdigit(peek())) {
                    while (!isEOF() && std::isdigit(peek())) {
                        lexeme += consume();
                    }
                } else {
                     std::cerr << "Warning: Malformed exponent at Line: " << current_line << ", Col: " << current_col << std::endl;
                     return Token(TokenType::UNKNOWN, lexeme, start_line, start_col);
                }
            }
        }

        // Check for suffixes (f, F, u, U, l, L, ll, LL) - simplified check
        if (!isEOF()) {
            char next_lower = std::tolower(peek());
             if (is_float && next_lower == 'f') {
                 lexeme += consume();
             } else if (!is_float) {
                 // Basic suffix check - allows U, L, LL in some order
                 bool U_seen = false;
                 bool L_seen = false;
                 bool LL_seen = false;
                 while(!isEOF()){
                    next_lower = std::tolower(peek());
                    if (next_lower == 'u' && !U_seen) { U_seen = true; lexeme += consume(); }
                    else if (next_lower == 'l' && !LL_seen) {
                        lexeme += consume();
                        if (L_seen) LL_seen = true; L_seen = true;
                    } else break;
                 }
             }
        }

        if (is_float) {
            return Token(TokenType::FLOAT_LITERAL, lexeme, start_line, start_col);
        } else {
            if (lexeme.empty()) { // Should be handled by initial '.' check redirecting
                 return recognizeOperator(start_line, start_col);
            }
            return Token(TokenType::INTEGER_LITERAL, lexeme, start_line, start_col);
        }
    }

    // Recognizes String Literals enclosed in double quotes ""
    Token recognizeStringLiteral(int start_line, int start_col) {
        std::string raw_lexeme;
        raw_lexeme += consume(); // Consume starting '"'

        while (!isEOF()) {
            char current_char = peek();
            if (current_char == '\\') { // Handle escape sequences
                raw_lexeme += consume(); // Consume '\'
                if (!isEOF()) {
                     raw_lexeme += consume(); // Consume the escaped character
                } else {
                    std::cerr << "Error: Unterminated escape sequence in string literal at Line: " << current_line << ", Col: " << current_col << std::endl;
                    return Token(TokenType::UNKNOWN, raw_lexeme, start_line, start_col);
                }
            } else if (current_char == '"') {
                raw_lexeme += consume(); // Consume ending '"'
                return Token(TokenType::STRING_LITERAL, raw_lexeme, start_line, start_col);
            } else if (current_char == '\n') {
                 std::cerr << "Error: Newline in string literal at Line: " << start_line << ", Col: " << start_col << std::endl;
                return Token(TokenType::UNKNOWN, raw_lexeme, start_line, start_col); // Stop processing this broken literal
            } else {
                 raw_lexeme += consume();
            }
        }
        std::cerr << "Error: Unterminated string literal starting at Line: " << start_line << ", Col: " << start_col << std::endl;
        return Token(TokenType::UNKNOWN, raw_lexeme, start_line, start_col);
    }

    // Recognizes Character Literals enclosed in single quotes ''
    Token recognizeCharLiteral(int start_line, int start_col) {
        std::string raw_lexeme;
        raw_lexeme += consume(); // Consume starting '''
        if (isEOF()) {
             std::cerr << "Error: Unterminated char literal at Line: " << start_line << ", Col: " << start_col << std::endl;
            return Token(TokenType::UNKNOWN, raw_lexeme, start_line, start_col);
        }
        if (peek() == '\'') {
             raw_lexeme += consume(); // Consume '
             std::cerr << "Error: Empty char literal '' at Line: " << start_line << ", Col: " << start_col << std::endl;
             return Token(TokenType::UNKNOWN, raw_lexeme, start_line, start_col);
        }
        if (peek() == '\\') { // Handle escape sequence
            raw_lexeme += consume(); // Consume '\'
             if (!isEOF()) { raw_lexeme += consume(); } // Consume escaped char
             else {
                  std::cerr << "Error: Unterminated escape in char literal at Line: " << start_line << ", Col: " << start_col << std::endl;
                 return Token(TokenType::UNKNOWN, raw_lexeme, start_line, start_col);
             }
        } else { raw_lexeme += consume(); } // Consume the character

        if (!isEOF() && peek() == '\'') {
            raw_lexeme += consume(); // Consume closing '''
            // Basic length check (optional warning)
            if (raw_lexeme.length() > 4 || (raw_lexeme.length() > 3 && raw_lexeme[1] != '\\')) {
                  std::cerr << "Warning: Multi-character/Malformed char constant " << raw_lexeme << " at Line: " << start_line << ", Col: " << start_col << std::endl;
            }
            return Token(TokenType::CHAR_LITERAL, raw_lexeme, start_line, start_col);
        } else {
            std::cerr << "Error: Malformed or unterminated char literal starting at Line: " << start_line << ", Col: " << start_col << " Found: " << raw_lexeme << peek() << "..." << std::endl;
             while (!isEOF() && peek() != '\'' && !std::isspace(peek()) && peek() != ';') { raw_lexeme += consume(); } // Consume until likely end
             if (!isEOF() && peek() == '\'') raw_lexeme += consume();
            return Token(TokenType::UNKNOWN, raw_lexeme, start_line, start_col);
        }
    }

     // Recognizes preprocessor directives (lines starting with #)
     Token recognizePreprocessor(int start_line, int start_col) {
        std::string lexeme;
        lexeme += consume(); // Consume '#'
        while (!isEOF() && peek() != '\n') {
            if (peek() == '\\') { // Handle line continuation
                if (peek(1) == '\n') { consume(); consume(); } // Consume '\' and '\n'
                 else if (peek(1) == '\r' && peek(2) == '\n') { consume(); consume(); consume(); } // Consume '\', '\r', '\n'
                else { lexeme += consume(); } // '\' not followed by newline
            } else { lexeme += consume(); }
        }
        return Token(TokenType::PREPROCESSOR, lexeme, start_line, start_col);
    }

     // Recognizes operators and the colon separator
    Token recognizeOperator(int start_line, int start_col) {
        std::string lexeme;
        size_t start_pos = current_pos;
        char c1 = peek();

        if (c1 == '.' && std::isdigit(peek(1))) { return recognizeNumberLiteral(start_line, start_col); }
        if (c1 == '\0') { return Token(TokenType::UNKNOWN, "", start_line, start_col); } // Avoid consuming EOF

        c1 = consume();
        lexeme += c1;
        char c2 = peek();

        switch (c1) {
            // Check for multi-character operators
            case '+': if (c2 == '+' || c2 == '=') lexeme += consume(); break;
            case '-': if (c2 == '-' || c2 == '=' || c2 == '>') lexeme += consume(); break;
            case '*': if (c2 == '=') lexeme += consume(); break;
            case '/': if (c2 == '=') lexeme += consume(); break; // Comments handled earlier
            case '%': if (c2 == '=') lexeme += consume(); break;
            case '=': if (c2 == '=') lexeme += consume(); break;
            case '!': if (c2 == '=') lexeme += consume(); break;
            case '<': if (c2 == '<') { lexeme += consume(); if (peek() == '=') lexeme += consume(); } else if (c2 == '=') lexeme += consume(); break;
            case '>': if (c2 == '>') { lexeme += consume(); if (peek() == '=') lexeme += consume(); } else if (c2 == '=') lexeme += consume(); break;
            case '&': if (c2 == '&' || c2 == '=') lexeme += consume(); break;
            case '|': if (c2 == '|' || c2 == '=') lexeme += consume(); break;
            case '^': if (c2 == '=') lexeme += consume(); break;
            case ':': if (c2 == ':') lexeme += consume(); /* :: is Operator */ else return Token(TokenType::COLON, lexeme, start_line, start_col); break; // Single : is Separator
            case '.': if (c2 == '.' && peek(1) == '.') { lexeme += consume(); lexeme += consume(); } /* ... is Operator */ break; // Single . is Operator
            // Single char operators fall through
            case '~': case '?': break;
            // Any other character consumed here that wasn't punctuation/other is unknown
            default:
                  // If c1 was one of the simple separators, it should have been handled before calling this.
                 // This case handles unrecognized symbols.
                std::cerr << "Warning: Unrecognized character '" << c1 << "' treated as UNKNOWN at Line: " << start_line << ", Col: " << start_col << std::endl;
                return Token(TokenType::UNKNOWN, lexeme, start_line, start_col);
        }
        // If we reached here, it's an OPERATOR (multi-char, single-char like +, *, ., or ::, ...)
        return Token(TokenType::OPERATOR, lexeme, start_line, start_col);
    }
};
// --- End of Lexer Class ---


//-----------------------------------------------------------------------------
// 3. Main Function (Modified for Categorized Table Output)
//-----------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_filename.cpp>" << std::endl;
        return 1;
    }
    std::string input_filename = argv[1];
    std::string output_filename = "lexer_output.txt";

    std::ifstream inputFile(input_filename);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open input file: " << input_filename << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << inputFile.rdbuf();
    std::string source_code = buffer.str();
    inputFile.close();

    std::cout << "Read source code from: " << input_filename << std::endl;

    Lexer lexer(source_code);
    std::vector<Token> tokens;
     try {
        tokens = lexer.getAllTokens();
     } catch (const std::exception& e) {
         std::cerr << "Lexer Error: " << e.what() << std::endl;
         return 1;
     } catch (...) {
         std::cerr << "An unknown lexer error occurred." << std::endl;
         return 1;
     }

    std::ofstream outputFile(output_filename);
    if (!outputFile.is_open()) {
        std::cerr << "Error: Could not open output file: " << output_filename << std::endl;
        return 1;
    }

    std::cout << "Writing categorized lexer output to: " << output_filename << std::endl;

    // --- Write Table Header ---
    outputFile << std::left << std::setw(12) << "Token Num" << " | "
               << std::left << std::setw(15) << "Type" << " | "
               << "Lexeme" << std::endl;
    outputFile << std::string(50, '-') << std::endl; // Separator line

    // --- Write Token Data ---
    int token_number = 1;
    for (const auto& token : tokens) {
        std::string broad_category = getBroadCategory(token.type);

        // Format and write the row
        outputFile << std::left << std::setw(12) << token_number << " | "
                   << std::left << std::setw(15) << broad_category << " | "
                   << token.lexeme << std::endl;

        // Optionally print to console as well
        // std::cout << std::left << std::setw(12) << token_number << " | "
        //           << std::left << std::setw(15) << broad_category << " | "
        //           << token.lexeme << std::endl;


        if (token.type == TokenType::END_OF_FILE) {
            break; // No need to increment after EOF
        }
         // Stop if a critical error occurs? (getAllTokens should mostly prevent infinite loops now)
        if (token.type == TokenType::UNKNOWN) {
            // You might choose to stop or just continue logging errors
        }

        token_number++;
    }

    outputFile.close();

    std::cout << "Lexical analysis complete. Categorized output saved." << std::endl;

    return 0;
}