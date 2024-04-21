#include "Sheet.h"
#include <algorithm>

using namespace std;


// Sheet
Sheet::Sheet() = default;

void Sheet::SetCell(Position pos, std::string text)
{
    if (!pos.IsValid()) {
        throw InvalidPositionException("position: " + pos.ToString() + " is invalid");
    }
    cols_size = std::max(cols_size, pos.col + 1);
    rows_size = std::max(rows_size, pos.row + 1);
    if (cells.size() <= pos.row) {
        cells.resize(pos.row + 1);
    }
    if (cells[pos.row].size() <= pos.col) {
        cells[pos.row].resize(cols_size);
    }
    if (cells[pos.row][pos.col] != nullptr && cells[pos.row][pos.col]->GetText() == text) {
        return;
    }
    else if (cells[pos.row][pos.col] != nullptr) {
        cells[pos.row][pos.col]->SetText(text);
        cells[pos.row][pos.col]->UpdateCell();
        try {
            cells[pos.row][pos.col]->InvalidateCachedValues();
        } catch (CircularDependencyException& exp) {
            cells[pos.row][pos.col]->Backup();
            throw exp;
        }
        return;
    }
    cells[pos.row][pos.col] = make_unique<Cell>(pos, text, *this);
}

const ICell* Sheet::GetCell(Position pos) const
{
    if (!pos.IsValid()) {
        throw InvalidPositionException("position: " + pos.ToString() + " is invalid");
    }
    if (pos.row >= cells.size() || pos.col >= cells[pos.row].size()) {
        return nullptr;
    }
    return cells[pos.row][pos.col].get();
}

ICell* Sheet::GetCell(Position pos)
{
    if (!pos.IsValid()) {
        throw InvalidPositionException("position: " + pos.ToString() + " is invalid");
    }
    if (pos.row < cells.size() && pos.col >= cells[pos.row].size() && pos.col < cols_size) {
        SetCell(pos, "");
    }
    if (pos.row >= cells.size() || pos.col >= cols_size) {
        return nullptr;
    }
    return cells[pos.row][pos.col].get();
}

void Sheet::ClearCell(Position pos)
{
    if (!pos.IsValid()) {
        throw InvalidPositionException("position: " + pos.ToString() + " is invalid");
    }
    if (!cells.empty() && cells.size() > pos.row && cells[pos.row].size() > pos.col) {
        cells[pos.row][pos.col]->InvalidateCachedValues();
        cells[pos.row][pos.col] = nullptr;
        int rows, cols;
        rows = rows_size;
        cols = cols_size;
        rows_size = 0;
        cols_size = 0;
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols && cells[row].size(); ++col) {
                if (cells[row][col] != nullptr) {
                    rows_size = std::max(rows_size, row + 1);
                    cols_size = std::max(cols_size, col + 1);
                }
            }
        }
    }
}

void Sheet::InsertRows(int before, int count)
{
    if (cells.size() + count >= Position::kMaxRows) {
        throw TableTooBigException("cells.size() + count >= Position::kMaxRows");
    }
    size_t prev_size = cells.size();
    cells.resize(prev_size + count);
    rows_size = cells.size();
    for (int row = prev_size - 1; row >= before; --row) {
        std::swap(cells[row], cells[row + count]);
    }
    for (size_t row = 0; row < cells.size(); ++row) {
        for (size_t col = 0; col < cells[row].size(); ++col) {
            if (cells[row][col]) {
                cells[row][col]->SetPosition({(int)row, (int)col});
                cells[row][col]->UpdateInsertedRows(before, count);
            }
        }
    }
}

void Sheet::InsertCols(int before, int count)
{
    if (cols_size + count >= Position::kMaxRows) {
        throw TableTooBigException("cells.size() + count >= Position::kMaxRows");
    }
    size_t prev_size = cols_size;
    for (size_t i = 0; i < cells.size(); ++i) {
        cells[i].resize(cols_size + count);
    }
    cols_size += count;
    for (int row = 0; row < cells.size(); ++row) {
        for (int col = prev_size - 1; col >= before; --col) {
            std::swap(cells[row][col], cells[row][col + count]);
        }
    }
    for (size_t row = 0; row < cells.size(); ++row) {
        for (size_t col = 0; col < cells[row].size(); ++col) {
            if (cells[row][col]) {
                cells[row][col]->SetPosition({(int)row, (int)col});
                cells[row][col]->UpdateInsertedCols(before, count);
            }
        }
    }
}

void Sheet::DeleteRows(int first, int count)
{
    // delete all rows from first to min(first + count, rows_size)
    if (first + count > rows_size) {
        throw TableTooBigException("Deleting rows: first + count > rows_size");
    }
    for (size_t row = first; row < (first + count) && rows_size; ++row) {
        for (size_t col = 0; col < cells[row].size(); ++col) {
            cells[row][col] = nullptr;
        }
    }
    for (size_t row = first + count; row < rows_size; ++row) {
        std::swap(cells[row], cells[row - count]);
    }
    for (size_t row = 0; row < cells.size(); ++row) {
        for (size_t col = 0; col < cells[row].size(); ++col) {
            if (cells[row][col]) {
                cells[row][col]->SetPosition({(int)row, (int)col});
                cells[row][col]->UpdateDeletedRows(first, count);
            }
        }
    }
    rows_size -= count;
    cols_size = rows_size == 0 ? 0 : cols_size;
}

void Sheet::DeleteCols(int first, int count)
{
    if (first + count > cols_size) {
        throw TableTooBigException("Deleting cols: first + count > cols_size");
    }
    for (size_t row = 0; row < rows_size; ++row) {
        for (size_t col = first; col < first + count && cells[row].size(); ++col) {
            cells[row][col] = nullptr;
        }
    }
    for (size_t row = 0; row < rows_size; ++row) {
        for (size_t col = first + count; col < cols_size && cells[row].size(); ++col) {
            std::swap(cells[row][col], cells[row][col - count]);
        }
    }
    for (size_t row = 0; row < cells.size(); ++row) {
        for (size_t col = 0; col < cells[row].size(); ++col) {
            if (cells[row][col]) {
                cells[row][col]->SetPosition({(int)row, (int)col});
                cells[row][col]->UpdateDeletedCols(first, count);
            }
        }
    }
    cols_size -= count;
    rows_size = cols_size == 0 ? 0 : rows_size;
}

Size Sheet::GetPrintableSize() const
{
    return Size{rows_size, cols_size};
}

void Sheet::PrintValues(std::ostream& output) const
{
    for (size_t row = 0; row < cells.size(); ++row) {
        for (size_t col = 0; col < cols_size; ++col) {
            if (cells[row].size() > col && cells[row][col]) {
                output << cells[row][col]->GetValue();
            }
            if (col != cols_size - 1)
                output << '\t';
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const
{
    /*output << "\\";
    for (size_t col = 0; col < cols_size; ++col) {
            output << Position{ 0, (int)col }.ToString() << '\t';
    }
    output << '\n';*/
    for (size_t row = 0; row < cells.size(); ++row) {
        // output << row + 1;
        for (size_t col = 0; col < cols_size; ++col) {
            if (cells[row].size() > col && cells[row][col]) {
                output << cells[row][col]->GetText();
            }
            if (col != cols_size - 1)
                output << '\t';
        }
        output << '\n';
    }
}

// Sheet