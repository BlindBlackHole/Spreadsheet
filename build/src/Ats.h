#pragma once
#include "common.h"
#include <cmath>
#include <iostream>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>


class AstContext
{
public:
    mutable int subtree_size = 1;  // default = 1
    virtual double Evaluate(const ISheet&, bool wantParallel = false) = 0;
    virtual std::string ToString(char, bool = false, bool = false) = 0;
};

class AstNumber : public AstContext
{
private:
    double value;

public:
    AstNumber(double value);

    double Evaluate(const ISheet&, bool wantParallel = false) override;

    std::string ToString(char, bool, bool) override;
};

class AstCell : public AstContext
{
private:
    std::string pos;

public:
    AstCell(std::string pos);

    double Evaluate(const ISheet& sheet, bool wantParallel = false) override;

    std::string ToString(char, bool, bool) override;
};

class AstBinaryOperation : public AstContext
{
public:
    std::shared_ptr<AstContext> lhs;
    std::shared_ptr<AstContext> rhs;
    char op;

public:
    explicit AstBinaryOperation(char operation);

    void SetParams(std::shared_ptr<AstContext> lhs, std::shared_ptr<AstContext> rhs);

    double Evaluate(const ISheet& sheet, bool wantParallel = false) override;

    double Evaluate(const ISheet& sheet, std::unordered_map<std::uintptr_t, double>& results, bool wantParallel = false);

    std::string ToString(char other_op, bool isRight = false, bool isUnary = false) override;

    std::string ToString(std::string lhsValue);

    std::string ToString(std::unordered_map<std::uintptr_t, std::string>& results);
};

class AstUnaryOperator : public AstContext
{
public:
    std::shared_ptr<AstContext> ctx;
    char op;

public:
    AstUnaryOperator(std::shared_ptr<AstContext> ctx, char op);

    double Evaluate(const ISheet& sheet, bool wantParallel = false) override;

    std::string ToString(char, bool, bool) override;
};

class Ast
{
public:
    std::stack<std::shared_ptr<AstContext>> vertexes;
    std::stack<std::shared_ptr<AstContext>> formula;
    std::set<Position> ref_cells;

    std::vector<std::weak_ptr<AstBinaryOperation>> operations;

public:
    Ast() = default;

    ~Ast() {
        Clear();
    }

    void PutToStack(std::shared_ptr<AstContext> context, bool isBinaryOp = false);

    std::string GetExpression() const;

    double Evaluate(const ISheet& sheet, bool wantParallel = false);

    double EvaluateParallel(const ISheet& sheet, bool wantParallel = true);

    void Ast::Clear() {
        std::unordered_set<std::shared_ptr<AstContext>> visited;
        std::stack<std::shared_ptr<AstContext>> stack;

        while (!vertexes.empty()) {
            stack.push(vertexes.top());
            vertexes.pop();
        }

        while (!stack.empty()) {
            auto node = stack.top();
            stack.pop();

            if (!node || visited.count(node)) continue;
            visited.insert(node);

            if (auto bin = std::dynamic_pointer_cast<AstBinaryOperation>(node)) {
                if (bin->lhs) stack.push(bin->lhs);
                if (bin->rhs) stack.push(bin->rhs);
                bin->lhs.reset();
                bin->rhs.reset();
            }
            else if (auto un = std::dynamic_pointer_cast<AstUnaryOperator>(node)) {
                if (un->ctx) stack.push(un->ctx);
                un->ctx.reset();
            }
        }

        formula = {};
        ref_cells = {};
        operations = {};
    }
};
