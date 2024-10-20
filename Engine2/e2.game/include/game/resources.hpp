
#pragma once 

#include <e2/utils.hpp>

#include <cstdint>
#include <unordered_set>

namespace e2
{

	class IFiscalStream;

	struct ResourceTable : public e2::Data
	{
		virtual void write(e2::IStream& destination) const override;
		virtual bool read(e2::IStream& source) override;

		float gold{};
		float wood{};
		float iron{};
		float steel{};

		void clear();

		ResourceTable operator*(float) const;

		ResourceTable operator+(ResourceTable const& other) const;

		ResourceTable operator-(ResourceTable const& other) const;


		ResourceTable& operator+=(ResourceTable const& other);

		ResourceTable& operator-=(ResourceTable const& other);
	};




	struct GameResources : public e2::Data
	{
		virtual void write(e2::IStream& destination) const override;
		virtual bool read(e2::IStream& source) override;

		void collectRevenue();
		void collectExpenditures();

		void calculateProfits();

		void applyProfits();

		void onNewTurn();
		
		// funds, persistent value
		ResourceTable funds;

		// revenue streams, calculated at start of each round
		ResourceTable revenue;

		// expenses, calculated at the start of each round
		ResourceTable expenditures;

		// profits, calculated by (profits = revenue - expenditure)
		ResourceTable profits;

		std::unordered_set<e2::IFiscalStream*> fiscalStreams;
	};

	class IFiscalStream
	{
	public:
		virtual void collectRevenue(ResourceTable& outRevenueTable) = 0;
		virtual void collectExpenditure(ResourceTable& outExpenditureTable) = 0;
	};

}