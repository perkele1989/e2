
#include "game/resources.hpp"

void e2::GameResources::calculateProfits()
{
	profits.gold = revenue.gold - expenditures.gold;
	profits.wood = revenue.wood - expenditures.wood;
	profits.stone = revenue.stone - expenditures.stone;
	profits.metal = revenue.metal - expenditures.metal;
	profits.oil = revenue.oil - expenditures.oil;
	profits.uranium = revenue.uranium - expenditures.uranium;
	profits.meteorite = revenue.meteorite - expenditures.meteorite;
}