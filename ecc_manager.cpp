#include "config.h"

#include "ecc_manager.hpp"

#include <phosphor-logging/elog-errors.hpp>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifdef ECC_PHOSPHOR_LOGGING
#include <xyz/openbmc_project/Memory/MemoryECC/error.hpp>
#endif

using namespace phosphor::logging;

namespace phosphor
{
namespace memory
{
static constexpr const char ECC_FILE[] = "/etc/ecc/maxlog.conf";
static constexpr const auto RESET_COUNT = "1";
static constexpr const char CLOSE_EDAC_REPORT[] = "off";

auto retries = 3;
static constexpr auto delay = std::chrono::milliseconds{100};
static constexpr auto interval = std::chrono::seconds{1};
#ifdef ECC_PHOSPHOR_LOGGING
static constexpr auto ceInterval = std::chrono::hours ceInterval{1};
#endif
static constexpr uint16_t selBMCGenID = 0x0020;
void ECC::init()
{
    namespace fs = std::filesystem;

    if (fs::exists(sysfsRootPath))
    {
        try
        {
            resetCounter();
            getMaxLogValue();
        }
        catch (const std::system_error& e)
        {
            log<level::INFO>(
                "Logging failing sysfs file",
                phosphor::logging::entry("FILE=%s", sysfsRootPath));
        }
    }
    _bus.request_name(BUSNAME);
}

std::string ECC::getValue(std::string fullPath)
{
    std::string val;
    std::ifstream ifs;

    while (true)
    {
        try
        {
            if (!ifs.is_open())
                ifs.open(fullPath);
            ifs.clear();
            ifs.seekg(0);
            ifs >> val;
        }
        catch (const std::exception& e)
        {
            --retries;
            std::this_thread::sleep_for(delay);
            continue;
        }
        break;
    }

    ifs.close();
    return val;
}

void ECC::writeValue(std::string fullPath, std::string value)
{
    std::ofstream ofs;
    while (true)
    {
        try
        {
            if (!ofs.is_open())
                ofs.open(fullPath);
            ofs.clear();
            ofs.seekp(0);
            ofs << value;
            ofs.flush();
        }
        catch (const std::exception& e)
        {
            --retries;
            std::this_thread::sleep_for(delay);
            continue;
        }
        break;
    }
    ofs.close();
}

void ECC::run()
{
    init();
    std::function<void()> callback(std::bind(&ECC::read, this));
    try
    {
        _timer.restart(interval);

        _bus.attach_event(_event.get(), SD_EVENT_PRIORITY_IMPORTANT);
        _event.loop();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Error in sysfs polling loop",
                        entry("ERROR=%s", e.what()));
        throw;
    }
}

void ECC::checkEccLogFull(int64_t ceCount, int64_t ueCount)
{
    std::string errorMsg = "ECC error(memory error logging limit reached)";
    std::vector<uint8_t> eccLogFullEventData{0x05, 0xff, 0xfe};
    auto total = ceCount + ueCount;

    if (total == 0)
    {
        // someone reset edac report from driver, so clear all parameter
        EccInterface::ceCount(ceCount);
        EccInterface::ueCount(ueCount);
        previousCeCounter = 0;
        previousUeCounter = 0;
        EccInterface::isLoggingLimitReached(false);
#ifdef ECC_PHOSPHOR_LOGGING
        startCeCount = 0;
        maxCeLimitReached = false;
#endif
        return;
    }

    if (total >= maxECCLog)
    {
#ifdef ECC_PHOSPHOR_LOGGING
        if (((previousCeCounter - startCeCount) >= maxECCLog) &&
            !maxCeLimitReached)
        {
            using error = sdbusplus::xyz::openbmc_project::Memory::MemoryECC::
                Error::isLoggingLimitReached;
            report<error>();

            maxCeLimitReached = true;
            maxCeLimitReachedTime = std::chrono::system_clock::now();
        }
#else
        // add SEL log
        addSELLog(errorMsg, OBJPATH, eccLogFullEventData, true, selBMCGenID);
#endif
        // set ECC state
        EccInterface::isLoggingLimitReached(true);
        controlEDACReport(CLOSE_EDAC_REPORT);
        EccInterface::state(MemoryECC::ECCStatus::LogFull);
    }
}

int ECC::checkCeCount()
{
    std::string item = "ce_count";
    std::string errorMsg = "ECC error(correctable)";
    int64_t value = 0;
    std::string fullPath = sysfsRootPath;
    fullPath.append(item);
    value = std::stoi(getValue(fullPath));
    std::vector<uint8_t> eccCeEventData{0x00, 0xff, 0xfe};

#ifdef ECC_PHOSPHOR_LOGGING
    auto currentTime = std::chrono::system_clock::now();

    // Start logging CE after user defined elaspsed time
    if (maxCeLimitReached &&
        (currentTime - maxCeLimitReachedTime >= ceInterval))
    {
        if (value)
            startCeCount = previousCeCounter = value;
        else
            startCeCount = previousCeCounter = 0;
        maxCeLimitReached = false;
    }
#endif
    for (int64_t i = previousCeCounter + 1; i <= value; i++)
    {
        previousCeCounter = i;
        EccInterface::ceCount(i);
#ifdef ECC_PHOSPHOR_LOGGING
        if ((i - startCeCount) < maxECCLog)
        {
            using warning = sdbusplus::xyz::openbmc_project::Memory::MemoryECC::
                Error::ceCount;
            report<warning>();
        }
#else
        // add SEL log
        addSELLog(errorMsg, OBJPATH, eccCeEventData, true, selBMCGenID);
#endif
        // set ECC state
        EccInterface::state(MemoryECC::ECCStatus::CE);
    }
    return value;
}

int ECC::checkUeCount()
{
    std::string item = "ue_count";
    std::string errorMsg = "ECC error(uncorrectable)";
    int64_t value = 0;
    std::string fullPath = sysfsRootPath;
    fullPath.append(item);
    value = std::stoi(getValue(fullPath));
    std::vector<uint8_t> eccUeEventData{0x01, 0xff, 0xfe};

    while (previousUeCounter < value)
    {
        previousUeCounter++;
        // add phosphor-logging log
        EccInterface::ueCount(previousUeCounter);
#ifdef ECC_PHOSPHOR_LOGGING
        if (previousUeCounter == 1)
        {
            using error = sdbusplus::xyz::openbmc_project::Memory::MemoryECC::
                Error::ueCount;
            report<error>();
        }
#else
        // add SEL log
        addSELLog(errorMsg, OBJPATH, eccUeEventData, true, selBMCGenID);
#endif
        // set ECC state
        EccInterface::state(MemoryECC::ECCStatus::UE);
    }
    return value;
}

void ECC::resetCounter()
{
    std::string item = "reset_counters";
    std::string fullPath = sysfsRootPath;
    fullPath.append(item);
    writeValue(fullPath, RESET_COUNT);
}

void ECC::read()
{
    int64_t ceCount = 0;
    int64_t ueCount = 0;
    ceCount = checkCeCount();
    ueCount = checkUeCount();
    checkEccLogFull(ceCount, ueCount);
}

void ECC::controlEDACReport(std::string op)
{
    writeValue(sysfsEDACReportPath, op);
}

// get max log from file
void ECC::getMaxLogValue()
{
    maxECCLog = std::stoi(getValue(ECC_FILE));
}

void ECC::addSELLog(std::string message, std::string path,
                    std::vector<uint8_t> selData, bool assert, uint16_t genId)
{
    // sdbusplus::bus_t bus = sdbusplus::bus::new_default();

    auto selCall = _bus.new_method_call(
        "xyz.openbmc_project.Logging.IPMI", "/xyz/openbmc_project/Logging/IPMI",
        "xyz.openbmc_project.Logging.IPMI", "IpmiSelAdd");
    selCall.append(message, path, selData, assert, genId);

    auto selReply = _bus.call(selCall);
    if (selReply.is_method_error())
    {
        log<level::ERR>("add SEL log error\n");
    }
}

} // namespace memory
} // namespace phosphor
