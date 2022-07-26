#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <functional>


namespace passlang {
	namespace Deleter {
		template<typename T>
		void addDeletable(T* object);
		void deleteAll();
	}

	template<typename T>
	class TypeHolder {
	private:
		void* ptr = nullptr;

	public:
		T type;

		TypeHolder(T type) {
			this->type = type;
		}

		template<typename T1>
		TypeHolder(T type, T1 value) {
			this->type = type;
			T1* t_ptr = new T1(value);
			ptr = t_ptr;
			Deleter::addDeletable(t_ptr);
		}

		template<typename T1>
		T1 get() {
			return *((T1*)ptr);
		}
	};


	/************* TOKENIZER *************/
	enum class TokenType {
		openBracket = 0,
		closeBracket,
		operand,
		operation,
		space,
		checkSeparator,
		openSquareBracket,
		closeSquareBracket,
		semicolon,
		numofChecksVariable,
		loopIteratorVariable,
		end
	};
	typedef TypeHolder<TokenType> Token;
	std::vector<Token> tokenize(std::string expression);


	/************* PARSER *************/
	enum class OperandType {
		number = 0,
		expression,
		randomrange,
		randomchoice,
		numofchecks,
		loopiterator
	};
	typedef TypeHolder<OperandType> Operand;

	enum class CheckElementType {
		number = 0,
		expression,
		random,
		randomrange,
		randomchoice,
		numofchecks,
		loopiterator
	};
	typedef TypeHolder<CheckElementType> CheckElement;

	enum class ChecksRowElementType {
		check = 0,
		loop,
		randomcheckchoice
	};
	typedef TypeHolder<ChecksRowElementType> ChecksRowElement;

	enum class RandomChoiceChanceType {
		none = 0,
		operand
	};
	typedef TypeHolder<RandomChoiceChanceType> RandomChoiceChance;

	enum class RandomChoiceValueType {
		operand = 0,
		checksrow
	};
	typedef TypeHolder<RandomChoiceValueType> RandomChoiceValue;

	struct ExpressionNode {
		Operand firstOperand;
		std::string operation;
		Operand secondOperand;
	};

	struct Check {
		CheckElement world;
		CheckElement x, y;
	};

	struct Loop {
		Operand length;
		std::vector<ChecksRowElement> checks;
	};

	struct RandomRange {
		Operand start;
		Operand finish;
	};

	struct RandomChoiceElement {
		RandomChoiceValue value;
		RandomChoiceChance chance;
		RandomChoiceChance equals;
	};
	struct RandomChoice {
		RandomChoiceValueType type;
		std::vector<RandomChoiceElement> choices;
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
				throw std::runtime_error("Parser::peekToken: out of bounds");
			}
			return tokens[index];
		}

		Token popToken() {
			if (index >= tokens.size()) {
				throw std::runtime_error("Parser::popToken: out of bounds");
			}
			return tokens[index++];
		}

		void skipSpace() {
			while (peekToken().type == TokenType::space) {
				popToken();
			}
		}

		auto parse() {
			return parseChecksRow();
		}

		std::vector<ChecksRowElement> parseChecksRow() {
			std::vector<ChecksRowElement> checks;
			while (peekToken().type != TokenType::end && peekToken().type != TokenType::closeBracket) {
				checks.push_back(parseCheck());
				skipSpace();
			}
			popToken();
			return checks;
		}

		ChecksRowElement parseCheck() {
			skipSpace();

			CheckElement rand = CheckElement(CheckElementType::random);

			if (peekToken().type == TokenType::openSquareBracket) {
				size_t old_index = index;
				RandomChoice randomCheckChoice = parseRandomChoice(true);
				if (peekToken().type == TokenType::checkSeparator) {
					index = old_index;
				}
				else {
					return ChecksRowElement(ChecksRowElementType::randomcheckchoice, randomCheckChoice);
				}
			}

			CheckElement world = parseCheckElement();
			if (peekToken().type != TokenType::checkSeparator) {
				if (peekToken().type == TokenType::openBracket) {
					return parseLoop(world);
				}
				else {
					return ChecksRowElement(ChecksRowElementType::check, Check{world, rand, rand});
				}
			}
			popToken();

			CheckElement x = parseCheckElement();
			if (popToken().type != TokenType::checkSeparator) {
				throw std::runtime_error("Parser::parseCheck: can't find \".\" after x coordinate");
			}

			CheckElement y = parseCheckElement();

			return ChecksRowElement(ChecksRowElementType::check, Check{world, x, y});
		}

		RandomChoice parseRandomChoice(bool is_checks=false) {
			if (popToken().type != TokenType::openSquareBracket) {
				throw std::runtime_error("Parser::parseRandomChoice: can't find \"[\" at the start");
			}
			skipSpace();

			RandomChoice randomChoice;
			if (is_checks) {
				randomChoice.type = RandomChoiceValueType::checksrow;
			}
			else {
				randomChoice.type = RandomChoiceValueType::operand;
			}

			while (peekToken().type != TokenType::closeSquareBracket) {
				randomChoice.choices.push_back(parseRandomChoiceElement(is_checks));
				skipSpace();
			}
			popToken(); // closeSquareBracket

			return randomChoice;
		}

		RandomChoiceElement parseRandomChoiceElement(bool is_check=false) {
			skipSpace();

			RandomChoiceValue value = RandomChoiceValue(RandomChoiceValueType::operand);
			if (is_check) {
				value = RandomChoiceValue(RandomChoiceValueType::checksrow, parseCheck());
			}
			else {
				value = RandomChoiceValue(RandomChoiceValueType::operand, parseOperand());
			}

			RandomChoiceChance chance = RandomChoiceChance(RandomChoiceChanceType::none);
			RandomChoiceChance equals = RandomChoiceChance(RandomChoiceChanceType::none);
			if (peekToken().type == TokenType::semicolon) {
				popToken();
				if (peekToken().type == TokenType::space) {
					throw std::runtime_error("Parser::parseRandomChoiceElement: chance must be set after semicolon without spaces");
				}
				chance = RandomChoiceChance(RandomChoiceChanceType::operand, parseOperand());

				if (peekToken().type == TokenType::semicolon) {
					popToken();
					if (peekToken().type == TokenType::space) {
						throw std::runtime_error("Parser::parseRandomChoiceElement: equalable must be set after semicolon without spaces");
					}
					equals = RandomChoiceChance(RandomChoiceChanceType::operand, parseOperand());
				}
			}

			return RandomChoiceElement{value, chance, equals};
		}

		ChecksRowElement parseLoop(CheckElement length) {
			Operand loopLength = CheckElement2Operand(length);

			if (popToken().type != TokenType::openBracket) {
				throw std::runtime_error("Parser::parseLoop: can't find \"(\" at the start");
			}
			Loop loop{loopLength, parseChecksRow()};
			return ChecksRowElement(ChecksRowElementType::loop, loop);
		}

		CheckElement parseCheckElement() {
			Token token = peekToken();
			CheckElement checkElement = CheckElement(CheckElementType::number);

			if (token.type == TokenType::openBracket) {
				checkElement = CheckElement(CheckElementType::expression, parseExpression());
			}
			else if (token.type == TokenType::openSquareBracket) {
				checkElement = CheckElement(CheckElementType::randomchoice, parseRandomChoice());
			}
			else if (token.type == TokenType::operand) {
				checkElement = CheckElement(CheckElementType::number, popToken().get<int>());
			}
			else if (token.type == TokenType::numofChecksVariable) {
				checkElement = CheckElement(CheckElementType::numofchecks);
				popToken();
			}
			else if (token.type == TokenType::loopIteratorVariable) {
				checkElement = CheckElement(CheckElementType::loopiterator, popToken().get<int>());
			}
			else if (token.type == TokenType::operation && token.get<std::string>() == "-") {
				checkElement = CheckElement(CheckElementType::random);
				popToken();
				return checkElement;
			}
			else {
				throw std::runtime_error("Parser::parseCheckElement: can't use given Token");
			}

			if (peekToken().type == TokenType::operation && peekToken().get<std::string>() == "-") {
				return CheckElement(CheckElementType::randomrange, parseRandomRange(CheckElement2Operand(checkElement)));
			}
			return checkElement;
		}

		Operand CheckElement2Operand(CheckElement checkElement) {
			if (checkElement.type == CheckElementType::number) {
				return Operand(OperandType::number, checkElement.get<int>());
			}
			else if (checkElement.type == CheckElementType::expression) {
				return Operand(OperandType::expression, checkElement.get<ExpressionNode>());
			}
			else if (checkElement.type == CheckElementType::randomrange) {
				return Operand(OperandType::randomchoice, checkElement.get<RandomRange>());
			}
			else if (checkElement.type == CheckElementType::randomchoice) {
				return Operand(OperandType::randomchoice, checkElement.get<RandomChoice>());
			}
			else if (checkElement.type == CheckElementType::numofchecks) {
				return Operand(OperandType::numofchecks);
			}
			else if (checkElement.type == CheckElementType::loopiterator) {
				return Operand(OperandType::loopiterator, checkElement.get<int>());
			}
			throw std::runtime_error("Parser::CheckElement2Operand: can't use given CheckElement");
		}

		RandomRange parseRandomRange(Operand start) {
			if (peekToken().type != TokenType::operand && peekToken().get<std::string>() != "-") {
				throw std::runtime_error("Parser::parseRandomRange: can't find \"-\" after first operand");
			}
			popToken();

			Operand finish = parseOperand(true);

			return RandomRange{start, finish};
		};

		ExpressionNode parseExpression() {
			skipSpace();
			Token openBracket = popToken();
			if (openBracket.type != TokenType::openBracket) {
				throw std::runtime_error("Parser::parseExpression: can't find \"(\" at the start");
			}

			std::vector<Operand> operands = {parseOperand()};
			std::vector<std::string> operations;

			do {
				skipSpace();
				Token operation = popToken();
				if (operation.type != TokenType::operation) {
					throw std::runtime_error("Parser::parseExpression: can't find operation after operand");
				}
				operations.push_back(operation.get<std::string>());

				operands.push_back(parseOperand());
				skipSpace();
			} while (peekToken().type != TokenType::closeBracket);
			popToken(); // closeBracket

			// Builds list of operations and operands into nodes, sorts them by math rules
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

			Operand op_null = Operand(OperandType::number, 0);

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
					node = ExpressionNode{Operand(OperandType::expression, node), operations[i], operands[i + 1]};
				}
			}

			ExpressionNode wrapper = ExpressionNode{op_null, "+", op_null};
			if (minus_operations.size()) {
				ExpressionNode minus_node = ExpressionNode{minus_operands[0], minus_operations[0], minus_operands[1]};
				for (size_t i = 1; i < minus_operations.size(); i++) {
					minus_node = ExpressionNode{Operand(OperandType::expression, minus_node), minus_operations[i], minus_operands[i + 1]};
				}
				wrapper = ExpressionNode{Operand(OperandType::expression, wrapper), "-", Operand(OperandType::expression, minus_node)};
			}
			if (plus_operations.size()) {
				ExpressionNode plus_node = ExpressionNode{plus_operands[0], plus_operations[0], plus_operands[1]};
				for (size_t i = 1; i < plus_operations.size(); i++) {
					plus_node = ExpressionNode{Operand(OperandType::expression, plus_node), plus_operations[i], plus_operands[i + 1]};
				}
				wrapper = ExpressionNode{Operand(OperandType::expression, wrapper), "+", Operand(OperandType::expression, plus_node)};
			}
			node = ExpressionNode{Operand(OperandType::expression, wrapper), "+", Operand(OperandType::expression, node)};

			return node;
		}

		Operand parseOperand(bool is_finish=false) {
			skipSpace();
			Token token = peekToken();
			Operand operand = Operand(OperandType::number);

			if (token.type == TokenType::operand) {
				operand = Operand(OperandType::number, popToken().get<int>());
			}
			else if (token.type == TokenType::openBracket) {
				operand = Operand(OperandType::expression, parseExpression());
			}
			else if (token.type == TokenType::openSquareBracket) {
				operand = Operand(OperandType::randomchoice, parseRandomChoice());
			}
			else if (token.type == TokenType::numofChecksVariable) {
				operand = Operand(OperandType::numofchecks);
				popToken();
			}
			else if (token.type == TokenType::loopIteratorVariable) {
				operand = Operand(OperandType::loopiterator, popToken().get<int>());
			}
			else {
				throw std::runtime_error("Parser::parseOperand: can't use given Token");
			}

			if (peekToken().type == TokenType::operation && peekToken().get<std::string>() == "-") {
				if (is_finish) {
					throw std::runtime_error("Parser::parseOperand: randrange takes only 2 points, but second \"-\" was found");
				}
				return Operand(OperandType::randomrange, parseRandomRange(operand));
			}
			return operand;
		}
	};


	/************* INTERPRETER *************/
	const int randomPlaceholder = -1;

	struct C_Check {
		int world;
		int x, y;
	};
	extern std::ostream& operator<<(std::ostream& stream, C_Check check);

	enum class RandomChoiceResultType {
		number = 0,
		vector
	};
	typedef TypeHolder<RandomChoiceResultType> RandomChoiceResult;


	class Interpreter {
	private:
		std::vector<int> loopIterators;
		std::function<C_Check(int, int, int)> checkConstructor;
		std::function<int(int, int)> randrangeCallback;

	public:
		int numberOfChecks;

		Interpreter(int numberOfChecks, std::function<C_Check(int, int, int)> checkConstructor, std::function<int(int, int)> randrangeCallback) {
			this->numberOfChecks = numberOfChecks;
			this->checkConstructor = checkConstructor;
			this->randrangeCallback = randrangeCallback;
		}


		std::vector<C_Check> eval(ChecksRowElement check) {
			if (check.type == ChecksRowElementType::check) {
				return {eval(check.get<Check>())};
			}
			else if (check.type == ChecksRowElementType::loop) {
				return eval(check.get<Loop>());
			}
			else if (check.type == ChecksRowElementType::randomcheckchoice) {
				return eval(check.get<RandomChoice>()).get<std::vector<C_Check>>();
			}

			throw std::runtime_error("Interpreter::evalChecksRowElement: can't use given ChecksRowElement");
		}

		RandomChoiceResult eval(RandomChoice randomChoice) {
			float random = (rand() % 100) + 1;
			float chanceOnFree = 0;
			int freeChance = 100;
			int freeElements = 0;

			for (int i = 0; i < randomChoice.choices.size(); i++) {
				if (randomChoice.choices[i].equals.type == RandomChoiceChanceType::operand) {
					if (eval(randomChoice.choices[i].chance.get<Operand>()) == eval(randomChoice.choices[i].equals.get<Operand>())) {
						return eval(randomChoice.choices[i]);
					}
				}
				else if (randomChoice.choices[i].chance.type == RandomChoiceChanceType::operand) {
					freeChance -= eval(randomChoice.choices[i].chance.get<Operand>());
				}
				else {
					freeElements++;
				}
			}
			if (freeChance < 0) {
				throw std::runtime_error("Interpreter::evalRandomChoice: used more than 100 percents as chances");
			}

			if (freeElements) {
				chanceOnFree = (float)freeChance / freeElements;
				if (chanceOnFree < 0.0001 && freeChance > 0) {
					throw std::runtime_error("Interpreter::evalRandomChoide: too small chance for free elements, can't use");
				}
			}

			float chance = 0;
			for (int i = 0; i < randomChoice.choices.size(); i++) {
				if (randomChoice.choices[i].equals.type == RandomChoiceChanceType::operand) {
					continue;
				}
				else if (randomChoice.choices[i].chance.type == RandomChoiceChanceType::operand) {
					chance += eval(randomChoice.choices[i].chance.get<Operand>());
				}
				else {
					chance += chanceOnFree;
				}

				if (chance >= random) {
					return eval(randomChoice.choices[i]);
				}
			}

			if (randomChoice.type == RandomChoiceValueType::checksrow) {
				return RandomChoiceResult(RandomChoiceResultType::vector, std::vector<C_Check>());
			}
			throw std::runtime_error("Interpreter::evalRandomChoice: can't choose item");
		}

		RandomChoiceResult eval(RandomChoiceElement randomChoiceElement) {
			if (randomChoiceElement.value.type == RandomChoiceValueType::operand) {
				return RandomChoiceResult(RandomChoiceResultType::number, eval(randomChoiceElement.value.get<Operand>()));
			}
			else if (randomChoiceElement.value.type == RandomChoiceValueType::checksrow) {
				return RandomChoiceResult(RandomChoiceResultType::vector, eval(randomChoiceElement.value.get<ChecksRowElement>()));
			}
			throw std::runtime_error("Interpreter::evalRandomChoiceElement: can't use given RandomChoiceElement");
		}

		std::vector<C_Check> eval(Loop loop) {
			std::vector<C_Check> checks;
			int length = eval(loop.length);
			int loop_length = loop.checks.size();

			loopIterators.push_back(0);
			int iteratorIndex = loopIterators.size() - 1;

			for (int j = 0; j < length; j++) {
				for (int i = 0; i < loop_length; i++) {
					std::vector<C_Check> check = eval(loop.checks[i]);
					checks.insert(checks.end(), check.begin(), check.end());
					loopIterators[iteratorIndex]++;
				}
			}

			loopIterators.pop_back();

			return checks;
		}

		int eval(RandomRange randomRange) {
			int start = eval(randomRange.start);
			int finish = eval(randomRange.finish);

			if (finish < start) {
				int temp = start;
				start = finish;
				finish = temp;
			}
			return randrangeCallback(start, finish);
		}

		int eval(CheckElement checkElement) {
			if (checkElement.type == CheckElementType::number) {
				return checkElement.get<int>();
			}
			else if (checkElement.type == CheckElementType::expression) {
				return eval(checkElement.get<ExpressionNode>());
			}
			else if (checkElement.type == CheckElementType::random) {
				return randomPlaceholder;
			}
			else if (checkElement.type == CheckElementType::randomrange) {
				return eval(checkElement.get<RandomRange>());
			}
			else if (checkElement.type == CheckElementType::randomchoice) {
				return eval(checkElement.get<RandomChoice>()).get<int>();
			}
			else if (checkElement.type == CheckElementType::numofchecks) {
				return numberOfChecks;
			}
			else if (checkElement.type == CheckElementType::loopiterator) {
				return getIterator(checkElement.get<int>());
			}
			throw std::runtime_error("Interpreter::evalCheckElement: can't use given CheckElement");
		}

		C_Check eval(Check check) {
			int world, x, y;

			world = eval(check.world);
			x = eval(check.x);
			y = eval(check.y);
			C_Check c_check = checkConstructor(world, x, y);

			return c_check;
		}

		int eval(ExpressionNode expression) {
			int firstOperand = eval(expression.firstOperand);
			int secondOperand = eval(expression.secondOperand);

			if (expression.operation == "+") {
				return firstOperand + secondOperand;
			}
			else if (expression.operation == "-") {
				return firstOperand - secondOperand;
			}
			else if (expression.operation == "*") {
				return firstOperand * secondOperand;
			}
			else if (expression.operation == "/") {
				return firstOperand / secondOperand;
			}
			else if (expression.operation == "%") {
				return firstOperand % secondOperand;
			}

			throw std::runtime_error(std::string("Interpreter::parseExpression: can't use given operator: ") + expression.operation);
		}

		int eval(Operand operand) {
			if (operand.type == OperandType::number) {
				return operand.get<int>();
			}
			else if (operand.type == OperandType::expression) {
				return eval(operand.get<ExpressionNode>());
			}
			else if (operand.type == OperandType::randomrange) {
				return eval(operand.get<RandomRange>());
			}
			else if (operand.type == OperandType::randomchoice) {
				return eval(operand.get<RandomChoice>()).get<int>();
			}
			else if (operand.type == OperandType::numofchecks) {
				return numberOfChecks;
			}
			else if (operand.type == OperandType::loopiterator) {
				return getIterator(operand.get<int>());
			}

			throw std::runtime_error("Interpreter::evalOperand: can't use given Operand");
		}

		int getIterator(int index) {
			if (index < 0 || index >= loopIterators.size()) {
				throw std::runtime_error(std::string("Interpreter::getInterator: can't find loop with iterator: i") + std::to_string(index));
			}
			return loopIterators[index];
		}
	};
}

std::function<std::vector<passlang::C_Check>(int, std::string)> initPasslang(std::function<passlang::C_Check(int, int, int)> checkConstructor, std::function<int(int, int)> randrangeCallback);
