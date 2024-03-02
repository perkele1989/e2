
#pragma once

#include <e2/utils.hpp>

{% for c in classes %}
	{% if c.fqn == "e2::Object" or "e2::Object" in c.deep_bases %}
		{% for n in c.namespace %}
namespace {{ n }}
{
		{% endfor %}
	class {{ c.name }};

	{% if c.fqn == "e2::ManagedObject" or "e2::ManagedObject" in c.deep_bases %}
	using {{c.name}}Ptr = e2::Ptr<{{c.fqn}}>;
	//using {{c.name}}WPtr = e2::WeakPtr<{{c.fqn}}>;
	{% endif%}

		{% for n in c.namespace %}
} 
		{% endfor %}
	{% endif%}
{% endfor %}

#if !defined(E2_SCG)

namespace e2
{

	{% for c in classes %}
		{% if c.fqn == "e2::Object" or "e2::Object" in c.deep_bases %}

		template<>
		class {{ api_define }} Allocator<{{c.fqn}}>
		{
		public:
			{% for con in c.constructors %}
				static {{c.fqn}}* create({{con.get_args_string()}});
			{% endfor%}

				static void destroy({{c.fqn}}* instance);
		};

		{% endif%}

	{% endfor%}

}
#endif