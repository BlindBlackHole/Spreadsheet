#pragma once

#include "FormulaBaseListener.h"
#include "../build/Ats.h"
#include <stack>
#include <iostream>
#include <cmath>

//class AstContext {
//public:
//	virtual double Evaluate(const ISheet&) = 0;
//	virtual std::string ToString(char, bool = false, bool = false) = 0;
//};
//
//class AstNumber : public AstContext {
//private:
//	double value;
//public:
//	AstNumber(double value) : value(value) {}
//	
//	double Evaluate(const ISheet&) override {
//		return value;
//	}
//
//	std::string ToString(char, bool, bool) override {
//		std::ostringstream out;
//		out << value;
//		return out.str();
//	}
//};
//
//class AstCell : public AstContext {
//private:
//	std::string pos;
//public:
//	AstCell(std::string pos) 
//		: pos(pos) {}
//
//	double Evaluate(const ISheet& sheet) override {
//		auto cell = sheet.GetCell(Position::FromString(pos));
//		if (!cell || cell->GetText() == "") {
//			return 0.0;
//		}
//		if (cell->GetText().find("#REF!") != std::string::npos) {
//			throw FormulaError::Category::Ref;
//		}
//		if (cell->GetText().find("#VALUE!") != std::string::npos) {
//			throw FormulaError::Category::Value;
//		}
//		if (cell->GetText().find("#DIV/0!") != std::string::npos) {
//			throw FormulaError::Category::Div0;
//		}
//		auto v = cell->GetValue();
//		if (auto value = std::get_if<std::string>(&v)) {
//			size_t pos;
//			try {
//				auto d =  std::stod(*value, &pos);
//				if (pos != value->size()) {
//					throw FormulaError::Category::Value;
//				}
//				return d;
//			}
//			catch (...) {
//				throw FormulaError::Category::Value;
//			}
//		}
//		else if (auto value = std::get_if<double>(&v)) {
//			return *value;
//		}
//		else if (auto value = std::get_if<FormulaError>(&v)) {
//			throw FormulaError::Category::Div0;
//		}
//		else {
//			std::cerr << "uncorrect value\n";
//			throw std::error_condition();
//		}
//	}
//
//	std::string ToString(char, bool, bool) override {
//		return pos;
//	}
//};
//
//class AstBinaryOperation : public AstContext {
//private:
//	std::shared_ptr<AstContext> lhs;
//	std::shared_ptr<AstContext> rhs;
//	char op;
//public:
//	explicit AstBinaryOperation(char operation) : op(operation) {}
//
//	void SetParams(std::shared_ptr<AstContext> lhs, std::shared_ptr<AstContext> rhs) {
//		this->lhs = lhs;
//		this->rhs = rhs;
//	}
//
//	double Evaluate(const ISheet& sheet) override {
//		if (op == '+') {
//			auto answer = lhs->Evaluate(sheet) + rhs->Evaluate(sheet);
//			if (!std::isfinite(answer)) {
//				throw FormulaError::Category::Div0;
//			}
//			return answer;
//		}
//		else if (op == '-') {
//			auto answer = lhs->Evaluate(sheet) - rhs->Evaluate(sheet);
//			if (!std::isfinite(answer)) {
//				throw FormulaError::Category::Div0;
//			}
//			return answer;
//		}
//		else if (op == '*') {
//			auto answer = lhs->Evaluate(sheet) * rhs->Evaluate(sheet);
//			if (!std::isfinite(answer)) {
//				throw FormulaError::Category::Div0;
//			}
//			return answer;
//		}
//		else if (op == '/') {
//			auto answer = lhs->Evaluate(sheet) / rhs->Evaluate(sheet);
//			if (!std::isfinite(answer)) {
//				throw FormulaError::Category::Div0;
//			}
//			return answer;
//		}
//		return 0.0;
//	}
//	std::string ToString(char other_op, bool isRight = false, bool isUnary = false) override {
//		//a-(b+-c)
//		if (isRight && other_op == '-' && (op == '-' || op == '+')) {
//			return '(' + lhs->ToString(op) + op + rhs->ToString(op, true) + ')';
//		}
//		//a/(b*/c)
//		if (isRight && other_op == '/' && (op == '*' || op == '/')) {
//			return '(' + lhs->ToString(op) + op + rhs->ToString(op, true) + ')';
//		}
//		//(a+-b)/*c
//		if ((other_op == '*' || other_op == '/') && (op == '+' || op == '-')) {
//			return '(' + lhs->ToString(op) + op + rhs->ToString(op, true) + ')';
//		}
//		//+-(a+-b)
//		if (isRight && isUnary && (other_op == '-' || other_op == '+') && (op == '+' || op == '-')) {
//			return '(' + lhs->ToString(op) + op + rhs->ToString(op, true) + ')';
//		}
//		return lhs->ToString(op) + op + rhs->ToString(op, true);
//	}
//};
//
//class AstUnaryOperator : public AstContext {
//private:
//	std::shared_ptr<AstContext> ctx;
//	char op;
//public:
//	AstUnaryOperator(std::shared_ptr<AstContext> ctx, char op) 
//		: ctx(ctx), op(op) {}
//
//	double Evaluate(const ISheet& sheet) override {
//		return ctx->Evaluate(sheet) * (op == '-' ? -1.0 : 1.0);
//	}
//
//	std::string ToString(char, bool, bool) override {
//		return op + ctx->ToString(op, true, true);
//	}
//};
//
//class Ast {
//private:
//public:
//	std::stack<std::shared_ptr<AstContext>> vertexes;
//	std::stack<std::shared_ptr<AstContext>> formula;
//	std::set<Position> ref_cells;
//	Ast() = default;
//	void PutToStack(std::shared_ptr<AstContext> context, bool isBinaryOp = false) {
//		if (vertexes.size() > 1 && isBinaryOp) {
//			auto operation = dynamic_cast<AstBinaryOperation*>(context.get());
//			std::shared_ptr<AstContext> rhs = vertexes.top();
//			vertexes.pop();
//			std::shared_ptr<AstContext> lhs = vertexes.top();
//			vertexes.pop();
//			operation->SetParams(lhs, rhs);
//			vertexes.push(std::make_shared<AstBinaryOperation>(*operation));
//			return;
//		}
//		vertexes.push(context);
//	}
//
//	std::string GetExpression() const{
//		return vertexes.top()->ToString('.');
//	}
//};


class MyFormulaListener : public FormulaBaseListener {
private:
	Ast ast;
public:
	void exitUnaryOp(FormulaParser::UnaryOpContext* ctx) override {
		auto arg = ast.vertexes.top();
		ast.vertexes.pop();
		ast.vertexes.push(std::make_shared<AstUnaryOperator>(arg, ctx->getText()[0]));
	}

	void exitLiteral(FormulaParser::LiteralContext* ctx) override {
		ast.formula.push(std::make_shared<AstNumber>(std::atof(ctx->getText().c_str())));
		ast.PutToStack(std::make_shared<AstNumber>(std::atof(ctx->getText().c_str())));
	}

	void exitCell(FormulaParser::CellContext* ctx) override {
		auto cell = ctx->getText();
		auto pos = Position::FromString(cell);
		if (!pos.IsValid()) {
			throw InvalidPositionException("position: " + pos.ToString() + " is invalid");
		}
		ast.formula.push(std::make_shared<AstCell>(ctx->getText()));
		//Save cells that present in formula;
		ast.ref_cells.insert(Position::FromString(ctx->getText()));
		ast.vertexes.push(std::make_shared<AstCell>(ctx->getText()));
	}

	void exitBinaryOp(FormulaParser::BinaryOpContext* ctx) override {
		if (ast.formula.empty()) {
			throw std::runtime_error("Uncorrect formula");
		}
		auto elem = ast.formula.top();
		ast.formula.pop();
		ast.formula.push(std::make_shared<AstBinaryOperation>(ctx->children[1]->getText()[0]));
		ast.formula.push(elem);
		ast.PutToStack(
			std::make_shared<AstBinaryOperation>(ctx->children[1]->getText()[0]), true);
	}

	Ast GetResult() {
		return ast;
	}
};
