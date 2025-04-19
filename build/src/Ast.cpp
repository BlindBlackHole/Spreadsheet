#include "Ats.h"
#include <unordered_map>
#include <sstream>
#include <future>

using namespace std;

// AstNumber
AstNumber::AstNumber(double value) : value(value) {}

double AstNumber::Evaluate(const ISheet&, bool wantParallel)
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

double AstCell::Evaluate(const ISheet& sheet, bool wantParallel)
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

double AstBinaryOperation::Evaluate(const ISheet& sheet, bool wantParallel)
{
    auto lhsValue = lhs->Evaluate(sheet, wantParallel);
    auto rhsValue = rhs->Evaluate(sheet, wantParallel);
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

        auto value = operand->Evaluate(sheet, false);
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

double AstUnaryOperator::Evaluate(const ISheet& sheet, bool wantParallel)
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

        operation->subtree_size = 1 + lhs->subtree_size + rhs->subtree_size;

        auto op = std::make_shared<AstBinaryOperation>(*operation);
        vertexes.push(op);
        operations.push_back(std::move(op));
        return;
    }
    vertexes.push(context);
}

constexpr size_t MIN_PARALLEL_OPERATIONS = 10;

std::string Ast::GetExpression() const
{
    if (operations.size() < MIN_PARALLEL_OPERATIONS) {
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

double Ast::Evaluate(const ISheet& sheet, bool wantParallel)
{
    if (operations.size() < MIN_PARALLEL_OPERATIONS) {
        return vertexes.top()->Evaluate(sheet);
    }

    if (wantParallel) {
        return EvaluateParallel(sheet, wantParallel);
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

double Ast::EvaluateParallel(const ISheet& sheet, bool wantParallel) {
    constexpr int parallel_threshold = 1000;

    std::unordered_map<std::uintptr_t, double> cached_results;
    std::unordered_map<std::uintptr_t, std::future<double>> futures;

    std::stack<std::shared_ptr<AstContext>> stack;

    // 1. Скидаємо всі вершини в стек
    std::stack<std::shared_ptr<AstContext>> temp_stack = vertexes;
    while (!temp_stack.empty()) {
        stack.push(temp_stack.top());
        temp_stack.pop();
    }

    // 2. Обхід графа обчислення
    while (!stack.empty()) {
        auto node = stack.top();
        stack.pop();

        auto id = reinterpret_cast<std::uintptr_t>(node.get());

        if (cached_results.count(id)) {
            continue;  // вже обчислено
        }

        // Якщо це бінарна операція — обчислюємо обидві сторони
        if (auto bin = std::dynamic_pointer_cast<AstBinaryOperation>(node)) {
            auto lhs_id = reinterpret_cast<std::uintptr_t>(bin->lhs.get());
            auto rhs_id = reinterpret_cast<std::uintptr_t>(bin->rhs.get());

            // Обчислити або запустити обчислення для lhs
            if (!cached_results.count(lhs_id)) {
                if (bin->lhs->subtree_size > parallel_threshold && !futures.count(lhs_id)) {
                    futures[lhs_id] = std::async(std::launch::async, [&sheet, lhs = bin->lhs]() {
                        std::cout << "lhs parallel" << std::endl;
                        return lhs->Evaluate(sheet, false);  // або EvaluateParallel, якщо рекурсія дозволена
                    });
                }
                else {
                    stack.push(bin->lhs);  // обчислимо вручну
                }
            }

            // Те ж саме для rhs
            if (!cached_results.count(rhs_id)) {
                if (bin->rhs->subtree_size > parallel_threshold && !futures.count(rhs_id)) {
                    futures[rhs_id] = std::async(std::launch::async, [&sheet, rhs = bin->rhs]() {
                        std::cout << "rhs parallel" << std::endl;
                        return rhs->Evaluate(sheet, false);
                    });
                }
                else {
                    stack.push(bin->rhs);
                }
            }

            // Перевіримо, чи вже все готове
            if ((cached_results.count(lhs_id) || futures.count(lhs_id)) &&
                (cached_results.count(rhs_id) || futures.count(rhs_id))) {

                // Забираємо з future, якщо треба
                if (futures.count(lhs_id)) {
                    cached_results[lhs_id] = futures[lhs_id].get();
                    futures.erase(lhs_id);
                }
                if (futures.count(rhs_id)) {
                    cached_results[rhs_id] = futures[rhs_id].get();
                    futures.erase(rhs_id);
                }

                // Обчислюємо поточну операцію
                double result = applyOperator(bin->op, cached_results[lhs_id], cached_results[rhs_id]);
                cached_results[id] = result;
            }
            else {
                // Поставимо назад, бо ще не готово
                stack.push(node);
            }

        }
        else {
            // Це не бінарна операція — просто обчислити
            double val = node->Evaluate(sheet);
            cached_results[id] = val;
        }
    }

    // Повертаємо результат з останнього вузла
    if (vertexes.empty())
        return 0.0;

    auto top_id = reinterpret_cast<std::uintptr_t>(vertexes.top().get());
    return cached_results.at(top_id);
}

// Ast