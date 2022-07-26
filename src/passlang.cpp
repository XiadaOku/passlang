#include "passlang.h"


namespace passlang {
	namespace Deleter {
		std::vector<std::function<void()>> deleteFunctions;

		template<typename T>
		void addDeletable(T* object) {
			std::function<void()> func = [object]() { delete object; };
			deleteFunctions.push_back(func);
		}

		void deleteAll() {
			for (size_t i = 0; i < deleteFunctions.size(); i++) {
				deleteFunctions[i]();
			}
			deleteFunctions.clear();
		}
	}

    std::ostream& operator<<(std::ostream& stream, C_Check check) {
		stream << int(check.world);
		stream << ".";
		stream << check.x;
		stream << ".";
		stream << check.y;
		return stream;
	}


    /************* TOKENIZER *************/
    std::vector<Token> tokenize(std::string expression) {
		std::vector<Token> tokens;

		for (size_t ind = 0; ind < expression.length(); ind++) {
			char i = expression[ind];
			size_t new_ind;

			switch (i) {
				case '(':
					tokens.push_back(Token(TokenType::openBracket));
					break;
				case ')':
					tokens.push_back(Token(TokenType::closeBracket));
					break;
				case '[':
					tokens.push_back(Token(TokenType::openSquareBracket));
					break;
				case ']':
					tokens.push_back(Token(TokenType::closeSquareBracket));
					break;
				case ';':
					tokens.push_back(Token(TokenType::semicolon));
					break;
				case '+':
				case '-':
				case '*':
				case '/':
				case '%':
					tokens.push_back(Token(TokenType::operation, std::string(1, i)));
					break;
				case '.':
					tokens.push_back(Token(TokenType::checkSeparator));
					break;
				case ' ':
					tokens.push_back(Token(TokenType::space));
					break;
				case 'n':
					tokens.push_back(Token(TokenType::numofChecksVariable));
					break;
				case 'i':
					new_ind = ind;
					do {
						new_ind++;
						i = expression[new_ind];
					} while (new_ind < expression.length() && i >= '0' && i <= '9');
					if (new_ind - (ind + 1)) {
						tokens.push_back(Token(TokenType::loopIteratorVariable, std::stoi(expression.substr(ind + 1, new_ind - (ind + 1)))));
						ind = new_ind - 1;
					}
					break;
				default:
					new_ind = ind;
					while (new_ind < expression.length() && i >= '0' && i <= '9') {
						new_ind++;
						i = expression[new_ind];
					}
					if (new_ind - ind) {
						tokens.push_back(Token(TokenType::operand, std::stoi(expression.substr(ind, new_ind - ind))));
						ind = new_ind - 1;
					}
					break;
			}
		}
		tokens.push_back(Token(TokenType::end));

		return tokens;
	}
}


std::function<std::vector<passlang::C_Check>(int, std::string)> initPasslang(std::function<passlang::C_Check(int, int, int)> checkConstructor, std::function<int(int, int)> randrangeCallback) {
	return [checkConstructor, randrangeCallback](int checksNumber, std::string expression) -> std::vector<passlang::C_Check> {
		auto tokens = passlang::tokenize(expression);
		passlang::Parser parser(tokens);
		auto trees = parser.parse();

		passlang::Interpreter interpreter = passlang::Interpreter(checksNumber, checkConstructor, randrangeCallback);

		std::vector<passlang::C_Check> results;
		for (auto i: trees) {
			std::vector<passlang::C_Check> checks = interpreter.eval(i);
			results.insert(results.end(), checks.begin(), checks.end());
		}
		passlang::Deleter::deleteAll();

		return results;
	};
}
