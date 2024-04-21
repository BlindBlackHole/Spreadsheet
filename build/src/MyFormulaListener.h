#pragma once

#include "Ats.h"
#include "FormulaBaseListener.h"
#include <cmath>
#include <iostream>
#include <stack>

class MyFormulaListener : public FormulaBaseListener
{
private:
    Ast ast;

public:
    void exitUnaryOp(FormulaParser::UnaryOpContext* ctx) override
    {
        auto arg = ast.vertexes.top();
        ast.vertexes.pop();
        ast.vertexes.push(std::make_shared<AstUnaryOperator>(arg, ctx->getText()[0]));
    }

    void exitLiteral(FormulaParser::LiteralContext* ctx) override
    {
        ast.formula.push(std::make_shared<AstNumber>(std::atof(ctx->getText().c_str())));
        ast.PutToStack(std::make_shared<AstNumber>(std::atof(ctx->getText().c_str())));
    }

    void exitCell(FormulaParser::CellContext* ctx) override
    {
        auto cell = ctx->getText();
        auto pos = Position::FromString(cell);
        if (!pos.IsValid()) {
            throw InvalidPositionException("position: " + pos.ToString() + " is invalid");
        }
        ast.formula.push(std::make_shared<AstCell>(ctx->getText()));
        // Save cells that present in formula;
        ast.ref_cells.insert(Position::FromString(ctx->getText()));
        ast.vertexes.push(std::make_shared<AstCell>(ctx->getText()));
    }

    void exitBinaryOp(FormulaParser::BinaryOpContext* ctx) override
    {
        if (ast.formula.empty()) {
            throw std::runtime_error("Uncorrect formula");
        }
        auto elem = ast.formula.top();
        ast.formula.pop();
        ast.formula.push(std::make_shared<AstBinaryOperation>(ctx->children[1]->getText()[0]));
        ast.formula.push(elem);
        ast.PutToStack(std::make_shared<AstBinaryOperation>(ctx->children[1]->getText()[0]), true);
    }

    Ast GetResult() { return ast; }
};
