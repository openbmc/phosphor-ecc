#include "config.h"

#include "ecc_manager.hpp"

#include <iostream>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/server/manager.hpp>

int main(void)
{

    /** @brief Dbus constructs */
    auto bus = sdbusplus::bus::new_default();

    // /** @brief sd_bus object manager */
    sdbusplus::server::manager::manager objManager(bus, OBJROOT);

    phosphor::memory::ECC obj(bus, OBJPATH);

    obj.run();

    return 0;
}
