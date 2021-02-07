#include "../common.h";
#include "../formula.h"
#include "../build/cell.h"
#include "../build/Sheet.h"
#include <algorithm>
#include <optional>
#include <string>
#include <iostream>
#include <unordered_set>
#include <cmath>
#include <cstdlib>

using namespace std;

//Position
bool Position::operator==(const Position& rhs) const {
	return std::tie(row, col) == std::tie(rhs.row, rhs.col);
}

bool Position::operator<(const Position& rhs) const {
	return std::tie(row, col) < std::tie(rhs.row, rhs.col);
}

bool Position::IsValid() const {
	return row >= 0 && row < kMaxRows && col >=0 && col <kMaxCols;
}

string Position::ToString() const {
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

Position Position::FromString(std::string_view str) {
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
		return Position{ -1, -1 };
	}
	int col_number = 0;
	int pos = count;
	for (int i = 0; i < pos; ++i) {
		col_number += std::pow(26, --count) * int(str[i] + 1 - 'A');
	}
	return Position{ std::atoi(row_name.c_str()) - 1, col_number - 1 };
}
//Position

//Size
bool Size::operator==(const Size& rhs) const {
	return std::tie(rows, cols) == std::tie(rhs.rows, rhs.cols);
}
//Size

//FormulaError
FormulaError::FormulaError(Category category) : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const {
	return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
	return this->category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
	if (category_ == Category::Ref) {
		return "#REF!";
	} else if (category_ == Category::Value) {
		return "#VALUE!";
	}
	else if (category_ == Category::Div0) {
		return "#DIV/0!";
	}
	else {
		throw runtime_error("unknown category");
	}
}
//FormulaError

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
	output << fe.ToString();
	return output;
}

std::ostream& operator<<(std::ostream& output, ICell::Value& value) {
	std::visit([&](const auto& x) { output << x; }, value);
	return output;
}

//struct Hasher {
//	size_t operator()(const Position& pos) const {
//		const size_t coef = 2'946'901;
//
//		const hash<int> int_hasher;
//
//		return (
//			coef * int_hasher(pos.row) +
//			int_hasher(pos.col)
//			);
//	}
//};
//
//class Cell : public ICell {
//private:
//	Position position;
//	Position prev_position;
//	std::string text;
//	std::string backup_text;
//	ISheet& sheet;
//	std::unique_ptr<IFormula> f;
//	std::unique_ptr<IFormula> backup_f;
//	mutable std::optional<Value> cached_val;
//	IFormula::HandlingResult changes = IFormula::HandlingResult::NothingChanged;
//	std::unordered_set<Position, Hasher> in_cells;
//	std::unordered_set<Position, Hasher> out_cells;
//private:
//	void UpdateDependencies(const Position& prev_pos) {
//		out_cells = {};
//		//in_cells = {};
//		for (const Position& ref_pos : f->GetReferencedCells()) { //O(N)
//			//add ougoing cell
//			out_cells.insert(ref_pos);
//			//add incoming cell
//			auto cell = dynamic_cast<Cell*>(sheet.GetCell(ref_pos));
//			if (!cell) {
//				sheet.SetCell(ref_pos, "");
//				cell = dynamic_cast<Cell*>(sheet.GetCell(ref_pos));
//			}
//			cell->in_cells.erase(prev_pos);
//			cell->in_cells.insert(position);
//		}
//	}
//public:
//	Cell(Position pos, std::string text, ISheet& sheet) // O(N)
//		: position(pos), text(text), sheet(sheet)
//	{
//		if (text.size() > 1 && text[0] == '=') {
//			f = ParseFormula(text.substr(1, text.size() - 1));
//			UpdateDependencies(position);
//		}
//	}
//
//	void UpdateCell() {
//		backup_f = std::move(f);
//		if (isFormula()) {
//			f = ParseFormula(text.substr(1, text.size() - 1));
//			UpdateDependencies(position);
//		}
//	}
//
//	void InvalidateValue() {
//		cached_val = nullopt;
//	}
//
//	void RecursionIndalidate(Position pos) {
//		Cell* cell = dynamic_cast<Cell*>(sheet.GetCell(pos));
//		if (cell) {
//			cell->InvalidateValue();
//			for (const auto& in_pos : cell->in_cells) {
//				for (const auto& out_pos : out_cells) {
//					if (pos == out_pos)
//						throw CircularDependencyException("");
//				}
//				RecursionIndalidate(in_pos);
//			}
//		}
//	}
//
//	void InvalidateCachedValues() {
//		cached_val = nullopt;
//		RecursionIndalidate(position);
//	}
//
//	void SetPosition(Position pos) {
//		prev_position = position;
//		position = pos;
//	}
//
//	void Backup() {
//		text = backup_text;
//		f = std::move(backup_f);
//	}
//
//	bool isFormula() const {
//		return text.size() > 1 && text[0] == '=';
//	}
//
//	void SetText(std::string text) {
//		backup_text = this->text;
//		this->text = text;
//	}
//
//	using Value = std::variant<std::string, double, FormulaError>;
//
//	Value GetValue() const override { //O(N)
//		if (cached_val)
//			return *cached_val;
//		//Value value;
//		if (changes == IFormula::HandlingResult::ReferencesChanged)
//			return FormulaError::Category::Ref;
//		// Is formula
//		if (text.size() > 1 && text[0] == '=') {
//			auto v = f->Evaluate(sheet);
//			if (auto number = std::get_if<double>(&v)) {
//				cached_val = *number;
//			}
//			else if (auto error = std::get_if<FormulaError>(&v)) {
//				cached_val = *error;
//			}
//		}
//		//Is text look like "'=A2+B2" and will be only text, not formula
//		else if (text.size() > 1 && text[0] == '\'') {
//			cached_val = text.substr(1, text.size() - 1);
//		}
//		//Simple text or number(as text)
//		else {
//			cached_val = text;
//		}
//		return *cached_val;
//	}
//
//	std::string GetText() const override { // O(1)
//		return text;
//	}
//
//	std::vector<Position> GetReferencedCells() const override { // O(1)
//		if (!f) {
//			return {};
//		}
//		return f->GetReferencedCells();
//	}
//
//	void UpdateInsertedCols(int before, int count) { // O(N)
//		if (isFormula()) {
//			changes = f->HandleInsertedCols(before, count);
//			text = '=' + f->GetExpression();
//			UpdateDependencies(prev_position);
//		}
//	}
//
//	void UpdateInsertedRows(int before, int count) {
//		if (isFormula()) {
//			changes = f->HandleInsertedRows(before, count);
//			text = '=' + f->GetExpression();
//			UpdateDependencies(prev_position);
//		}
//	}
//
//	void UpdateDeletedRows(int first, int count) {
//		if (isFormula()) {
//			changes = f->HandleDeletedRows(first, count);
//			text = '=' + f->GetExpression();
//			UpdateDependencies(prev_position);
//		}
//	}
//
//	void UpdateDeletedCols(int first, int count) {
//		if (isFormula()) {
//			changes = f->HandleDeletedCols(first, count);
//			text = '=' + f->GetExpression();
//			UpdateDependencies(prev_position);
//		}
//	}
//};
//
//std::ostream& operator<<(std::ostream& output, Cell::Value value) {
//	std::visit([&](const auto& x) { output << x; }, value);
//	return output;
//}

//class Sheet : public ISheet {
//private:
//	vector<vector<unique_ptr<Cell>>> cells;
//	int cols_size=0;
//	int rows_size=0;
//public:
//	Sheet() {}
//
//	void SetCell(Position pos, std::string text) {
//		if (!pos.IsValid()) {
//			throw InvalidPositionException("position: " + pos.ToString() + " is invalid");
//		}
//		cols_size = std::max(cols_size, pos.col + 1);
//		rows_size = std::max(rows_size, pos.row + 1);
//		if (cells.size() <= pos.row) {
//			cells.resize(pos.row + 1);
//		}
//		if (cells[pos.row].size() <= pos.col) {
//			cells[pos.row].resize(cols_size);
//		}
//		if (cells[pos.row][pos.col] != nullptr && cells[pos.row][pos.col]->GetText() == text) {
//			return;
//		}else if (cells[pos.row][pos.col] != nullptr) {
//			cells[pos.row][pos.col]->SetText(text);
//			cells[pos.row][pos.col]->UpdateCell();
//			try {
//				cells[pos.row][pos.col]->InvalidateCachedValues();
//			}
//			catch (CircularDependencyException& exp) {
//				cells[pos.row][pos.col]->Backup();
//				throw exp;
//			}
//			return;
//		}
//		cells[pos.row][pos.col] = make_unique<Cell>(pos, text, *this);
//	}
//	const ICell* GetCell(Position pos) const {
//		if (!pos.IsValid()) {
//			throw InvalidPositionException("position: " + pos.ToString() + " is invalid");
//		}
//		if (pos.row >= cells.size() || pos.col >= cells[pos.row].size()) {
//			return nullptr;
//		}
//		return cells[pos.row][pos.col].get();
//	}
//	ICell* GetCell(Position pos) {
//		if (!pos.IsValid()) {
//			throw InvalidPositionException("position: " + pos.ToString() + " is invalid");
//		}
//		if (pos.row < cells.size() && pos.col >= cells[pos.row].size() && pos.col < cols_size) {
//			SetCell(pos, "");
//		}
//		if (pos.row >= cells.size() || pos.col >= cols_size) {
//			return nullptr;
//		}
//		return cells[pos.row][pos.col].get();
//	}
//	void ClearCell(Position pos) {
//		if (!pos.IsValid()) {
//			throw InvalidPositionException("position: " + pos.ToString() + " is invalid");
//		}
//		if (!cells.empty() && cells.size() > pos.row && cells[pos.row].size() > pos.col) {
//			cells[pos.row][pos.col]->InvalidateCachedValues();
//			cells[pos.row][pos.col] = nullptr;
//			int rows, cols;
//			rows = rows_size;
//			cols = cols_size;
//			rows_size = 0;
//			cols_size = 0;
//			for (int row = 0; row < rows; ++row) {
//				for (int col = 0; col < cols && cells[row].size(); ++col) {
//					if (cells[row][col] != nullptr) {
//						rows_size = std::max(rows_size, row + 1);
//						cols_size = std::max(cols_size, col + 1);
//					}
//				}
//			}
//		}
//	}
//	
//
//	void InsertRows(int before, int count = 1) {
//		if (cells.size() + count >= Position::kMaxRows) {
//			throw TableTooBigException("cells.size() + count >= Position::kMaxRows");
//		}
//		size_t prev_size = cells.size();
//		cells.resize(prev_size + count);
//		rows_size = cells.size();
//		for (int row = prev_size - 1; row >= before; --row) {
//			std::swap(cells[row], cells[row + count]);
//		}
//		for (size_t row = 0; row < cells.size(); ++row) {
//			for (size_t col = 0; col < cells[row].size(); ++col) {
//				if (cells[row][col]) {
//					cells[row][col]->SetPosition({ (int)row, (int)col });
//					cells[row][col]->UpdateInsertedRows(before, count);
//				}
//			}
//		}
//	}
//	void InsertCols(int before, int count = 1) {
//		if (cols_size + count >= Position::kMaxRows) {
//			throw TableTooBigException("cells.size() + count >= Position::kMaxRows");
//		}
//		size_t prev_size = cols_size;
//		for (size_t i = 0; i < cells.size(); ++i) {
//			cells[i].resize(cols_size + count);
//		}
//		cols_size += count;
//		for (int row = 0; row < cells.size(); ++row) {
//			for (int col = prev_size - 1; col >= before; --col) {
//				std::swap(cells[row][col], cells[row][col + count]);
//			}
//		}
//		for (size_t row = 0; row < cells.size(); ++row) {
//			for (size_t col = 0; col < cells[row].size(); ++col) {
//				if (cells[row][col]) {
//					cells[row][col]->SetPosition({ (int)row, (int)col });
//					cells[row][col]->UpdateInsertedCols(before, count);
//				}
//			}
//		}
//	}
//	void DeleteRows(int first, int count = 1) {
//		//delete all rows from first to min(first + count, rows_size)
//		if (first + count > rows_size) {
//			throw TableTooBigException("Deleting rows: first + count > rows_size");
//		}
//		for (size_t row = first; row < (first + count) && rows_size; ++row) {
//			for (size_t col = 0; col < cells[row].size(); ++col) {
//				cells[row][col] = nullptr;
//			}
//		}
//		for (size_t row = first + count; row < rows_size; ++row) {
//			std::swap(cells[row], cells[row - count]);
//		}
//		for (size_t row = 0; row < cells.size(); ++row) {
//			for (size_t col = 0; col < cells[row].size(); ++col) {
//				if (cells[row][col]) {
//					cells[row][col]->SetPosition({ (int)row, (int)col });
//					cells[row][col]->UpdateDeletedRows(first, count);
//				}
//			}
//		}
//		rows_size -= count;
//		cols_size = rows_size == 0 ? 0 : cols_size;
//	}
//	void DeleteCols(int first, int count = 1) {
//		if (first + count > cols_size) {
//			throw TableTooBigException("Deleting cols: first + count > cols_size");
//		}
//		for (size_t row = 0; row < rows_size; ++row) {
//			for (size_t col = first; col < first + count && cells[row].size(); ++col) {
//				cells[row][col] = nullptr;
//			}
//		}
//		for (size_t row = 0; row < rows_size; ++row) {
//			for (size_t col = first + count; col < cols_size && cells[row].size(); ++col) {
//				std::swap(cells[row][col], cells[row][col - count]);
//			}
//		}
//		for (size_t row = 0; row < cells.size(); ++row) {
//			for (size_t col = 0; col < cells[row].size(); ++col) {
//				if (cells[row][col]) {
//					cells[row][col]->SetPosition({ (int)row, (int)col });
//					cells[row][col]->UpdateDeletedCols(first, count);
//				}
//			}
//		}
//		cols_size -= count;
//		rows_size = cols_size == 0 ? 0 : rows_size;
//	}
//	Size GetPrintableSize() const {
//		return Size{ rows_size, cols_size };
//	}
//	void PrintValues(std::ostream& output) const {
//		for (size_t row = 0; row < cells.size(); ++row) {
//			for (size_t col = 0; col < cols_size; ++col) {
//				if (cells[row].size() > col && cells[row][col]) {
//					output << cells[row][col]->GetValue();
//				}
//				if (col != cols_size - 1)
//					output << '\t';
//			}
//			output << '\n';
//		}
//	}
//	void PrintTexts(std::ostream& output) const {
//		/*output << "\\";
//		for (size_t col = 0; col < cols_size; ++col) {
//			output << Position{ 0, (int)col }.ToString() << '\t';
//		}
//		output << '\n';*/
//		for (size_t row = 0; row < cells.size(); ++row) {
//			//output << row + 1;
//			for (size_t col = 0; col < cols_size; ++col) {
//				if (cells[row].size() > col && cells[row][col]) {
//					output << cells[row][col]->GetText();
//				}
//				if(col != cols_size - 1)
//					output << '\t';
//			}
//			output << '\n';
//		}
//	}
//};

std::unique_ptr<ISheet> CreateSheet() {
	return make_unique<Sheet>();
}