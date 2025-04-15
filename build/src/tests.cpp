#pragma once

#include "tests.h"

#include "FormulaBaseListener.h"
#include "FormulaLexer.h"
#include "FormulaListener.h"
#include "FormulaParser.h"
#include "MyFormulaListener.h"
#include "common.h"
#include "formula.h"
#include "test_runner.h"

#include <fstream>
#include <execution>
#include <algorithm>


namespace {

    std::string ToString(FormulaError::Category category)
    {
        return std::string(FormulaError(category).ToString());
    }

    std::ostream& operator<<(std::ostream& output, Position pos)
    {
        return output << "(" << pos.row << ", " << pos.col << ")";
    }

    Position operator"" _pos(const char* str, std::size_t)
    {
        return Position::FromString(str);
    }

    std::ostream& operator<<(std::ostream& output, Size size)
    {
        return output << "(" << size.rows << ", " << size.cols << ")";
    }

    std::ostream& operator<<(std::ostream& output, const ICell::Value& value)
    {
        std::visit([&](const auto& x) { output << x; }, value);
        return output;
    }

    std::string_view ToString(IFormula::HandlingResult hr)
    {
        switch (hr) {
        case IFormula::HandlingResult::NothingChanged: return "NothingChanged";
        case IFormula::HandlingResult::ReferencesRenamedOnly: return "ReferencesRenamedOnly";
        case IFormula::HandlingResult::ReferencesChanged: return "ReferencesChanged";
        }
        return "";
    }

    std::ostream& operator<<(std::ostream& output, IFormula::HandlingResult hr)
    {
        return output << ToString(hr);
    }

}  // namespace

void TestPositionAndStringConversion()
{
    auto testSingle = [](Position pos, std::string_view str) {
        ASSERT_EQUAL(pos.ToString(), str);
        ASSERT_EQUAL(Position::FromString(str), pos);
    };

    for (int i = 0; i < 25; ++i) {
        testSingle(Position{i, i}, char('A' + i) + std::to_string(i + 1));
    }

    testSingle(Position{0, 0}, "A1");
    testSingle(Position{0, 1}, "B1");
    testSingle(Position{0, 25}, "Z1");
    testSingle(Position{0, 26}, "AA1");
    testSingle(Position{0, 27}, "AB1");
    testSingle(Position{0, 51}, "AZ1");
    testSingle(Position{0, 52}, "BA1");
    testSingle(Position{0, 53}, "BB1");
    testSingle(Position{0, 77}, "BZ1");
    testSingle(Position{0, 78}, "CA1");
    testSingle(Position{0, 701}, "ZZ1");
    testSingle(Position{0, 702}, "AAA1");
    testSingle(Position{136, 2}, "C137");
    testSingle(Position{Position::kMaxRows - 1, Position::kMaxCols - 1}, "XFD16384");
}

void TestPositionToStringInvalid()
{
    ASSERT_EQUAL((Position{-1, -1}).ToString(), "");
    ASSERT_EQUAL((Position{-10, 0}).ToString(), "");
    ASSERT_EQUAL((Position{1, -3}).ToString(), "");
}

void TestStringToPositionInvalid()
{
    ASSERT(!Position::FromString("").IsValid());
    ASSERT(!Position::FromString("A").IsValid());
    ASSERT(!Position::FromString("1").IsValid());
    ASSERT(!Position::FromString("e2").IsValid());
    ASSERT(!Position::FromString("A0").IsValid());
    ASSERT(!Position::FromString("A-1").IsValid());
    ASSERT(!Position::FromString("A+1").IsValid());
    ASSERT(!Position::FromString("R2D2").IsValid());
    ASSERT(!Position::FromString("C3PO").IsValid());
    ASSERT(!Position::FromString("XFD16385").IsValid());
    ASSERT(!Position::FromString("XFE16384").IsValid());
    ASSERT(!Position::FromString("A1234567890123456789").IsValid());
    ASSERT(!Position::FromString("ABCDEFGHIJKLMNOPQRS8").IsValid());
}

void TestEmpty()
{
    auto sheet = CreateSheet();
    ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{0, 0}));
}

void TestInvalidPosition()
{
    auto sheet = CreateSheet();
    try {
        sheet->SetCell(Position{-1, 0}, "");
    } catch (const InvalidPositionException&) {
    }
    try {
        sheet->GetCell(Position{0, -2});
    } catch (const InvalidPositionException&) {
    }
    try {
        sheet->ClearCell(Position{Position::kMaxRows, 0});
    } catch (const InvalidPositionException&) {
    }
}

void TestSetCellPlainText()
{
    auto sheet = CreateSheet();

    auto checkCell = [&](Position pos, std::string text) {
        sheet->SetCell(pos, text);
        ICell* cell = sheet->GetCell(pos);
        ASSERT(cell != nullptr);
        ASSERT_EQUAL(cell->GetText(), text);
        ASSERT_EQUAL(std::get<std::string>(cell->GetValue()), text);
    };

    checkCell("A1"_pos, "Hello");
    checkCell("A1"_pos, "World");
    checkCell("B2"_pos, "Purr");
    checkCell("A3"_pos, "Meow");

    const ISheet& constSheet = *sheet;
    ASSERT_EQUAL(constSheet.GetCell("B2"_pos)->GetText(), "Purr");

    sheet->SetCell("A3"_pos, "'=escaped");
    ICell* cell = sheet->GetCell("A3"_pos);
    ASSERT_EQUAL(cell->GetText(), "'=escaped");
    ASSERT_EQUAL(std::get<std::string>(cell->GetValue()), "=escaped");
}

void TestClearCell()
{
    auto sheet = CreateSheet();

    sheet->SetCell("C2"_pos, "Me gusta");
    sheet->ClearCell("C2"_pos);
    ASSERT(sheet->GetCell("C2"_pos) == nullptr);

    sheet->ClearCell("A1"_pos);
    sheet->ClearCell("J10"_pos);
}

void TestFormulaArithmetic()
{
    auto sheet = CreateSheet();
    auto evaluate = [&](std::string expr) {
        return std::get<double>(ParseFormula(std::move(expr))->Evaluate(*sheet));
    };

    ASSERT_EQUAL(evaluate("1"), 1);
    ASSERT_EQUAL(evaluate("42"), 42);
    ASSERT_EQUAL(evaluate("2 + 2"), 4);
    ASSERT_EQUAL(evaluate("2 + 2*2"), 6);
    ASSERT_EQUAL(evaluate("4/2 + 6/3"), 4);
    ASSERT_EQUAL(evaluate("(2+3)*4 + (3-4)*5"), 15);
    ASSERT_EQUAL(evaluate("(12+13) * (14+(13-24/(1+1))*55-46)"), 575);
}

void TestFormulaReferences()
{
    auto sheet = CreateSheet();
    auto evaluate = [&](std::string expr) {
        return std::get<double>(ParseFormula(std::move(expr))->Evaluate(*sheet));
    };

    sheet->SetCell("A1"_pos, "1");
    ASSERT_EQUAL(evaluate("A1"), 1);
    sheet->SetCell("A2"_pos, "2");
    ASSERT_EQUAL(evaluate("A1+A2"), 3);

    sheet->SetCell("B3"_pos, "");
    ASSERT_EQUAL(evaluate("A1+B3"), 1);  
    ASSERT_EQUAL(evaluate("A1+B1"), 1);  
    ASSERT_EQUAL(evaluate("A1+E4"), 1);  
}

void TestFormulaExpressionFormatting()
{
    auto reformat = [](std::string expr) { return ParseFormula(std::move(expr))->GetExpression(); };

    ASSERT_EQUAL(reformat("  1  "), "1");
    ASSERT_EQUAL(reformat("  -1  "), "-1");
    ASSERT_EQUAL(reformat("2 + 2"), "2+2");
    ASSERT_EQUAL(reformat("(2*3)+4"), "2*3+4");
    ASSERT_EQUAL(reformat("(2*3)-4"), "2*3-4");
    ASSERT_EQUAL(reformat("( ( (  1) ) )"), "1");
    ASSERT_EQUAL(reformat("(2+3)+(1+2)"), "2+3+1+2");
    ASSERT_EQUAL(reformat("(2+3)*(1+2)"), "(2+3)*(1+2)");
    ASSERT_EQUAL(reformat("2+3*4"), "2+3*4");
    ASSERT_EQUAL(reformat("(2+3)*4"), "(2+3)*4");
    ASSERT_EQUAL(reformat("(123 + 456) / -B35 * 1"), "(123+456)/-B35*1");
    ASSERT_EQUAL(reformat("-(123 + 456) / -B35 * 1"), "-(123+456)/-B35*1");
    ASSERT_EQUAL(reformat("+(123 - 456) / -B35 * 1"), "+(123-456)/-B35*1");
    ASSERT_EQUAL(reformat("(1 / 2) / 3"), "1/2/3");
    ASSERT_EQUAL(reformat("1 / (2 / 3)"), "1/(2/3)");
    ASSERT_EQUAL(reformat("-    (    (2)     *   (3)    )"), "-2*3");
    ASSERT_EQUAL(reformat("+    (    2     /   3    )"), "+2/3");
    ASSERT_EQUAL(reformat("-    (    2     *   3    ) * 4"), "-2*3*4");
    ASSERT_EQUAL(reformat("-    (    2     +   (3)    ) * 4"), "-(2+3)*4");
    ASSERT_EQUAL(reformat("(-    (    2     +   (3)    ) * 4)"), "-(2+3)*4");
    ASSERT_EQUAL(reformat("1/(2+3)"), "1/(2+3)");
    ASSERT_EQUAL(reformat("1+(2+3)"), "1+2+3");
}

void TestFormulaReferencedCells()
{
    ASSERT(ParseFormula("1")->GetReferencedCells().empty());

    auto a1 = ParseFormula("A1");
    ASSERT_EQUAL(a1->GetReferencedCells(), (std::vector{"A1"_pos}));

    auto b2c3 = ParseFormula("B2+C3");
    ASSERT_EQUAL(b2c3->GetReferencedCells(), (std::vector{"B2"_pos, "C3"_pos}));

    auto tricky = ParseFormula("A1 + A2 + A1 + A3 + A1 + A2 + A1");
    ASSERT_EQUAL(tricky->GetExpression(), "A1+A2+A1+A3+A1+A2+A1");
    ASSERT_EQUAL(tricky->GetReferencedCells(), (std::vector{"A1"_pos, "A2"_pos, "A3"_pos}));
}

void TestFormulaHandleInsertion()
{
    auto f = ParseFormula("A1");
    ASSERT_EQUAL(f->GetReferencedCells(), std::vector{"A1"_pos});

    auto hr = f->HandleInsertedCols(0);
    ASSERT_EQUAL(f->GetExpression(), "B1");
    ASSERT_EQUAL(hr, IFormula::HandlingResult::ReferencesRenamedOnly);
    ASSERT_EQUAL(f->GetReferencedCells(), std::vector{"B1"_pos});

    hr = f->HandleInsertedRows(0);
    ASSERT_EQUAL(f->GetExpression(), "B2");
    ASSERT_EQUAL(hr, IFormula::HandlingResult::ReferencesRenamedOnly);
    ASSERT_EQUAL(f->GetReferencedCells(), std::vector{"B2"_pos});

    hr = f->HandleInsertedRows(2);
    ASSERT_EQUAL(f->GetExpression(), "B2");
    ASSERT_EQUAL(hr, IFormula::HandlingResult::NothingChanged);
    ASSERT_EQUAL(f->GetReferencedCells(), std::vector{"B2"_pos});

    f = ParseFormula("A1+B2");
    ASSERT_EQUAL(f->GetExpression(), "A1+B2");
    ASSERT_EQUAL(f->GetReferencedCells(), (std::vector{"A1"_pos, "B2"_pos}));

    hr = f->HandleInsertedCols(1);
    ASSERT_EQUAL(f->GetExpression(), "A1+C2");
    ASSERT_EQUAL(hr, IFormula::HandlingResult::ReferencesRenamedOnly);
    ASSERT_EQUAL(f->GetReferencedCells(), (std::vector{"A1"_pos, "C2"_pos}));

    hr = f->HandleInsertedRows(1);
    ASSERT_EQUAL(f->GetExpression(), "A1+C3");
    ASSERT_EQUAL(hr, IFormula::HandlingResult::ReferencesRenamedOnly);
    ASSERT_EQUAL(f->GetReferencedCells(), (std::vector{"A1"_pos, "C3"_pos}));

    hr = f->HandleInsertedCols(0, 3);
    ASSERT_EQUAL(f->GetExpression(), "D1+F3");
    ASSERT_EQUAL(hr, IFormula::HandlingResult::ReferencesRenamedOnly);
    ASSERT_EQUAL(f->GetReferencedCells(), (std::vector{"D1"_pos, "F3"_pos}));

    hr = f->HandleInsertedRows(0, 3);
    ASSERT_EQUAL(f->GetExpression(), "D4+F6");
    ASSERT_EQUAL(hr, IFormula::HandlingResult::ReferencesRenamedOnly);
    ASSERT_EQUAL(f->GetReferencedCells(), (std::vector{"D4"_pos, "F6"_pos}));
}

void TestInsertionOverflow()
{
    const auto maxp = Position{Position::kMaxRows - 1, Position::kMaxCols - 1};

    auto sheet = CreateSheet();
    std::string text = "There be dragons";
    sheet->SetCell(maxp, text);
    try {
        sheet->InsertCols(1);
        ASSERT(false);  // InsertCols must throw exception
    } catch (const TableTooBigException&) {
        ASSERT_EQUAL(sheet->GetCell(maxp)->GetText(), text);
    }
    try {
        sheet->InsertRows(1);
    } catch (const TableTooBigException&) {
        ASSERT_EQUAL(sheet->GetCell(maxp)->GetText(), text);
    }

    sheet = CreateSheet();
    text = "=" + maxp.ToString();
    sheet->SetCell("A1"_pos, text);
    try {
        sheet->InsertCols(1);
        ASSERT(false);  // InsertCols must throw exception
    } catch (const TableTooBigException&) {
        ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetText(), text);
    }
    try {
        sheet->InsertRows(1);
        ASSERT(false);  // InsertRows must throw exception
    } catch (const TableTooBigException&) {
        ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetText(), text);
    }
}

void TestFormulaHandleDeletion()
{
    auto f = ParseFormula("B2");
    ASSERT_EQUAL(f->GetReferencedCells(), std::vector{"B2"_pos});

    auto hr = f->HandleDeletedCols(0);
    ASSERT_EQUAL(f->GetExpression(), "A2");
    ASSERT_EQUAL(hr, IFormula::HandlingResult::ReferencesRenamedOnly);
    ASSERT_EQUAL(f->GetReferencedCells(), std::vector{"A2"_pos});

    hr = f->HandleDeletedRows(0);
    ASSERT_EQUAL(f->GetExpression(), "A1");
    ASSERT_EQUAL(hr, IFormula::HandlingResult::ReferencesRenamedOnly);
    ASSERT_EQUAL(f->GetReferencedCells(), std::vector{"A1"_pos});

    const auto ref = ToString(FormulaError::Category::Ref);

    f = ParseFormula("A1+C3");
    ASSERT_EQUAL(f->GetReferencedCells(), (std::vector{"A1"_pos, "C3"_pos}));

    hr = f->HandleDeletedCols(1);
    ASSERT_EQUAL(f->GetExpression(), "A1+B3");
    ASSERT_EQUAL(hr, IFormula::HandlingResult::ReferencesRenamedOnly);
    ASSERT_EQUAL(f->GetReferencedCells(), (std::vector{"A1"_pos, "B3"_pos}));

    hr = f->HandleDeletedRows(1);
    ASSERT_EQUAL(f->GetExpression(), "A1+B2");
    ASSERT_EQUAL(hr, IFormula::HandlingResult::ReferencesRenamedOnly);
    ASSERT_EQUAL(f->GetReferencedCells(), (std::vector{"A1"_pos, "B2"_pos}));

    hr = f->HandleDeletedRows(0);
    ASSERT_EQUAL(f->GetExpression(), ref + "+B1");
    ASSERT_EQUAL(hr, IFormula::HandlingResult::ReferencesChanged);
    ASSERT_EQUAL(f->GetReferencedCells(), std::vector{"B1"_pos});

    hr = f->HandleDeletedCols(1);
    ASSERT_EQUAL(f->GetExpression(), ref + "+" + ref);
    ASSERT_EQUAL(hr, IFormula::HandlingResult::ReferencesChanged);
    ASSERT(f->GetReferencedCells().empty());
}

void TestErrorValue()
{
    auto sheet = CreateSheet();
    sheet->SetCell("E2"_pos, "A1");
    sheet->SetCell("E4"_pos, "=E2");
    ASSERT_EQUAL(sheet->GetCell("E4"_pos)->GetValue(), ICell::Value(FormulaError::Category::Value));

    sheet->SetCell("E2"_pos, "3D");
    ASSERT_EQUAL(sheet->GetCell("E4"_pos)->GetValue(), ICell::Value(FormulaError::Category::Value));
}

void TestErrorDiv0()
{
    auto sheet = CreateSheet();

    constexpr double max = std::numeric_limits<double>::max();

    sheet->SetCell("A1"_pos, "=1/0");
    ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), ICell::Value(FormulaError::Category::Div0));

    sheet->SetCell("A1"_pos, "=1e+200/1e-200");
    ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), ICell::Value(FormulaError::Category::Div0));

    sheet->SetCell("A1"_pos, "=0/0");
    ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), ICell::Value(FormulaError::Category::Div0));

    {
        std::ostringstream formula;
        formula << '=' << max << '+' << max;
        sheet->SetCell("A1"_pos, formula.str());
        ASSERT_EQUAL(
            sheet->GetCell("A1"_pos)->GetValue(), ICell::Value(FormulaError::Category::Div0)
        );
    }

    {
        std::ostringstream formula;
        formula << '=' << -max << '-' << max;
        sheet->SetCell("A1"_pos, formula.str());
        ASSERT_EQUAL(
            sheet->GetCell("A1"_pos)->GetValue(), ICell::Value(FormulaError::Category::Div0)
        );
    }

    {
        std::ostringstream formula;
        formula << '=' << max << '*' << max;
        sheet->SetCell("A1"_pos, formula.str());
        ASSERT_EQUAL(
            sheet->GetCell("A1"_pos)->GetValue(), ICell::Value(FormulaError::Category::Div0)
        );
    }
}

void TestEmptyCellTreatedAsZero()
{
    auto sheet = CreateSheet();
    sheet->SetCell("A1"_pos, "=B2");
    ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), ICell::Value(0));
}

void TestFormulaInvalidPosition()
{
    auto sheet = CreateSheet();
    auto try_formula = [&](const std::string& formula) {
        try {
            sheet->SetCell("A1"_pos, formula);
            ASSERT(false);
        } catch (const FormulaException&) {
            // we expect this one
        }
    };

    try_formula("=X0");
    try_formula("=ABCD1");
    try_formula("=A123456");
    try_formula("=ABCDEFGHIJKLMNOPQRS1234567890");
    try_formula("=XFD16385");
    try_formula("=XFE16384");
    try_formula("=R2D2");
}

void TestCellErrorPropagation()
{
    auto sheet = CreateSheet();
    sheet->SetCell("A1"_pos, "=1");
    sheet->SetCell("A2"_pos, "=A1");
    sheet->SetCell("A3"_pos, "=A2");
    sheet->DeleteRows(0);

    ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), ICell::Value(FormulaError::Category::Ref));
    ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetText(), "=" + ToString(FormulaError::Category::Ref));

    ASSERT_EQUAL(sheet->GetCell("A2"_pos)->GetValue(), ICell::Value(FormulaError::Category::Ref));
    ASSERT_EQUAL(sheet->GetCell("A2"_pos)->GetText(), "=A1");

    sheet->SetCell("B1"_pos, "=1/0");
    sheet->SetCell("A2"_pos, "=A1+B1");
    auto value = sheet->GetCell("A2"_pos)->GetValue();
    ASSERT(
        value == ICell::Value(FormulaError::Category::Ref)
        || value == ICell::Value(FormulaError::Category::Div0)
    );
}

void TestCellsDeletionSimple()
{
    auto sheet = CreateSheet();
    sheet->SetCell("A1"_pos, "1");
    sheet->SetCell("A2"_pos, "2");
    sheet->SetCell("A3"_pos, "3");
    sheet->DeleteRows(1);
    ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetText(), "1");
    ASSERT_EQUAL(sheet->GetCell("A2"_pos)->GetText(), "3");

    sheet = CreateSheet();
    sheet->SetCell("A1"_pos, "1");
    sheet->SetCell("B1"_pos, "2");
    sheet->SetCell("C1"_pos, "3");
    sheet->DeleteCols(1);
    ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetText(), "1");
    ASSERT_EQUAL(sheet->GetCell("B1"_pos)->GetText(), "3");
}

void TestCellsDeletion()
{
    auto sheet = CreateSheet();
    sheet->SetCell("A1"_pos, "=1");
    sheet->SetCell("A2"_pos, "=A1");
    sheet->SetCell("A3"_pos, "=A2");
    sheet->SetCell("B3"_pos, "=A1+A3");
    sheet->DeleteRows(1);
    ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetText(), "=1");
    ASSERT_EQUAL(sheet->GetCell("A2"_pos)->GetValue(), ICell::Value(FormulaError::Category::Ref));
    ASSERT_EQUAL(sheet->GetCell("B2"_pos)->GetText(), "=A1+A2");

    sheet = CreateSheet();
    sheet->SetCell("A1"_pos, "=1");
    sheet->SetCell("B1"_pos, "=A1");
    sheet->SetCell("C1"_pos, "=B1");
    sheet->SetCell("C2"_pos, "=A1+C1");
    sheet->DeleteCols(1);
    ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetText(), "=1");
    ASSERT_EQUAL(sheet->GetCell("B1"_pos)->GetValue(), ICell::Value(FormulaError::Category::Ref));
    ASSERT_EQUAL(sheet->GetCell("B2"_pos)->GetText(), "=A1+B1");
}

void TestCellsDeletionAdjacent()
{
    auto sheet = CreateSheet();
    sheet->SetCell("A2"_pos, "=1");
    sheet->SetCell("A3"_pos, "=A1+A2");
    sheet->DeleteRows(0);

    sheet = CreateSheet();
    sheet->SetCell("B1"_pos, "=1");
    sheet->SetCell("C1"_pos, "=A1+B1");
    sheet->DeleteCols(0);
}

void TestPrint()
{
    auto sheet = CreateSheet();
    sheet->SetCell("A2"_pos, "meow");
    sheet->SetCell("B2"_pos, "=35");

    ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{2, 2}));

    std::ostringstream texts;
    sheet->PrintTexts(texts);
    ASSERT_EQUAL(texts.str(), "\t\nmeow\t=35\n");

    std::ostringstream values;
    sheet->PrintValues(values);
    ASSERT_EQUAL(values.str(), "\t\nmeow\t35\n");
}

void TestCellReferences()
{
    auto sheet = CreateSheet();
    sheet->SetCell("A1"_pos, "1");
    sheet->SetCell("A2"_pos, "=A1");
    sheet->SetCell("B2"_pos, "=A1");

    ASSERT(sheet->GetCell("A1"_pos)->GetReferencedCells().empty());
    ASSERT_EQUAL(sheet->GetCell("A2"_pos)->GetReferencedCells(), std::vector{"A1"_pos});
    ASSERT_EQUAL(sheet->GetCell("B2"_pos)->GetReferencedCells(), std::vector{"A1"_pos});

    sheet->SetCell("B2"_pos, "=B1");
    ASSERT(sheet->GetCell("B1"_pos)->GetReferencedCells().empty());
    ASSERT_EQUAL(sheet->GetCell("B2"_pos)->GetReferencedCells(), std::vector{"B1"_pos});

    sheet->SetCell("A2"_pos, "");
    ASSERT(sheet->GetCell("A1"_pos)->GetReferencedCells().empty());
    ASSERT(sheet->GetCell("A2"_pos)->GetReferencedCells().empty());

    sheet->SetCell("B1"_pos, "=C3");
    ASSERT_EQUAL(sheet->GetCell("B1"_pos)->GetReferencedCells(), std::vector{"C3"_pos});
}

void TestFormulaIncorrect()
{
    auto isIncorrect = [](std::string expression) {
        try {
            ParseFormula(std::move(expression));
        } catch (const FormulaException&) {
            return true;
        }
        return false;
    };

    ASSERT(isIncorrect("A2B"));
    ASSERT(isIncorrect("3X"));
    ASSERT(isIncorrect("A0++"));
    ASSERT(isIncorrect("((1)"));
    ASSERT(isIncorrect("2+4-"));
}

void TestCellCircularReferences()
{
    {
        auto sheet = CreateSheet();
        sheet->SetCell("E2"_pos, "=E4");
        sheet->SetCell("E4"_pos, "=X9");
        sheet->SetCell("X9"_pos, "=M6");
        sheet->SetCell("M6"_pos, "Ready");

        bool caught = false;
        try {
            sheet->SetCell("M6"_pos, "=E2");
        } catch (const CircularDependencyException&) {
            caught = true;
        }

        ASSERT(caught);
        ASSERT_EQUAL(sheet->GetCell("M6"_pos)->GetText(), "Ready");
    }
    {
        auto sheet = CreateSheet();
        sheet->SetCell("A1"_pos, "=C3");
        sheet->SetCell("B2"_pos, "=X9");
        sheet->SetCell("C3"_pos, "=B2");
        sheet->SetCell("X9"_pos, "3");
        sheet->InsertCols(2, 4);
        sheet->DeleteCols(2, 4);
        bool caught = false;
        try {
            sheet->SetCell("X9"_pos, "=A1");
        } catch (const CircularDependencyException&) {
            caught = true;
        }
        ASSERT(caught);
        ASSERT_EQUAL(sheet->GetCell("X9"_pos)->GetText(), "3");

        sheet->InsertRows(0, 4);
        sheet->DeleteRows(0, 4);
        caught = false;
        try {
            sheet->SetCell("X9"_pos, "=A1");
        } catch (const CircularDependencyException&) {
            caught = true;
        }
        ASSERT(caught);
        ASSERT_EQUAL(sheet->GetCell("X9"_pos)->GetText(), "3");

        sheet->InsertCols(2, 4);
        sheet->InsertRows(0, 4);
        sheet->DeleteCols(2, 4);
        sheet->DeleteRows(0, 4);
        caught = false;
        try {
            sheet->SetCell("X9"_pos, "=A1");
        } catch (const CircularDependencyException&) {
            caught = true;
        }
        ASSERT(caught);
        ASSERT_EQUAL(sheet->GetCell("X9"_pos)->GetText(), "3");
    }
}

void TestChangeCellValue()
{
    auto sheet = CreateSheet();
    sheet->SetCell("A1"_pos, "3");
    sheet->SetCell("A2"_pos, "2");
    sheet->SetCell("A3"_pos, "=1");
    sheet->SetCell("B2"_pos, "=A3 + A2 + A1");
    sheet->GetCell("B2"_pos)->GetValue();
    ASSERT_EQUAL(sheet->GetCell("B2"_pos)->GetValue(), ICell::Value(6));
    sheet->SetCell("A1"_pos, "4");
    ASSERT_EQUAL(sheet->GetCell("B2"_pos)->GetValue(), ICell::Value(7));
}

void TestInvalidateCachedValues()
{
    auto sheet = CreateSheet();
    sheet->SetCell("A1"_pos, "=A2");
    sheet->SetCell("A2"_pos, "=A3");
    sheet->SetCell("A3"_pos, "=10");
    ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), ICell::Value(10));
    ASSERT_EQUAL(sheet->GetCell("A2"_pos)->GetValue(), ICell::Value(10));
    ASSERT_EQUAL(sheet->GetCell("A3"_pos)->GetValue(), ICell::Value(10));
    sheet->SetCell("A3"_pos, "=15");
    sheet->GetCell("A1"_pos)->GetValue();
    ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), ICell::Value(15));
    ASSERT_EQUAL(sheet->GetCell("A2"_pos)->GetValue(), ICell::Value(15));
    ASSERT_EQUAL(sheet->GetCell("A3"_pos)->GetValue(), ICell::Value(15));
    sheet->ClearCell("A3"_pos);
    ASSERT_EQUAL(sheet->GetCell("A3"_pos), nullptr);
    ASSERT_EQUAL(sheet->GetCell("A2"_pos)->GetValue(), ICell::Value(0));
    ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), ICell::Value(0));
}

void TestDeletedAllCells()
{
    {
        auto sheet = CreateSheet();
        sheet->SetCell("A1"_pos, "=A2");
        sheet->SetCell("A2"_pos, "=A3");
        sheet->SetCell("A3"_pos, "=10");
        sheet->DeleteCols(0, 1);
        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{0, 0}));
    }
    {
        auto sheet = CreateSheet();
        sheet->SetCell("A1"_pos, "10");
        sheet->ClearCell("A1"_pos);
        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{0, 0}));
    }
    {
        auto sheet = CreateSheet();
        sheet->SetCell("A1"_pos, "10");
        sheet->SetCell("A2"_pos, "20");
        sheet->SetCell("A3"_pos, "30");
        sheet->ClearCell("A1"_pos);
        sheet->ClearCell("A2"_pos);
        sheet->ClearCell("A3"_pos);
        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{0, 0}));
    }
}

void TestInsert()
{
    {
        auto sheet = CreateSheet();
        sheet->SetCell("A1"_pos, "=B2");
        sheet->SetCell("B2"_pos, "=C3");
        sheet->SetCell("C3"_pos, "=10");
        sheet->InsertCols(1, 3);
        ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), ICell::Value(10));
        ASSERT_EQUAL(sheet->GetCell("E2"_pos)->GetValue(), ICell::Value(10));
        ASSERT_EQUAL(sheet->GetCell("F3"_pos)->GetValue(), ICell::Value(10));
    }
    {
        auto sheet = CreateSheet();
        sheet->SetCell("A1"_pos, "=B2");
        sheet->SetCell("B2"_pos, "=C3");
        sheet->SetCell("C3"_pos, "=10");
        sheet->InsertRows(1, 3);
        ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), ICell::Value(10));
        ASSERT_EQUAL(sheet->GetCell("B5"_pos)->GetValue(), ICell::Value(10));
        ASSERT_EQUAL(sheet->GetCell("C6"_pos)->GetValue(), ICell::Value(10));
    }
}

void TestDeletion()
{
    {
        auto sheet = CreateSheet();
        sheet->SetCell("A1"_pos, "=B2");
        sheet->SetCell("B2"_pos, "=C3");
        sheet->SetCell("C3"_pos, "=10");
        sheet->InsertCols(1, 3);
        sheet->DeleteCols(1, 3);
        ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), ICell::Value(10));
        ASSERT_EQUAL(sheet->GetCell("B2"_pos)->GetValue(), ICell::Value(10));
        ASSERT_EQUAL(sheet->GetCell("C3"_pos)->GetValue(), ICell::Value(10));
    }
    {
        auto sheet = CreateSheet();
        sheet->SetCell("A1"_pos, "=B2");
        sheet->SetCell("B2"_pos, "=C3");
        sheet->SetCell("C3"_pos, "=10");
        sheet->InsertRows(1, 3);
        sheet->DeleteRows(1, 3);
        ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), ICell::Value(10));
        ASSERT_EQUAL(sheet->GetCell("B2"_pos)->GetValue(), ICell::Value(10));
        ASSERT_EQUAL(sheet->GetCell("C3"_pos)->GetValue(), ICell::Value(10));
    }
}

namespace {

    auto createLargeTable(std::unique_ptr<ISheet>& sheet, Position start, int rows, int cols, std::string defValue = "1")
    {
        std::cout << "createLargeTable: " << start.ToString() << std::endl;

        std::string largeFormula = "=";

        for (int row = start.row; row < start.row + rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                const auto pos = Position{ row, col };
                sheet->SetCell(pos, defValue);

                if (!(pos == start)) {
                    largeFormula += pos.ToString() + "+";
                }
            }
        }

        std::cout << "table filled" << std::endl;

        largeFormula.erase(largeFormula.end() - 1);

        sheet->SetCell(start, largeFormula);

        std::cout << "largeFormula set" << std::endl;
    }

    void createDAGFriendlyTable(std::unique_ptr<ISheet>& sheet, int rows, int cols) {
        std::cout << "createDAGFriendlyTable: " << rows << "x" << cols << std::endl;

        // 1. Заповнюємо перший ряд базовими значеннями
        for (int col = 0; col < cols; ++col) {
            sheet->SetCell(Position{ 0, col }, "1");
        }

        // 2. Інші рядки формулами, які залежать від попереднього рядка
        for (int row = 1; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                std::string formula = "=";

                // Додаємо залежності: зліва, центр, справа в попередньому рядку
                for (int delta = -1; delta <= 1; ++delta) {
                    int prev_col = col + delta;
                    if (prev_col >= 0 && prev_col < cols) {
                        formula += Position{ row - 1, prev_col }.ToString() + "+";
                    }
                }

                formula.pop_back(); // remove last '+'
                sheet->SetCell(Position{ row, col }, formula);
            }
        }

        std::cout << "DAG table created" << std::endl;
    }

    void createStressTestTable(std::unique_ptr<ISheet>& sheet, int rows, int cols) {
        std::cout << "Generating " << rows << "x" << cols << " DAG table in parallel..." << std::endl;

        using CellData = std::tuple<Position, std::string>;
        std::vector<CellData> prepared_cells;
        prepared_cells.reserve(rows * cols);

        std::vector<int> indices(cols);
        std::iota(indices.begin(), indices.end(), 0);

        // 1. Перший ряд — просто значення
        for (int col : indices) {
            prepared_cells.emplace_back(Position{ 0, col }, "1");
        }

        // 2. Наступні рядки — формули
        for (int row = 1; row < rows; ++row) {
            std::vector<CellData> row_cells(cols);

            std::for_each(std::execution::par, indices.begin(), indices.end(), [&](int col) {
                std::string formula = "=";
                for (int delta = -1; delta <= 1; ++delta) {
                    int prev_col = col + delta;
                    if (prev_col >= 0 && prev_col < cols) {
                        formula += Position{ row - 1, prev_col }.ToString() + "+";
                    }
                }
                formula.pop_back();
                row_cells[col] = { Position{row, col}, std::move(formula) };
                });

            prepared_cells.insert(prepared_cells.end(), row_cells.begin(), row_cells.end());

            if (row % 50 == 0) {
                std::cout << "  Row " << row << " of " << rows << " prepared" << std::endl;
            }
        }

        std::cout << "Filling sheet sequentially..." << std::endl;

        for (const auto& [pos, text] : prepared_cells) {
            sheet->SetCell(pos, text);  // безпечно, бо послідовно
        }

        std::cout << "DAG table filled." << std::endl;
    }

    std::string generateBalancedFormula(const std::vector<std::string>& vars) {
        if (vars.size() == 1) return vars[0];

        std::vector<std::string> next;
        for (size_t i = 0; i + 1 < vars.size(); i += 2) {
            next.push_back("(" + vars[i] + "+" + vars[i + 1] + ")");
        }
        // якщо непарна кількість — додаємо останній без об'єднання
        if (vars.size() % 2 != 0) {
            next.push_back(vars.back());
        }
        return generateBalancedFormula(next);
    }

    void GenerateHeavyFormula(std::unique_ptr<ISheet>& sheet, Position target, int count) {
        std::vector<std::string> refs;
        for (int i = 0; i < count; ++i) {
            Position pos{ 0, i };
            sheet->SetCell(pos, "1");
            refs.push_back(pos.ToString());
        }

        std::string formula = "=" + generateBalancedFormula(refs);
        sheet->SetCell(target, formula);
    }

}

void TestParallelFormulaEval() {
    auto sheet = CreateSheet();
    // Створимо формулу з 1024 клітинок (глибина ~10, ширина — широка)
    GenerateHeavyFormula(sheet, Position{ 1, 0 }, 1024);

    auto start = std::chrono::high_resolution_clock::now();
    auto value = sheet->GetCell({ 1, 0 })->GetValue();  // має викликати EvaluateParallel()
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Evaluated in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";
}


void TestParallelDAGTable() 
{
    auto sheet = CreateSheet();
    createStressTestTable(sheet, 20, 5002);
    //createDAGFriendlyTable(sheet, 10, 10000); // 100x100 DAG

    std::ofstream file("dag-table2.csv");
    sheet->PrintValues(file);
}

void TestLargeTable()
{
    auto sheet = CreateSheet();
    createLargeTable(sheet, Position{ 0,0 }, 100, 100);
    createLargeTable(sheet, Position{ 100,0 }, 100, 100, "2");
    createLargeTable(sheet, Position{ 200,0 }, 100, 100, "3");
    createLargeTable(sheet, Position{ 300,0 }, 100, 100, "4");

    std::ostringstream texts;

    std::ofstream file("table.csv");
    sheet->PrintValues(file);
}
