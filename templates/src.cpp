#include "{{ file.include_path }}"
#include <e2/utils.hpp>

//#include "e2/managedptr_impl.inl"

{% set ns = namespace(cn=0) %}
{% for c in classes %}
    {% if c.fqn == "e2::Object" or "e2::Object" in c.deep_bases %}
        {% set cnt = "type" ~ ns.cn %}
        {% set cntC = "Type" ~ ns.cn %}
        {% set arena = "arena" in c.tags %}
namespace 
{
    e2::Type {{ cnt }};

        {% if arena %}
    e2::Arena<{{ c.fqn }}>* {{cnt}}Arena{};
        {% endif %}

}

// NOTE: This gap in the namespace is important.

		{% for constr in c.constructors %}
{{ c.fqn }}* e2::Allocator<{{c.fqn}}>::create({{ constr.get_args_string() }})
{
			{% if c.abstract %}
	LogError("attempted to create instance of abstract type '{{ c.fqn }}', returning null");
	return nullptr;
			{% elif arena %}
	return {{ cnt }}Arena->create({{ constr.get_args_string_no_type() }});
			{% else %}
	return new {{ c.fqn }}({{ constr.get_args_string_no_type() }});
			{% endif %}
}
		{% endfor %}

void e2::Allocator<{{c.fqn}}>::destroy({{c.fqn}}* instance)
{
	{% if "e2::ManagedObject" in c.deep_bases %}
	if(instance->block()->isPendingKill())
		if(instance->block()->objRef <= 0)
			instance->type()->destroy(instance);
		else
			LogError("invalid state for managed object of type {{c.fqn}} to be destroyed, expect leaks");
	else 
		instance->block()->flagPendingKill();
	{% else %}
	// This destroy function can be invoked for instances of subtypes, so we need to get the runtime type and destroy via that
	instance->type()->destroy(instance);
	{% endif %}
}

			
namespace 
{
    void register{{cntC}}()
    {
        {% if arena %}
         {{ cnt }}Arena = new e2::Arena<{{c.fqn}}>({{ c.get_arena_size() }});
        {% endif %}

		{% if c.abstract %}
		{{ cnt }}.isAbstract = true;
		{% else %}
		{{ cnt }}.isAbstract = false;
        {% endif %}

		{{ cnt }}.name = "{{ c.name }}";
		{{ cnt }}.fqn = "{{ c.fqn }}";

		// This function is generated for the purpose of dynamically instnacing objects by type, its NOT symmetric to destroy!
		{{ cnt }}.create = []() -> e2::Object*
		{
			{% if c.abstract %}
			LogError("attempted to create instance of abstract type '{{ c.fqn }}', returning null");
			return nullptr;
       		{% elif "dynamic" in c.tags %}
			return e2::create<{{ c.fqn }}>();
			{% else %}
			LogError("type {{ c.fqn }} not tagged dynamic, so we can't dynamically create them by name during runtime");
			return nullptr;
			{% endif %}
		};

		// This function is generated for the purpose of non-polymorphically destroying an instance of a given type, its NOT symmetric to create!!!
		{{ cnt }}.destroy = [](e2::Object* object)
		{
        {% if arena %}
			{{ cnt }}Arena->destroy(object->cast<{{ c.fqn }}>());
        {% else %}
			delete object;
        {% endif %}
		};

		{{ cnt }}.bases = {
		{% for base in c.bases %}
			"{{ base }}",
		{% endfor %}
		};

		{{ cnt }}.allBases = {
		{% for base in c.deep_bases %}
			"{{ base }}",
		{% endfor %}
		};

		{{ cnt }}.registerType();
    }
}


e2::Type const* {{ c.fqn }}::staticType()
{
	return &::{{ cnt }};
}

e2::Type const* {{ c.fqn }}::type()
{
	return &::{{ cnt }};
}

void {{ c.fqn }}::registerType()
{
	::register{{ cntC }}();
}



//template class e2::ManagedPtr<{{ c.fqn }}>;

		{% set ns.cn = ns.cn + 1 %}
	{% endif%}
{% endfor %}
