
{% for f in files %}
	{% if f.needs_init() %}
#include "{{ f.include_path }}"
	{% endif %}
{% endfor %}

namespace
{
	void registerGeneratedTypes()
	{
		{% for f in files %}
			{% for c in f.classes %}
				{% if "e2::Object" in c.deep_bases %}
		{{c.fqn}}::registerType();
				{% endif %}
			{% endfor %}
		{% endfor %}
	}
}
