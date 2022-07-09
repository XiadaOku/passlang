#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>


enum class TokenType {
	openBracket = 0,
	closeBracket,
	operand,
	operation
};

class Token {
private:
	void* ptr = nullptr;

public:
	TokenType type;

	Token(TokenType type) {
		this->type = type;
	}

	template<typename T>
	Token(TokenType type, T value) {
		this->type = type;
		ptr = new T(value);
	}
	
	template<typename T>
	T get() {
		return *((T*)ptr);
	}
};

std::vector<Token> tokenize(std::string expression) {
	std::vector<Token> tokens;

	for (size_t ind = 0; ind < expression.length(); ind++) {
		char i = expression[ind];
		switch (i) {
			case '(':
				tokens.push_back(Token(TokenType::openBracket));
				break;
			case ')':
				tokens.push_back(Token(TokenType::closeBracket));
				break;
			case '+':
			case '-':
			case '*':
			case '/':
				tokens.push_back(Token(TokenType::operation, std::string(1, i)));
				break;
			default:
				size_t new_ind = ind;
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
	
	return tokens;
}

enum class OperandType {
	number = 0,
	expression
};

class Operand {
private:
	void* ptr;

public:
	OperandType type;

	template<typename T>
	Operand(OperandType type, T value) {
		this->type = type;
		this->ptr = new T(value);
	}
	
	template<typename T>
	T get() {
		return *((T*)ptr);
	}
};

struct ExpressionNode {
	Operand firstOperand;
	std::string operation;
	Operand secondOperand;
	int color = 0;
};

class Parser {
private:
	size_t index = 0;

public:
	std::vector<Token> tokens;

	Parser(std::vector<Token> tokens) {
		this->tokens = tokens;
	}

	Token peekToken() {
		if (index >= tokens.size()) {
			throw std::runtime_error("aaaaaaaaa");
		}
		return tokens[index];
	}

	Token popToken() {
		if (index >= tokens.size()) {
			throw std::runtime_error("aaaa");
		}
		Token token = tokens[index++];
		return token;
	}

	ExpressionNode parse() {
		Token openBracket = popToken();
		if (openBracket.type != TokenType::openBracket) {
			throw std::runtime_error("where's my (");
		}

		std::vector<Operand> operands = {parseOperand()};
		std::vector<std::string> operations;
				
		do {
			Token operation = popToken();
			if (operation.type != TokenType::operation) {
				throw std::runtime_error("where's my +");
			}
			operations.push_back(operation.getString());

			operands.push_back(parseOperand());
		} while (peekToken().type != TokenType::closeBracket);
		popToken(); // closeBracket
		
		std::vector<std::string> plus_operations, minus_operations;
		std::vector<Operand> plus_operands, minus_operands;
		
		for (int i = 0; i < operations.size(); i++) {
			int j = i;
			while (j < operations.size() && (operations[j] == "*" || operations[j] == "/")) {
				j++;
			}
			
			if (j > i) {
				if (i == 0 || operations[i - 1] == "+") {
					plus_operations.insert(plus_operations.end(), operations.begin() + i, operations.begin() + j);
					plus_operands.insert(plus_operands.end(), operands.begin() + i, operands.begin() + j + 1);
				}
				else if (operations[i - 1] == "-") {
					minus_operations.insert(minus_operations.end(), operations.begin() + i, operations.begin() + j);
					minus_operands.insert(minus_operands.end(), operands.begin() + i, operands.begin() + j + 1);
				}
				operations.erase(operations.begin() + i - (i != 0), operations.begin() + j);
				operands.erase(operands.begin() + i, operands.begin() + j + 1);
				i -= 1;
			}
		}
		
		Operand op_null = Operand(OperandType::number, new int(0));
		
		ExpressionNode node{op_null, "+", op_null};
		if (operations.size() == 0) {
			Operand op = op_null;
			if (operands.size()) {
				op = operands[0];
			}
			node = ExpressionNode{op_null, "+", op};
		}
		else {
			node = ExpressionNode{operands[0], operations[0], operands[1]};
			for (size_t i = 1; i < operations.size(); i++) {
				node = ExpressionNode{Operand(OperandType::expression, new ExpressionNode(node)), operations[i], operands[i + 1]};
			}
		}
		
		ExpressionNode wrapper = ExpressionNode{op_null, "+", op_null};
		if (minus_operations.size()) {
			ExpressionNode minus_node = ExpressionNode{minus_operands[0], minus_operations[0], minus_operands[1]};
			for (size_t i = 1; i < minus_operations.size(); i++) {
				minus_node = ExpressionNode{Operand(OperandType::expression, new ExpressionNode(minus_node)), minus_operations[i], minus_operands[i + 1]};
			}
			wrapper = ExpressionNode{Operand(OperandType::expression, new ExpressionNode(wrapper)), "-", Operand(OperandType::expression, new ExpressionNode(minus_node))};
		}
		if (plus_operations.size()) {
			ExpressionNode plus_node = ExpressionNode{plus_operands[0], plus_operations[0], plus_operands[1]};
			for (size_t i = 1; i < plus_operations.size(); i++) {
				plus_node = ExpressionNode{Operand(OperandType::expression, new ExpressionNode(plus_node)), plus_operations[i], plus_operands[i + 1]};
			}
			wrapper = ExpressionNode{Operand(OperandType::expression, new ExpressionNode(wrapper)), "+", Operand(OperandType::expression, new ExpressionNode(plus_node))};
		}
		node = ExpressionNode{Operand(OperandType::expression, new ExpressionNode(wrapper)), "+", Operand(OperandType::expression, new ExpressionNode(node))};
		
		return node;
	}

	Operand parseOperand() {
		Token token = peekToken();
		if (token.type == TokenType::operand) {
			return Operand(OperandType::number, new int(popToken().getInt()));
		}
		else if (token.type == TokenType::openBracket) {
			return Operand(OperandType::expression, new ExpressionNode(parse()));
		}
		else {
			throw std::runtime_error("no such operand");
		}
	}
};

class Interpreter {
public:
	static int eval(ExpressionNode expression) {
		int firstOperand = eval(expression.firstOperand);
		int secondOperand = eval(expression.secondOperand);
		
		if (expression.operation == "+") {
			return firstOperand + secondOperand;
		}
		if (expression.operation == "-") {
			return firstOperand - secondOperand;
		}
		if (expression.operation == "*") {
			return firstOperand * secondOperand;
		}
		if (expression.operation == "/") {
			return firstOperand / secondOperand;
		}
		
		throw std::runtime_error(std::string("no such operator ") + expression.operation);
	}
	
	static int eval(Operand operand) {
		if (operand.type == OperandType::number) {
			return operand.get<int>();
		}
		if (operand.type == OperandType::expression) {
			return eval(operand.get<ExpressionNode>());
		}
		
		throw std::runtime_error("can't eval operand");
	}
};

void dfs(ExpressionNode node) {
	node.color = 1;
	std::cout << "(";
	if (node.firstOperand.type == OperandType::expression) {
		dfs(node.firstOperand.get<ExpressionNode>());
	}
	else {
		std::cout << node.firstOperand.get<int>();
	}
	
	std::cout << " " << node.operation << " ";
	
	if (node.secondOperand.type == OperandType::expression) {
		dfs(node.secondOperand.get<ExpressionNode>());
	}
	else {
		std::cout << node.secondOperand.get<int>();
	}
	std::cout << ")";
}

// (2 + 1 + (4 + 6) / 10) -> 4
int main(int argc, char** args) {
	std::string expression;
	std::getline(std::cin, expression);
	
	auto tokens = tokenize(expression);
	Parser parser(tokens);
	auto tree = parser.parse();
	
	dfs(tree);
	std::cout << std::endl;
	
	std::cout << Interpreter::eval(tree) << std::endl;

	return 0;
}
