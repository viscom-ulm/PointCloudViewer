/**
 * @file   main.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.24
 *
 * @brief  Program entry point.
 */

#include "core/main.h"
#include <fstream>
#include <iostream>
#include <g3log/logworker.hpp>
#include <g3log/loglevels.hpp>
#include "core/g3log/filesink.h"
#include "core/FrameworkInternal.h"
#include "core/app_internal/ApplicationNodeInternal.h"
#include "core/initialize.h"
#include "app/CoordinatorNode.h"
#include "app/WorkerNode.h"


int main(int argc, char** argv)
{
    const std::string directory = "./";
    const std::string name = "viscomlabfw.log";
    auto worker = g3::LogWorker::createLogWorker();
    auto handle = worker->addSink(std::make_unique<viscom::FileSink>(name, directory, false), &viscom::FileSink::fileWrite);

    if constexpr (viscom::DEBUG_MODE) g3::log_levels::enable(WARNING);
    else g3::log_levels::disable(WARNING);

    initializeLogging(worker.get());

    LOG(INFO) << "Log created.";

    viscom::FWConfiguration config;
    if (argc > 1) config = viscom::LoadConfiguration(argv[1]);
    else config = viscom::LoadConfiguration("framework.cfg");
    config.resourceSearchPaths_.emplace_back(config.baseDirectory_ + "extern/fwenh/resources/");

    auto appNode = Application_Init(config, [](viscom::ApplicationNodeInternal* node) { return std::make_unique<viscom::CoordinatorNode>(node); },
        [](viscom::ApplicationNodeInternal* node) { return std::make_unique<viscom::WorkerNode>(node); });

    // Main loop
    LOG(INFO) << "Started Rendering.";
    appNode->Render();

    return 0;
}
