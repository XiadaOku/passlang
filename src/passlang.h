#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdlib>
#include <ctime>
#include <functional>


namespace passlang {
	extern std::vector<std::function<void()>> deletables;
	template<typename T>
	extern void addDeletable(T* object);
	extern void deleteAll();

	template<typename T>
	class Flaged {
	private:
		void* ptr = nullptr;

	public:
		T type;

		Flaged(T type) {
			this->type = type;
		}

		template<typename T1>
		Flaged(T type, T1 value) {
			this->type = type;
			T1* t_ptr = new T1(value);
			ptr = t_ptr;
			addDeletable(t_ptr);
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
	typedef Flaged<TokenType> Token;
	std::vector<Token> tokenize(std::string expression);


	/************* PARSER *************/
	// types
	enum class OperandType {
		number = 0,
		expression,
		randomrange,
		randomchoice,
		numofchecks,
		loopiterator
	};
	typedef Flaged<OperandType> Operand;

	enum class CheckElementType {
		number = 0,
		expression,
		random,
		randomrange,
		randomchoice,
		numofchecks,
		loopiterator
	};
	typedef Flaged<CheckElementType> CheckElement;

	enum class ChecksRowElementType {
		check = 0,
		loop,
		randomcheckchoice
	};
	typedef Flaged<ChecksRowElementType> ChecksRowElement;

	enum class RandomChoiceChanceType {
		none = 0,
		operand
	};
	typedef Flaged<RandomChoiceChanceType> RandomChoiceChance;

	enum class RandomChoiceValueType {
		operand = 0,
		checksrow
	};
	typedef Flaged<RandomChoiceValueType> RandomChoiceValue;

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
	typedef std::vector<RandomChoiceElement> RandomChoice;


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
				int old_index = index;
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
				throw std::runtime_error("error when tryed to parse check");
			}

			CheckElement y = parseCheckElement();

			return ChecksRowElement(ChecksRowElementType::check, Check{world, x, y});
		}

		RandomChoice parseRandomChoice(bool is_checks=false) {
			if (popToken().type != TokenType::openSquareBracket) {
				throw std::runtime_error("waited for [ at the start of randchoice");
			}

			RandomChoice randomChoice;
			while (peekToken().type != TokenType::closeSquareBracket) {
				randomChoice.push_back(parseRandomChoiceElement(is_checks));
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
					throw std::runtime_error("chance must be set after semicolon without spaces");
				}
				chance = RandomChoiceChance(RandomChoiceChanceType::operand, parseOperand());

				if (peekToken().type == TokenType::semicolon) {
					popToken();
					if (peekToken().type == TokenType::space) {
						throw std::runtime_error("equals must be set after semicolon without spaces");
					}
					equals = RandomChoiceChance(RandomChoiceChanceType::operand, parseOperand());
				}
			}

			return RandomChoiceElement{value, chance, equals};
		}

		ChecksRowElement parseLoop(CheckElement length) {
			Operand loopLength = CheckElement2Operand(length);

			if (popToken().type != TokenType::openBracket) {
				throw std::runtime_error("waited for ( in start of loop");
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
				throw std::runtime_error("no known check element like this");
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
			throw std::runtime_error("can't use checkElementType as operandType");
		}

		RandomRange parseRandomRange(Operand start) {
			if (peekToken().type != TokenType::operand && peekToken().get<std::string>() != "-") {
				throw std::runtime_error("waited - in randrange");
			}
			popToken();

			Operand finish = parseOperand(true);

			return RandomRange{start, finish};
		};

		ExpressionNode parseExpression() {
			skipSpace();
			Token openBracket = popToken();
			if (openBracket.type != TokenType::openBracket) {
				throw std::runtime_error("where's my (");
			}

			std::vector<Operand> operands = {parseOperand()};
			std::vector<std::string> operations;

			do {
				skipSpace();
				Token operation = popToken();
				if (operation.type != TokenType::operation) {
					throw std::runtime_error("where's my +");
				}
				operations.push_back(operation.get<std::string>());

				operands.push_back(parseOperand());
				skipSpace();
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
				throw std::runtime_error("no such operand");
			}

			if (peekToken().type == TokenType::operation && peekToken().get<std::string>() == "-") {
				if (is_finish) {
					throw std::runtime_error("randrange takes only two points");
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
	typedef Flaged<RandomChoiceResultType> RandomChoiceResult;


	class Interpreter {
	private:
		std::vector<int> loopIterators;
		std::function<C_Check(int, int, int)> checkConstructor;

	public:
		int numberOfChecks;

		Interpreter(int numberOfChecks, std::function<C_Check(int, int, int)> checkConstructor) {
			std::srand(std::time(nullptr));
			this->numberOfChecks = numberOfChecks;
			this->checkConstructor = checkConstructor;
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

			throw std::runtime_error("no known type like this for checkRowElement");
		}

		RandomChoiceResult eval(RandomChoice randomChoice) {
			float random = (rand() % 100) + 1;
			float chanceOnFree = 0;
			int freeChance = 100;
			int freeElements = 0;

			for (int i = 0; i < randomChoice.size(); i++) {
				if (randomChoice[i].equals.type == RandomChoiceChanceType::operand) {
					if (eval(randomChoice[i].chance.get<Operand>()) == eval(randomChoice[i].equals.get<Operand>())) {
						return eval(randomChoice[i]);
					}
				}
				else if (randomChoice[i].chance.type == RandomChoiceChanceType::operand) {
					freeChance -= eval(randomChoice[i].chance.get<Operand>());
				}
				else {
					freeElements++;
				}
			}
			if (freeChance < 0) {
				throw std::runtime_error("used more than 100 percents as chances");
			}

			if (freeElements) {
				chanceOnFree = (float)freeChance / freeElements;
				if (chanceOnFree == 0 && freeChance > 0) {
					throw std::runtime_error("too small chance for free elements, can't use");
				}
			}

			float chance = 0;
			for (int i = 0; i < randomChoice.size(); i++) {
				if (randomChoice[i].equals.type == RandomChoiceChanceType::operand) {
					continue;
				}
				else if (randomChoice[i].chance.type == RandomChoiceChanceType::operand) {
					chance += eval(randomChoice[i].chance.get<Operand>());
				}
				else {
					chance += chanceOnFree;
				}

				if (chance >= random) {
					return eval(randomChoice[i]);
				}
			}

			if (randomChoice[0].value.type == RandomChoiceValueType::checksrow) {
				return RandomChoiceResult(RandomChoiceResultType::vector, std::vector<C_Check>());
			}
			throw std::runtime_error("can't choose anything in randomchoice");
		}

		RandomChoiceResult eval(RandomChoiceElement randomChoiceElement) {
			if (randomChoiceElement.value.type == RandomChoiceValueType::operand) {
				return RandomChoiceResult(RandomChoiceResultType::number, eval(randomChoiceElement.value.get<Operand>()));
			}
			else if (randomChoiceElement.value.type == RandomChoiceValueType::checksrow) {
				return RandomChoiceResult(RandomChoiceResultType::vector, eval(randomChoiceElement.value.get<ChecksRowElement>()));
			}
			throw std::runtime_error("can't use this RandomChoiceValueType");
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
				throw std::runtime_error("finish can't be lower than start of randrage");
			}

			return start + (std::rand() % (finish - start + 1));
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
			throw std::runtime_error("not known checkElement type");
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

			throw std::runtime_error(std::string("no such operator ") + expression.operation);
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

			throw std::runtime_error("can't eval operand");
		}

		int getIterator(int index) {
			if (index < 0 || index >= loopIterators.size()) {
				throw std::runtime_error("no loop with such iterator");
			}
			return loopIterators[index];
		}
	};
}

std::function<std::vector<passlang::C_Check>(int, std::string)> initPasslang(std::function<passlang::C_Check(int, int, int)> checkConstructor);
