#include "../formula.h";
#include "FormulaBaseListener.h"
#include "FormulaLexer.h"
#include "FormulaListener.h"
#include "FormulaParser.h"
#include "../build/MyFormulaListener.h"
#include <optional>

using namespace std;

class BailErrorListener : public antlr4::BaseErrorListener {
public:
	void syntaxError(antlr4::Recognizer* /* recognizer */,
		antlr4::Token* /* offendingSymbol */, size_t /* line */,
		size_t /* charPositionInLine */, const std::string& msg,
		std::exception_ptr /* e */
	) override {
		throw std::runtime_error("Error when lexing: " + msg);
	}
};

Ast GetContext(std::istream& in) {
	antlr4::ANTLRInputStream input(in);

	FormulaLexer lexer(&input);
	BailErrorListener error_listener;
	lexer.removeErrorListeners();
	lexer.addErrorListener(&error_listener);

	antlr4::CommonTokenStream tokens(&lexer);

	FormulaParser parser(&tokens);
	auto error_handler = std::make_shared<antlr4::BailErrorStrategy>();
	parser.setErrorHandler(error_handler);
	parser.removeErrorListeners();

	antlr4::tree::ParseTree* tree = parser.main(); // метод соответствует корневому правилу
	MyFormulaListener listener;
	try {
		antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);
	}
	catch (...) {
		throw FormulaException("");
	}
	auto ast = listener.GetResult();
	return ast;
}

class Formula : public IFormula {
private:
	Ast ast;
	std::string expr;
	std::vector<Position> ref_cells;
	std::string formatted_expr;
	HandlingResult res = HandlingResult::NothingChanged;
	optional<Value> value;

	void UpdateData() {
		istringstream in(expr);
		try {
			ast = GetContext(in);
		}
		catch (...) {
			throw FormulaException("");
		}
		ref_cells = { ast.ref_cells.begin(), ast.ref_cells.end() };
		expr = ast.GetExpression();
	}
public:
	Formula(std::string expr) 
		: expr(expr) {
		UpdateData();
	}

	Value Evaluate(const ISheet& sheet) const {
		try {
			return ast.vertexes.top()->Evaluate(sheet);
		}
		//If cell contain non number/cell value
		catch (FormulaError::Category c) {
			return c;
		}
	}
	std::string GetExpression() const {
		return expr;
	}
	std::vector<Position> GetReferencedCells() const {
		return ref_cells;
	}

	HandlingResult HandleInsertedRows(int before, int count = 1) {
		res = HandlingResult::NothingChanged;
		if (count == 0) {
			return res;
		}
		bool changed = false;
		for (auto& cell : ref_cells) {
			if (before <= cell.row) {
				Position old = cell;
				if (cell.row + count >= Position::kMaxCols)
					throw TableTooBigException("cell.row + count");
				cell.row += count;
				changed = true;
				size_t pos = 0;
				while (pos != std::string::npos) {
					pos = expr.find(old.ToString());
					if (pos == std::string::npos)
						break;
					expr.replace(
						expr.begin() + pos, 
						expr.begin() + pos + old.ToString().size(), 
						cell.ToString());
				}
			}
		}
		if (changed) {
			if (res != HandlingResult::ReferencesChanged)
				res = HandlingResult::ReferencesRenamedOnly;
			if (res == HandlingResult::ReferencesRenamedOnly) {
				UpdateData();
			}
			return res;
		}
		return res;
	}
	HandlingResult HandleInsertedCols(int before, int count = 1) {
		res = HandlingResult::NothingChanged;
		if (count == 0) {
			return res;
		}
		bool changed = false;
		for (auto& cell : ref_cells) {
			if (before <= cell.col) {
				Position old = cell;
				if (cell.col + count >= Position::kMaxCols)
					throw TableTooBigException("cell.col + count");
				cell.col += count;
				changed = true;
				size_t pos = 0;
				while (pos != std::string::npos) {
					pos = expr.find(old.ToString());
					if (pos == std::string::npos)
						break;
					expr.replace(
						expr.begin() + pos,
						expr.begin() + pos + old.ToString().size(),
						cell.ToString());
				}
			}
		}
		if (changed) {
			if (res != HandlingResult::ReferencesChanged)
				res = HandlingResult::ReferencesRenamedOnly;
			if (res == HandlingResult::ReferencesRenamedOnly) {
				UpdateData();
			}
			return res;
		}
		return res;
	}
	HandlingResult HandleDeletedRows(int first, int count = 1) {
		res = HandlingResult::NothingChanged;
		if (count == 0) {
			return res;
		}
		vector<int> error_refs_indexes;
		for (size_t i = 0; i < ref_cells.size(); ++i) {
			auto& cell = ref_cells[i];
			if (cell.row >= first && cell.row < first + count) {
				auto error = FormulaError(FormulaError::Category::Ref);
				Position old = cell;
				size_t pos = 0;
				while (pos != std::string::npos) {
					pos = expr.find(old.ToString());
					if (pos == std::string::npos)
						break;
					expr.replace(
						expr.begin() + pos,
						expr.begin() + pos + old.ToString().size(),
						error.ToString());
				}
				formatted_expr = expr;
				res = HandlingResult::ReferencesChanged;
				error_refs_indexes.push_back(i);
			}
			else if (cell.row >= first + count) {
				Position old = cell;
				cell.row -= count;
				size_t pos = 0;
				while (pos != std::string::npos) {
					pos = expr.find(old.ToString());
					if (pos == std::string::npos)
						break;
					expr.replace(
						expr.begin() + pos,
						expr.begin() + pos + old.ToString().size(),
						cell.ToString());
				}
				if(res != HandlingResult::ReferencesChanged)
					res = HandlingResult::ReferencesRenamedOnly;
			}
		}
		for (int index : error_refs_indexes) {
			ref_cells.erase(ref_cells.begin() + index);
		}
		//UpdateData();
		if (res == HandlingResult::ReferencesChanged) {
			value = FormulaError::Category::Ref;
		}
		if (res == HandlingResult::ReferencesRenamedOnly) {
			UpdateData();
		}
		return res;
	}
	HandlingResult HandleDeletedCols(int first, int count = 1) {
		res = HandlingResult::NothingChanged;
		if (count == 0) {
			return res;
		}
		vector<int> error_refs_indexes;
		for (size_t i = 0; i < ref_cells.size(); ++i) {
			auto& cell = ref_cells[i];
			if (cell.col >= first && cell.col < first + count) {
				auto error = FormulaError(FormulaError::Category::Ref);
				Position old = cell;
				size_t pos = 0;
				while (pos != std::string::npos) {
					pos = expr.find(old.ToString());
					if (pos == std::string::npos)
						break;
					expr.replace(
						expr.begin() + pos,
						expr.begin() + pos + old.ToString().size(),
						error.ToString());
				}
				formatted_expr = expr;
				res = HandlingResult::ReferencesChanged;
				error_refs_indexes.push_back(i);
			}
			else if (cell.col >= first + count) {
				Position old = cell;
				cell.col -= count;
				size_t pos = 0;
				while (pos != std::string::npos) {
					pos = expr.find(old.ToString());
					if (pos == std::string::npos)
						break;
					expr.replace(
						expr.begin() + pos,
						expr.begin() + pos + old.ToString().size(),
						cell.ToString());
				}
				if (res != HandlingResult::ReferencesChanged)
					res = HandlingResult::ReferencesRenamedOnly;
			}
		}
		for (int index : error_refs_indexes) {
			ref_cells.erase(ref_cells.begin() + index);
		}
		if (res == HandlingResult::ReferencesChanged) {
			value = FormulaError::Category::Ref;
		}
		if (res == HandlingResult::ReferencesRenamedOnly) {
			UpdateData();
		}
		return res;
	}
};

std::unique_ptr<IFormula> ParseFormula(std::string expression) {
	return make_unique<Formula>(expression);
}

