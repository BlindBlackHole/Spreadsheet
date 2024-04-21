#pragma once
#include "common.h"
#include <cmath>
#include <iostream>
#include <set>
#include <stack>


class AstContext
{
public:
    virtual double Evaluate(const ISheet&) = 0;
    virtual std::string ToString(char, bool = false, bool = false) = 0;
};

class AstNumber : public AstContext
{
private:
    double value;

public:
    AstNumber(double value);

    double Evaluate(const ISheet&) override;

    std::string ToString(char, bool, bool) override;
};

class AstCell : public AstContext
{
private:
    std::string pos;

public:
    AstCell(std::string pos);

    double Evaluate(const ISheet& sheet) override;

    std::string ToString(char, bool, bool) override;
};

class AstBinaryOperation : public AstContext
{
private:
    std::shared_ptr<AstContext> lhs;
    std::shared_ptr<AstContext> rhs;
    char op;

public:
    explicit AstBinaryOperation(char operation);

    void SetParams(std::shared_ptr<AstContext> lhs, std::shared_ptr<AstContext> rhs);

    double Evaluate(const ISheet& sheet) override;

    std::string ToString(char other_op, bool isRight = false, bool isUnary = false) override;
};

class AstUnaryOperator : public AstContext
{
private:
    std::shared_ptr<AstContext> ctx;
    char op;

public:
    AstUnaryOperator(std::shared_ptr<AstContext> ctx, char op);

    double Evaluate(const ISheet& sheet) override;

    std::string ToString(char, bool, bool) override;
};

class Ast
{
public:
    std::stack<std::shared_ptr<AstContext>> vertexes;
    std::stack<std::shared_ptr<AstContext>> formula;
    std::set<Position> ref_cells;

public:
    Ast() = default;

    void PutToStack(std::shared_ptr<AstContext> context, bool isBinaryOp = false);

    std::string GetExpression() const;
};
