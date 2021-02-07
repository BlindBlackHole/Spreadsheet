#include "../build/Ats.h"
#include <sstream>

using namespace std;

//AstNumber
AstNumber::AstNumber(double value) : value(value) {}

double AstNumber::Evaluate(const ISheet&) {
	return value;
}

std::string AstNumber::ToString(char, bool, bool) {
	std::ostringstream out;
	out << value;
	return out.str();
}
//AstNumber


//AstCell
AstCell::AstCell(std::string pos)
	: pos(pos) {}

double AstCell::Evaluate(const ISheet& sheet) {
	auto cell = sheet.GetCell(Position::FromString(pos));
	if (!cell || cell->GetText() == "") {
		return 0.0;
	}
	if (cell->GetText().find("#REF!") != std::string::npos) {
		throw FormulaError::Category::Ref;
	}
	if (cell->GetText().find("#VALUE!") != std::string::npos) {
		throw FormulaError::Category::Value;
	}
	if (cell->GetText().find("#DIV/0!") != std::string::npos) {
		throw FormulaError::Category::Div0;
	}
	auto v = cell->GetValue();
	if (auto value = std::get_if<std::string>(&v)) {
		size_t pos;
		try {
			auto d = std::stod(*value, &pos);
			if (pos != value->size()) {
				throw FormulaError::Category::Value;
			}
			return d;
		}
		catch (...) {
			throw FormulaError::Category::Value;
		}
	}
	else if (auto value = std::get_if<double>(&v)) {
		return *value;
	}
	else if (auto value = std::get_if<FormulaError>(&v)) {
		throw FormulaError::Category::Div0;
	}
	else {
		std::cerr << "uncorrect value\n";
		throw std::error_condition();
	}
}

std::string AstCell::ToString(char, bool, bool) {
	return pos;
}
//AstCell

//AstBinaryOperation
AstBinaryOperation::AstBinaryOperation(char operation) : op(operation) {}

void AstBinaryOperation::SetParams(std::shared_ptr<AstContext> lhs, std::shared_ptr<AstContext> rhs) {
	this->lhs = lhs;
	this->rhs = rhs;
}

double AstBinaryOperation::Evaluate(const ISheet& sheet) {
	if (op == '+') {
		auto answer = lhs->Evaluate(sheet) + rhs->Evaluate(sheet);
		if (!std::isfinite(answer)) {
			throw FormulaError::Category::Div0;
		}
		return answer;
	}
	else if (op == '-') {
		auto answer = lhs->Evaluate(sheet) - rhs->Evaluate(sheet);
		if (!std::isfinite(answer)) {
			throw FormulaError::Category::Div0;
		}
		return answer;
	}
	else if (op == '*') {
		auto answer = lhs->Evaluate(sheet) * rhs->Evaluate(sheet);
		if (!std::isfinite(answer)) {
			throw FormulaError::Category::Div0;
		}
		return answer;
	}
	else if (op == '/') {
		auto answer = lhs->Evaluate(sheet) / rhs->Evaluate(sheet);
		if (!std::isfinite(answer)) {
			throw FormulaError::Category::Div0;
		}
		return answer;
	}
	return 0.0;
}

std::string AstBinaryOperation::ToString(char other_op, bool isRight, bool isUnary) {
	//a-(b+-c)
	if (isRight && other_op == '-' && (op == '-' || op == '+')) {
		return '(' + lhs->ToString(op) + op + rhs->ToString(op, true) + ')';
	}
	//a/(b*/c)
	if (isRight && other_op == '/' && (op == '*' || op == '/')) {
		return '(' + lhs->ToString(op) + op + rhs->ToString(op, true) + ')';
	}
	//(a+-b)/*c
	if ((other_op == '*' || other_op == '/') && (op == '+' || op == '-')) {
		return '(' + lhs->ToString(op) + op + rhs->ToString(op, true) + ')';
	}
	//+-(a+-b)
	if (isRight && isUnary && (other_op == '-' || other_op == '+') && (op == '+' || op == '-')) {
		return '(' + lhs->ToString(op) + op + rhs->ToString(op, true) + ')';
	}
	return lhs->ToString(op) + op + rhs->ToString(op, true);
}
//AstBinaryOperation

//AstUnaryOperator
AstUnaryOperator::AstUnaryOperator(std::shared_ptr<AstContext> ctx, char op)
	: ctx(ctx), op(op) {}

double AstUnaryOperator::Evaluate(const ISheet& sheet) {
	return ctx->Evaluate(sheet) * (op == '-' ? -1.0 : 1.0);
}

std::string AstUnaryOperator::ToString(char, bool, bool) {
	return op + ctx->ToString(op, true, true);
}
//AstUnaryOperator

//Ast
void Ast::PutToStack(std::shared_ptr<AstContext> context, bool isBinaryOp) {
	if (vertexes.size() > 1 && isBinaryOp) {
		auto operation = dynamic_cast<AstBinaryOperation*>(context.get());
		std::shared_ptr<AstContext> rhs = vertexes.top();
		vertexes.pop();
		std::shared_ptr<AstContext> lhs = vertexes.top();
		vertexes.pop();
		operation->SetParams(lhs, rhs);
		vertexes.push(std::make_shared<AstBinaryOperation>(*operation));
		return;
	}
	vertexes.push(context);
}

std::string Ast::GetExpression() const {
	return vertexes.top()->ToString('.');
}
//Ast