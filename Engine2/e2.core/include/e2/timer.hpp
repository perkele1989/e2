
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>

#include <chrono>

namespace e2
{
	/**
	 * The C++ standard / STL for time shit is hot overengineered garbage, so we wrap it to make it actually usable 
	 */

	/** A duration of time represented in ticks of nanoseconds (precision depends on systems steady_clock) */
	struct E2_API Duration
	{
		e2::Duration operator+(e2::Duration const& rhs);
		e2::Duration operator-(e2::Duration const& rhs);

		double seconds();
		double milliseconds();
		double microseconds();
		double nanoseconds();

		std::chrono::duration<int64_t, std::nano> m_nanoseconds;
	};

	/** A moment/point in time */
	struct E2_API Moment
	{
		static Moment epoch();

		Moment(std::chrono::time_point<std::chrono::steady_clock> const& v);
		Moment();

		// The duration of time that has elapsed since this moment 
		e2::Duration durationSince() const;

		/** Finds the time passed between rhs and this */
		e2::Duration operator-(e2::Moment const& rhs);

		std::chrono::time_point<std::chrono::steady_clock> m_point;
	};

	/** Returns the current moment in time */
	E2_API Moment timeNow();

	/** Simple timer class */
	class E2_API Timer
	{
	public:
		Timer();

		void reset();

		double seconds();
	protected:
		e2::Moment m_start;
	};


}