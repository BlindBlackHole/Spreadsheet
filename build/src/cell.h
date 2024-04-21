#pragma once

#include "common.h"
#include "formula.h"
#include <optional>
#include <unordered_set>

struct Hasher
{
    size_t operator()(const Position& pos) const;
};

class Cell : public ICell
{
private:
    Position position;
    Position prev_position;
    std::string text;
    std::string backup_text;
    ISheet& sheet;
    std::unique_ptr<IFormula> f;
    std::unique_ptr<IFormula> backup_f;
    mutable std::optional<Value> cached_val;
    IFormula::HandlingResult changes = IFormula::HandlingResult::NothingChanged;
    std::unordered_set<Position, Hasher> in_cells;
    std::unordered_set<Position, Hasher> out_cells;

private:
    void UpdateDependencies(const Position& prev_pos);

public:
    Cell(Position pos, std::string text, ISheet& sheet);

    using Value = std::variant<std::string, double, FormulaError>;

    void UpdateCell();

    void InvalidateValue();

    void RecursionIndalidate(Position pos);

    void InvalidateCachedValues();

    void SetPosition(Position pos);

    void Backup();

    bool isFormula() const;

    void SetText(std::string text);

    Value GetValue() const override;

    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    void UpdateInsertedCols(int before, int count);
    void UpdateInsertedRows(int before, int count);
    void UpdateDeletedRows(int first, int count);
    void UpdateDeletedCols(int first, int count);
};

std::ostream& operator<<(std::ostream& output, Cell::Value value);