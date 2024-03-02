# Memory allocation

To allocate and free memory on the heap in e2, use `e2::create` and `e2::destroy` functions:
```c++
Foo* myFoo = e2::create<Foo>(3.14f);
e2::destroy(myFoo);
```
> Use these functions for any objects you allocate on the heap, regardless if they're reflected or not. 

If the type you are allocating is a managed object (`e2::ManagedObject`), it is automatically reference counted and comes with a few goodies. For example, you may use `e2::ManagedPtr` with it. A managed pointer is more or less a `std::shared_ptr` from the STL, and typedefs for them are automatically created by the reflection system:

```c++
// creates a e2::ManagedPtr<Foo> and makes it point to myFoo
FooPtr myFoo2 = myFoo;

// creates another managed pointer, except this pointer owns its object, and will destroy it when out of scope
FooPtr myFoo3 = FooPtr::create(3.14f);
```

When you create a managed pointer, that pointer makes sure that it's data stays valid for the duration of the pointers scope, even if the owner of `myFoo` calls `destroy`. As soon as a managed object is destroyed, and all of its managed pointers are out of scope, it is destroyed.

## STL to e2 cheatsheet
* `new Type(args)` becomes `e2::create<Type>(args)`
* `delete x` becomes `e2::destroy(x)`
* `std::shared_ptr<Type>` becomes `TypePtr`
* `e2::TypePtr::create(args)` creates a scoped RAII pointer


# Reflection system

Classes and their respective methods and fields can be reflected using the built-in static code generator.

In order for a class to be reflected, you may do 5 things:

1. Include `<e2/utils.hpp>`
1. Publicly inherit from `e2::ManagedObject`
1. Add `E2_CLASS_DECLARATION()` as the top line inside your class
1. Add `filename.generated.hpp` at the bottom of the file in which the class reside
1. Optionally add `@tags(...)` to your class, methods or fields to configure the reflection for these elements

Example:

```c++
#pragma once 
#include <e2/utils.hpp>

/** @tags(dynamic, arenaSize=32) */
class Foo : public e2::ManagedObject
{
    E2_CLASS_DECLARATION()
public:

}

#include "foo.generated.hpp"
```

## Tags

Tags are specified by writing `@tags(x, y)` within a comment, replacing `x` and `y` with an arbitrary number of tags that you may want.

### Tags for classes:

### `@tags(dynamic)`
`dynamic` enables this class to be instanced by name during runtime with `e2::createDynamic(...)`. This is required for components and entities for example, as they are deserialized from disk and their typename identified from disk data.

All types with this tag require a default constructor (taking no parameters). If you do not need dynamic 

### `@tags(arena)`

`arena` enables the class to automatically be allocated from a memory arena. This arena is a chunk of pre-allocated memory, specified using the tag `arenaSize`, and alleviates performance since allocations using this tag is basically free.

### `@tags(arenaSize=256)`

`arenaSize` specifies the size of it's memory arena. A value of `256` will allow up to 256 instances of the class, until it errors out. Arenas do not dynamically grow.

> Arena sizes for different classes (including builtins) can be overriden in `memory.cfg`. It is encouraged to profile your game when close to shipping, and narrow these values down, as it's a very effective way of optimizing memory usage.

