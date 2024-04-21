#include "common.h"
#include "Sheet.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>


using namespace std;

// Position
bool Position::operator==(const Position& rhs) const
{
    return std::tie(row, col) == std::tie(rhs.row, rhs.col);
}

bool Position::operator<(const Position& rhs) const
{
    return std::tie(row, col) < std::tie(rhs.row, rhs.col);
}

bool Position::IsValid() const
{
    return row >= 0 && row < kMaxRows && col >= 0 && col < kMaxCols;
}

string Position::ToString() const
{
    if (!IsValid())
        return "";
    int dividend = col + 1;
    string column_name;
    int modulo;

    while (dividend > 0) {
        modulo = (dividend - 1) % 26;
        column_name = char('A' + modulo) + column_name;
        dividend = (int)((dividend - modulo) / 26);
    }
    return column_name + std::to_string(row + 1);
}

Position Position::FromString(std::string_view str)
{
    int row = 0, col = 0;
    int count = 0;
    string row_name;
    bool unknown_symbol = false;
    int index = 0;
    for (; index < str.size(); ++index) {
        if (str[index] >= 'A' && str[index] <= 'Z') {
            ++count;
        }
        else {
            break;
        }
    }
    for (; index < str.size(); ++index) {
        if (str[index] >= '0' && str[index] <= '9') {
            row_name += str[index];
        }
        else {
            unknown_symbol = true;
            break;
        }
    }
    if (str.empty() || count > 3 || unknown_symbol) {
        return Position{-1, -1};
    }
    int col_number = 0;
    int pos = count;
    for (int i = 0; i < pos; ++i) {
        col_number += std::pow(26, --count) * int(str[i] + 1 - 'A');
    }
    return Position{std::atoi(row_name.c_str()) - 1, col_number - 1};
}

// Position

// Size
bool Size::operator==(const Size& rhs) const
{
    return std::tie(rows, cols) == std::tie(rhs.rows, rhs.cols);
}

// Size

// FormulaError
FormulaError::FormulaError(Category category) : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const
{
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const
{
    return this->category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const
{
    if (category_ == Category::Ref) {
        return "#REF!";
    }
    else if (category_ == Category::Value) {
        return "#VALUE!";
    }
    else if (category_ == Category::Div0) {
        return "#DIV/0!";
    }
    else {
        throw runtime_error("unknown category");
    }
}

// FormulaError

std::ostream& operator<<(std::ostream& output, FormulaError fe)
{
    output << fe.ToString();
    return output;
}

std::ostream& operator<<(std::ostream& output, ICell::Value& value)
{
    std::visit([&](const auto& x) { output << x; }, value);
    return output;
}

std::unique_ptr<ISheet> CreateSheet()
{
    return make_unique<Sheet>();
}