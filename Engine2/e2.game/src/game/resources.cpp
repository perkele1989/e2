
#include "game/resources.hpp"

void e2::ResourceTable::write(e2::IStream& destination) const
{
	destination << gold;
	destination << wood;
	destination << iron;
	destination << steel;
}

bool e2::ResourceTable::read(e2::IStream& source)
{
	source >> gold;
	source >> wood;
	source >> iron;
	source >> steel;
	return true;
}

void e2::ResourceTable::clear()
{
	gold = 0.0f;
	wood = 0.0f;
	iron = 0.0f;
	steel = 0.0f;
}

e2::ResourceTable e2::ResourceTable::operator*(float v) const
{
	ResourceTable table = *this;
	table.gold *= v;
	table.wood *= v;
	table.iron *= v;
	table.steel *= v;
	return table;
}

e2::ResourceTable e2::ResourceTable::operator+(ResourceTable const& other) const
{
	ResourceTable table = *this;
	table += other;
	return table;
}

e2::ResourceTable e2::ResourceTable::operator-(ResourceTable const& other) const
{
	ResourceTable table = *this;
	table -= other;
	return table;
}

e2::ResourceTable& e2::ResourceTable::operator-=(ResourceTable const& other)
{
	gold -= other.gold;
	wood -= other.wood;
	iron -= other.iron;
	steel -= other.steel;
	
	return *this;
}

e2::ResourceTable& e2::ResourceTable::operator+=(ResourceTable const& other)
{
	gold += other.gold;
	wood += other.wood;
	iron += other.iron;
	steel += other.steel;
	
	return *this;
}

void e2::GameResources::write(e2::IStream& destination) const
{
	destination << funds;
}

bool e2::GameResources::read(e2::IStream& source)
{
	source >> funds;

	return true;
}

void e2::GameResources::collectRevenue()
{
	revenue.clear();
	for (e2::IFiscalStream* stream : fiscalStreams)
	{
		stream->collectRevenue(revenue);
	}
}

void e2::GameResources::collectExpenditures()
{
	expenditures.clear();
	for (e2::IFiscalStream* stream : fiscalStreams)
	{
		stream->collectExpenditure(expenditures);
	}
}

void e2::GameResources::calculateProfits()
{
	profits = revenue - expenditures;
}

void e2::GameResources::applyProfits()
{
	funds += profits;
}

void e2::GameResources::onNewTurn()
{
	collectRevenue();
	collectExpenditures();
	calculateProfits();
	applyProfits();
}
