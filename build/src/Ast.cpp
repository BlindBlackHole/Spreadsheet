#include "Ats.h"
#include <unordered_map>
#include <sstream>

using namespace std;

// AstNumber
AstNumber::AstNumber(double value) : value(value) {}

double AstNumber::Evaluate(const ISheet&)
{
    return value;
}

std::string AstNumber::ToString(char, bool, bool)
{
    std::ostringstream out;
    out << value;
    return out.str();
}

// AstNumber


// AstCell
AstCell::AstCell(std::string pos) : pos(pos) {}

double AstCell::Evaluate(const ISheet& sheet)
{
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
        } catch (...) {
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

std::string AstCell::ToString(char, bool, bool)
{
    return pos;
}

// AstCell

// AstBinaryOperation
AstBinaryOperation::AstBinaryOperation(char operation) : op(operation) {}

void AstBinaryOperation::SetParams(std::shared_ptr<AstContext> lhs, std::shared_ptr<AstContext> rhs)
{
    this->lhs = lhs;
    this->rhs = rhs;
}

namespace {

    double applyOperator(char op, double lhs, double rhs)
    {
        if (op == '+') {
            auto answer = lhs + rhs;
            if (!std::isfinite(answer)) {
                throw FormulaError::Category::Div0;
            }
            return answer;
        }
        else if (op == '-') {
            auto answer = lhs - rhs;
            if (!std::isfinite(answer)) {
                throw FormulaError::Category::Div0;
            }
            return answer;
        }
        else if (op == '*') {
            auto answer = lhs * rhs;
            if (!std::isfinite(answer)) {
                throw FormulaError::Category::Div0;
            }
            return answer;
        }
        else if (op == '/') {
            auto answer = lhs / rhs;
            if (!std::isfinite(answer)) {
                throw FormulaError::Category::Div0;
            }
            return answer;
        }
        return 0.0;
    }

}

double AstBinaryOperation::Evaluate(const ISheet& sheet)
{
    auto lhsValue = lhs->Evaluate(sheet);
    auto rhsValue = rhs->Evaluate(sheet);
    return applyOperator(op, lhsValue, rhsValue);
}

double AstBinaryOperation::Evaluate(const ISheet& sheet, std::unordered_map<std::uintptr_t, double>& results)
{
    auto getValue = [&](const auto& operand) {
        const auto ptr = reinterpret_cast<std::uintptr_t>(operand.get());
        const auto it = results.find(ptr);
        if (it != results.end()) {
            return it->second;
        }

        auto value = operand->Evaluate(sheet);
        results[ptr] = value;
        return value;
    };

    auto lhsValue = getValue(lhs);
    auto rhsValue = getValue(rhs);
    return applyOperator(op, lhsValue, rhsValue);
}

namespace {

    std::string formatResult(char op, std::string lhs, std::string rhs, char other_op, bool isRight = false, bool isUnary = false)
    {
        // a-(b+-c)
        if (isRight && other_op == '-' && (op == '-' || op == '+')) {
            return '(' + lhs + op + rhs + ')';
        }
        // a/(b*/c)
        if (isRight && other_op == '/' && (op == '*' || op == '/')) {
            return '(' + lhs + op + rhs + ')';
        }
        //(a+-b)/*c
        if ((other_op == '*' || other_op == '/') && (op == '+' || op == '-')) {
            return '(' + lhs + op + rhs + ')';
        }
        //+-(a+-b)
        if (isRight && isUnary && (other_op == '-' || other_op == '+') && (op == '+' || op == '-')) {
            return '(' + lhs + op + rhs + ')';
        }
        return lhs + op + rhs;
    }

}

std::string AstBinaryOperation::ToString(char other_op, bool isRight, bool isUnary)
{
    auto lhsValue = lhs->ToString(op);
    auto rhsValue = rhs->ToString(op, true);
    return formatResult(op, std::move(lhsValue), std::move(rhsValue), other_op, isRight, isUnary);
}

std::string AstBinaryOperation::ToString(std::string lhsValue)
{
    return lhsValue + op + rhs->ToString(op, true);
}

std::string AstBinaryOperation::ToString(std::unordered_map<std::uintptr_t, std::string>& results)
{
    auto getValue = [&] (const auto& operand, bool isRight) {
        const auto ptr = reinterpret_cast<std::uintptr_t>(operand.get());
        const auto it = results.find(ptr);
        if (it != results.end()) {
            return it->second;
        }

        auto value = operand->ToString(op, isRight);
        results[ptr] = value;
        return value;
    };
    
    auto lhsValue = getValue(lhs, false);
    auto rhsValue = getValue(rhs, true);

    return formatResult(op, std::move(lhsValue), std::move(rhsValue), '.');
}

// AstBinaryOperation

// AstUnaryOperator
AstUnaryOperator::AstUnaryOperator(std::shared_ptr<AstContext> ctx, char op) : ctx(ctx), op(op) {}

double AstUnaryOperator::Evaluate(const ISheet& sheet)
{
    return ctx->Evaluate(sheet) * (op == '-' ? -1.0 : 1.0);
}

std::string AstUnaryOperator::ToString(char, bool, bool)
{
    return op + ctx->ToString(op, true, true);
}

// AstUnaryOperator

// Ast
void Ast::PutToStack(std::shared_ptr<AstContext> context, bool isBinaryOp)
{
    if (vertexes.size() > 1 && isBinaryOp) {
        auto operation = dynamic_cast<AstBinaryOperation*>(context.get());
        std::shared_ptr<AstContext> rhs = vertexes.top();
        vertexes.pop();
        std::shared_ptr<AstContext> lhs = vertexes.top();
        vertexes.pop();
        operation->SetParams(lhs, rhs);
        auto op = std::make_shared<AstBinaryOperation>(*operation);
        vertexes.push(op);
        operations.push_back(std::move(op));
        return;
    }
    vertexes.push(context);
}

std::string Ast::GetExpression() const
{
    if (operations.size() < 2) {
        return vertexes.top()->ToString('.');
    }

    std::unordered_map<std::uintptr_t, std::string> cachedResults;

    std::string result;
    for (size_t i = 0; i < operations.size(); ++i) {
        std::shared_ptr ptr = operations[i].lock();
        result = ptr->ToString(cachedResults);
        cachedResults[reinterpret_cast<std::uintptr_t>(ptr.get())] = result;
    }

    return result;
}

double Ast::Evaluate(const ISheet& sheet)
{
    if (operations.size() < 2) {
        return vertexes.top()->Evaluate(sheet);
    }

    std::unordered_map<std::uintptr_t, double> cachedResults;

    double result{};
    for (size_t i = 0; i < operations.size(); ++i) {
        std::shared_ptr ptr = operations[i].lock();
        result = ptr->Evaluate(sheet, cachedResults);
        cachedResults[reinterpret_cast<std::uintptr_t>(ptr.get())] = result;
    }

    return result;
}

// Ast