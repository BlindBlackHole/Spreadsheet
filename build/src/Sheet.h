#pragma once
#include "cell.h"

class Sheet : public ISheet
{
private:
    std::vector<std::vector<std::shared_ptr<Cell>>> cells;
    std::vector<std::weak_ptr<Cell>> largeCells;
    int cols_size = 0;
    int rows_size = 0;

public:
    Sheet();

    void SetCell(Position pos, std::string text) override;
    const ICell* GetCell(Position pos) const override;
    ICell* GetCell(Position pos) override;
    void ClearCell(Position pos) override;


    void InsertRows(int before, int count = 1) override;
    void InsertCols(int before, int count = 1) override;

    void DeleteRows(int first, int count = 1) override;
    void DeleteCols(int first, int count = 1) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;
};