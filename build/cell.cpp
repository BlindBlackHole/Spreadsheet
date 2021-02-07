#include "../build/cell.h";

using namespace std;

//Hasher
size_t Hasher::operator()(const Position& pos) const {
	const size_t coef = 2'946'901;

	const hash<int> int_hasher;

	return (
		coef * int_hasher(pos.row) +
		int_hasher(pos.col)
		);
}
//Hasher


//Cell

void Cell::UpdateDependencies(const Position& prev_pos) {
	out_cells = {};
	//in_cells = {};
	for (const Position& ref_pos : f->GetReferencedCells()) { //O(N)
		//add ougoing cell
		out_cells.insert(ref_pos);
		//add incoming cell
		auto cell = dynamic_cast<Cell*>(sheet.GetCell(ref_pos));
		if (!cell) {
			sheet.SetCell(ref_pos, "");
			cell = dynamic_cast<Cell*>(sheet.GetCell(ref_pos));
		}
		cell->in_cells.erase(prev_pos);
		cell->in_cells.insert(position);
	}
}

Cell::Cell(Position pos, std::string text, ISheet& sheet) // O(N)
	: position(pos), text(text), sheet(sheet) {
	if (text.size() > 1 && text[0] == '=') {
		f = ParseFormula(text.substr(1, text.size() - 1));
		UpdateDependencies(position);
	}
}

void Cell::UpdateCell() {
	backup_f = std::move(f);
	if (isFormula()) {
		f = ParseFormula(text.substr(1, text.size() - 1));
		UpdateDependencies(position);
	}
}

void Cell::InvalidateValue() {
	cached_val = nullopt;
}

void Cell::RecursionIndalidate(Position pos) {
	Cell* cell = dynamic_cast<Cell*>(sheet.GetCell(pos));
	if (cell) {
		cell->InvalidateValue();
		for (const auto& in_pos : cell->in_cells) {
			for (const auto& out_pos : out_cells) {
				if (pos == out_pos)
					throw CircularDependencyException("");
			}
			RecursionIndalidate(in_pos);
		}
	}
}

void Cell::InvalidateCachedValues() {
	cached_val = nullopt;
	RecursionIndalidate(position);
}

void Cell::SetPosition(Position pos) {
	prev_position = position;
	position = pos;
}

void Cell::Backup() {
	text = backup_text;
	f = std::move(backup_f);
}

bool Cell::isFormula() const {
	return text.size() > 1 && text[0] == '=';
}

void Cell::SetText(std::string text) {
	backup_text = this->text;
	this->text = text;
}

Cell::Value Cell::GetValue() const { //O(N)
	if (cached_val)
		return *cached_val;
	//Value value;
	if (changes == IFormula::HandlingResult::ReferencesChanged)
		return FormulaError::Category::Ref;
	// Is formula
	if (text.size() > 1 && text[0] == '=') {
		auto v = f->Evaluate(sheet);
		if (auto number = std::get_if<double>(&v)) {
			cached_val = *number;
		}
		else if (auto error = std::get_if<FormulaError>(&v)) {
			cached_val = *error;
		}
	}
	//Is text look like "'=A2+B2" and will be only text, not formula
	else if (text.size() > 1 && text[0] == '\'') {
		cached_val = text.substr(1, text.size() - 1);
	}
	//Simple text or number(as text)
	else {
		cached_val = text;
	}
	return *cached_val;
}

std::string Cell::GetText() const { // O(1)
	return text;
}

std::vector<Position> Cell::GetReferencedCells() const { // O(1)
	if (!f) {
		return {};
	}
	return f->GetReferencedCells();
}

void Cell::UpdateInsertedCols(int before, int count) { // O(N)
	if (isFormula()) {
		changes = f->HandleInsertedCols(before, count);
		text = '=' + f->GetExpression();
		UpdateDependencies(prev_position);
	}
}

void Cell::UpdateInsertedRows(int before, int count) {
	if (isFormula()) {
		changes = f->HandleInsertedRows(before, count);
		text = '=' + f->GetExpression();
		UpdateDependencies(prev_position);
	}
}

void Cell::UpdateDeletedRows(int first, int count) {
	if (isFormula()) {
		changes = f->HandleDeletedRows(first, count);
		text = '=' + f->GetExpression();
		UpdateDependencies(prev_position);
	}
}

void Cell::UpdateDeletedCols(int first, int count) {
	if (isFormula()) {
		changes = f->HandleDeletedCols(first, count);
		text = '=' + f->GetExpression();
		UpdateDependencies(prev_position);
	}
}
//Cell

std::ostream& operator<<(std::ostream& output, Cell::Value value) {
	std::visit([&](const auto& x) { output << x; }, value);
	return output;
}



