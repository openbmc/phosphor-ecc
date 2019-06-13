#include "config.h"

#include "ecc_manager.hpp"

#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <phosphor-logging/elog-errors.hpp>
#include <string>

using namespace phosphor::logging;

namespace phosphor
{
namespace memory
{
#define ECC_FILE "/etc/ecc/maxlog.conf"
#define RESET_COUNT "1"
#define CLOSE_EDAC_REPORT "off"

auto retries = 3;
static constexpr auto delay = std::chrono::milliseconds{100};
static constexpr auto interval = 1000000;
static constexpr uint16_t selBMCGenID = 0x0020;
void ECC::init()
{

    namespace fs = std::experimental::filesystem;

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
        _timer.restart(std::chrono::microseconds(interval));

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

void ECC::checkEccLogFull()
{
    std::string ce = "ce_count";
    std::string ue = "ue_count";
    std::string errorMsg = "ECC error(memory error logging limit readched)";
    int64_t ceCount = 0;
    int64_t ueCount = 0;
    std::string ceFullPath = sysfsRootPath;
    std::string ueFullPath = sysfsRootPath;
    ceFullPath.append(ce);
    ueFullPath.append(ue);
    std::vector<uint8_t> eventData(3, 0xFF);
    bool assert = true;
    eventData[0] = 0x05;
    eventData[2] = 0xfe;
    // get ce and ue values
    ceCount = std::stoi(getValue(ceFullPath));
    ueCount = std::stoi(getValue(ueFullPath));

    auto total = ceCount + ueCount;
    bool isReached = false;
    if (total == 0)
    {
        // someone reset edac report from driver
        // so clear all parameter
        EccInterface::ceCount(ceCount);
        EccInterface::ueCount(ueCount);
        previousCeCounter = 0;
        previousUeCounter = 0;
        EccInterface::isLoggingLimitReached(isReached);
    }
    else if (total >= maxECCLog)
    {
        // add SEl log
        addSELLog(errorMsg, OBJPATH, eventData, assert, selBMCGenID);
        isReached = true;
        EccInterface::isLoggingLimitReached(isReached);
        controlEDACReport(CLOSE_EDAC_REPORT);
        // set ECC state
        EccInterface::state(MemoryECC::ECCStatus::LogFull);
    }
}

void ECC::checkCe_count()
{
    std::string item = "ce_count";
    std::string errorMsg = "ECC error(correctable)";
    int64_t value = 0;
    std::string fullPath = sysfsRootPath;
    fullPath.append(item);
    value = std::stoi(getValue(fullPath));
    std::vector<uint8_t> eventData(3, 0xFF);
    bool assert = true;
    eventData[0] = 0x00;
    eventData[2] = 0xfe;

    while (previousCeCounter < value)
    {
        previousCeCounter++;
        // add phosphor-logging log
        EccInterface::ceCount(previousCeCounter);
        // add SEl log
        addSELLog(errorMsg, OBJPATH, eventData, assert, selBMCGenID);
        // set ECC state
        EccInterface::state(MemoryECC::ECCStatus::CE);
    }
}

void ECC::checkUe_count()
{
    std::string item = "ue_count";
    std::string errorMsg = "ECC error(uncorrectable)";
    int64_t value = 0;
    std::string fullPath = sysfsRootPath;
    fullPath.append(item);
    value = std::stoi(getValue(fullPath));
    std::vector<uint8_t> eventData(3, 0xFF);
    bool assert = true;
    eventData[0] = 0x01;
    eventData[2] = 0xfe;

    while (previousUeCounter < value)
    {
        previousUeCounter++;
        // add phosphor-logging log
        EccInterface::ueCount(previousUeCounter);
        // add SEl log
        addSELLog(errorMsg, OBJPATH, eventData, assert, selBMCGenID);
        // set ECC state
        EccInterface::state(MemoryECC::ECCStatus::UE);
    }
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
    checkCe_count();
    checkUe_count();
    checkEccLogFull();
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
    // sdbusplus::bus::bus bus = sdbusplus::bus::new_default();

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
