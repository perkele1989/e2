
#include "game/resources.hpp"

void e2::ResourceTable::clear()
{
	wood = 0.0f;
	stone = 0.0f;
	metal = 0.0f;
	gold = 0.0f;
	oil = 0.0f;
	uranium = 0.0f;
	meteorite = 0.0f;
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
	wood -= other.wood;
	stone -= other.stone;
	metal -= other.metal;
	gold -= other.gold;
	oil -= other.oil;
	uranium -= other.uranium;
	meteorite -= other.meteorite;

	return *this;
}

e2::ResourceTable& e2::ResourceTable::operator+=(ResourceTable const& other)
{
	wood += other.wood;
	stone += other.stone;
	metal += other.metal;
	gold += other.gold;
	oil += other.oil;
	uranium += other.uranium;
	meteorite += other.meteorite;

	return *this;
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
