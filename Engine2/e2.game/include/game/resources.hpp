
#pragma once 

#include <cstdint>

namespace e2
{

	struct ResourceTable
	{
		int32_t wood{};
		int32_t stone{};
		int32_t metal{};
		int32_t gold{};
		int32_t oil{};
		int32_t uranium{};
		int32_t meteorite{};
	};


	struct GameResources
	{
		void calculateProfits();

		// funds, persistent value
		ResourceTable funds;

		// revenue streams, calculated at start of each round
		ResourceTable revenue;

		// expenses, calculated at the start of each round, can
		ResourceTable expenditures;

		// profits, calculated by (profits = revenue - expenditure)
		ResourceTable profits;
	};

}