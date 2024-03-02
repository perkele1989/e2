
#pragma once 

#include "foo.hpp"
#include "bar.hpp"

#include <e2/log.hpp>

#include "init.inl"


int main(int argc, char** argv)
{
	::registerGeneratedTypes();

	 
	/** Test e2::Ptr (passing) */
	if (true)
	{

		LogNotice("TestA");
		Bar* barA = e2::create<Bar>();
		e2::destroy(barA);

		LogNotice("TestB");
		{
			Bar* barB = e2::create<Bar>();
			LogNotice("TestB2");
			e2::Ptr<Bar> barC = barB;
			LogNotice("TestB3");
			e2::destroy(barB);
			LogNotice("TestB4");
		}
		LogNotice("TestB5");





		e2::Ptr<Bar> managedPtr1 = e2::Ptr<Bar>::create();
		{
			e2::Ptr<Bar> managedPtr2 = managedPtr1;
			managedPtr2->gr = 4.f;
		}
		LogNotice("Pod: {}", managedPtr1->gr);

		e2::Ptr<Bar> managedPtr3 = e2::Ptr<Bar>::create();
		e2::Ptr<Bar> managedPtr4 = managedPtr1;
		managedPtr4->gr = 1.0f;
		LogNotice("Pod: {}", managedPtr4->gr);
		managedPtr4 = managedPtr3;
		managedPtr4->gr = 3.0f;
		LogNotice("Pod: {}", managedPtr3->gr);
		LogNotice("Pod: {}", managedPtr4->gr);

		managedPtr1->keepAround();

	}

	LogNotice("PodZZZ");

	e2::ManagedObject::keepAroundPrune();

	LogNotice("PodCCC");

	return 0;
}