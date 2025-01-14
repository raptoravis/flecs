#include <cpp_api.h>

namespace ns {
struct NestedNameSpaceType { };

class NestedModule {
public:
    NestedModule(flecs::world& world) {
        flecs::module<ns::NestedModule>(world, "ns::NestedModule");
        flecs::component<Velocity>(world, "Velocity");
    }
};

class SimpleModule {
public:
    SimpleModule(flecs::world& world) {
        flecs::module<ns::SimpleModule>(world, "ns::SimpleModule");
        flecs::import<ns::NestedModule>(world);
        flecs::component<Position>(world, "Position");
    }
};

class NestedTypeModule {
public:
    struct NestedType { };

    NestedTypeModule(flecs::world& world) {
        world.module<NestedTypeModule>();
        world.component<NestedType>();
        world.component<NestedNameSpaceType>();
    }
};

class ModuleWithDifferentName {
public:
    ModuleWithDifferentName(flecs::world& world) {
        world.module<ModuleWithDifferentName>("ns::name_1");
        world.component<Position>("Position");
    }
};

class ModuleWithImport {
public:
    ModuleWithImport(flecs::world& world) {
        world.module<ModuleWithImport>("ns::name_2");
        world.import<ModuleWithDifferentName>();

        // Ensure imported component can be used by systems
        world.system<Position>()
            .each([](flecs::entity e, Position& p) { });

        world.system<Position>()
            .iter([](flecs::iter e, Position *p) { });        
    }
};

}

void Module_import() {
    flecs::world world;
    auto m = flecs::import<ns::SimpleModule>(world);
    test_assert(m.id() != 0);
    test_str(m.path().c_str(), "::ns::SimpleModule");
    test_assert(m.has(flecs::Module));

    auto e = flecs::entity(world)
        .add<Position>();
    test_assert(e.id() != 0);
    test_assert(e.has<Position>());
}

void Module_lookup_from_scope() {
    flecs::world world;
    flecs::import<ns::SimpleModule>(world);

    auto ns_entity = world.lookup("ns");
    test_assert(ns_entity.id() != 0);

    auto module_entity = world.lookup("ns::SimpleModule");
    test_assert(module_entity.id() != 0);

    auto position_entity = world.lookup("ns::SimpleModule::Position");
    test_assert(position_entity.id() != 0);

    auto nested_module = ns_entity.lookup("SimpleModule");
    test_assert(module_entity.id() == nested_module.id());

    auto module_position = module_entity.lookup("Position");
    test_assert(position_entity.id() == module_position.id());

    auto ns_position = ns_entity.lookup("SimpleModule::Position");
    test_assert(position_entity.id() == ns_position.id());    
}

void Module_nested_module() {
    flecs::world world;
    flecs::import<ns::SimpleModule>(world);

    auto velocity = world.lookup("ns::NestedModule::Velocity");
    test_assert(velocity.id() != 0);

    test_str(velocity.path().c_str(), "::ns::NestedModule::Velocity");
}

void Module_nested_type_module() {
    flecs::world world;
    world.import<ns::NestedTypeModule>();

    auto ns_entity = world.lookup("ns");
    test_assert(ns_entity.id() != 0);

    auto module_entity = world.lookup("ns::NestedTypeModule");
    test_assert(module_entity.id() != 0);

    auto type_entity = world.lookup("ns::NestedTypeModule::NestedType");
    test_assert(type_entity.id() != 0);

    auto ns_type_entity = world.lookup("ns::NestedTypeModule::NestedNameSpaceType");
    test_assert(ns_type_entity.id() != 0);

    int32_t childof_count = 0;
    type_entity.each(flecs::ChildOf, [&](flecs::entity) {
        childof_count ++;
    });

    test_int(childof_count, 1);

    childof_count = 0;
    ns_type_entity.each(flecs::ChildOf, [&](flecs::entity) {
        childof_count ++;
    });

    test_int(childof_count, 1);    
}

void Module_module_type_w_explicit_name() {
    flecs::world world;
    world.import<ns::ModuleWithImport>();

    auto ns_entity = world.lookup("ns");
    test_assert(ns_entity.id() != 0);

    auto module_1 = world.lookup("ns::name_1");
    test_assert(module_1.id() != 0);

    auto module_2 = world.lookup("ns::name_2");
    test_assert(module_2.id() != 0);

    auto comp = world.lookup("ns::name_1::Position");
    test_assert(comp.id() != 0);
}

void Module_component_redefinition_outside_module() {
    flecs::world world;

    world.import<ns::SimpleModule>();

    auto pos_comp = world.lookup("ns::SimpleModule::Position");
    test_assert(pos_comp.id() != 0);

    auto pos = world.component<Position>();
    test_assert(pos.id() != 0);
    test_assert(pos.id() == pos_comp.id());

    int32_t childof_count = 0;
    pos_comp.each(flecs::ChildOf, [&](flecs::entity) {
        childof_count ++;
    });

    test_int(childof_count, 1);
}

void Module_module_tag_on_namespace() {
    flecs::world world;

    auto mid = world.import<ns::NestedModule>();
    test_assert(mid.has(flecs::Module));

    auto nsid = world.lookup("ns");
    test_assert(nsid.has(flecs::Module));
}

static int module_ctor_invoked = 0;
static int module_dtor_invoked = 0;

class Module_w_dtor {
public:
    Module_w_dtor(flecs::world& world) {
        world.module<Module_w_dtor>();
        module_ctor_invoked ++;
    }

    ~Module_w_dtor() {
        module_dtor_invoked ++;
    }    
};

void Module_dtor_on_fini() {
    {
        flecs::world ecs;

        test_int(module_ctor_invoked, 0);
        test_int(module_dtor_invoked, 0);

        ecs.import<Module_w_dtor>();
        
        test_int(module_ctor_invoked, 1);
        test_int(module_dtor_invoked, 0);
    }

    test_int(module_dtor_invoked, 1);
}
