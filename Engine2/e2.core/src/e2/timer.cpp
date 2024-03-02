
#include "e2/timer.hpp"

e2::Timer::Timer()
{
	reset();
}

void e2::Timer::reset()
{
	m_start = e2::timeNow();
}

double e2::Timer::seconds()
{
	return (e2::timeNow() - m_start).seconds();
}

e2::Moment e2::timeNow()
{
	return { std::chrono::steady_clock::now() };
}

e2::Duration e2::Duration::operator+(e2::Duration const& rhs)
{
	return { m_nanoseconds + rhs.m_nanoseconds };
}

e2::Duration e2::Duration::operator-(e2::Duration const& rhs)
{
	return { m_nanoseconds - rhs.m_nanoseconds };
}

double e2::Duration::seconds()
{
	return nanoseconds() * 0.000'000'001;
}

double e2::Duration::milliseconds()
{
	return nanoseconds() * 0.000'001;
}

double e2::Duration::microseconds()
{
	return nanoseconds() * 0.001;
}

double e2::Duration::nanoseconds()
{
	return (double)m_nanoseconds.count();
}

e2::Moment e2::Moment::epoch()
{
	return { std::chrono::time_point<std::chrono::steady_clock>() };
}

e2::Moment::Moment()
{
	*this= e2::timeNow();
}

e2::Moment::Moment(std::chrono::time_point<std::chrono::steady_clock> const& v)
{
	m_point = v;
}

e2::Duration e2::Moment::durationSince() const
{
	return e2::timeNow() - *this;
}

e2::Duration e2::Moment::operator-(e2::Moment const& rhs)
{
	return { m_point - rhs.m_point };
}
