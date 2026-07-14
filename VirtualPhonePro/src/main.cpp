/**
 * @file main.cpp
 * @brief VirtualPhonePro main entry point
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "ReDroidController.hpp"
#include "DeviceProfile.hpp"

using namespace VirtualPhonePro;

void printBanner() {
    qDebug() << "";
    qDebug() << "==============================================";
    qDebug() << "  VirtualPhonePro v1.0.0";
    qDebug() << "  Anti-Detect Android Emulator Manager";
    qDebug() << "==============================================";
    qDebug() << "";
}

void setupLogging() {
    // Setup logging to file
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDir);
    
    QString logFile = logDir + "/virtualphonepro.log";
    
    // Redirect debug output to file
    QFile* logFileHandle = new QFile(logFile);
    if (logFileHandle->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(logFileHandle);
        stream << "\n=== VirtualPhonePro Started at " 
               << QDateTime::currentDateTime().toString(Qt::ISODate) 
               << " ===\n";
        stream.flush();
    }
}

int main(int argc, char *argv[])
{
    // Enable high DPI scaling
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    QCoreApplication app(argc, argv);
    
    // Set application info
    QCoreApplication::setApplicationName("VirtualPhonePro");
    QCoreApplication::setApplicationVersion("1.0.0");
    QCoreApplication::setOrganizationName("VirtualPhonePro");
    QCoreApplication::setOrganizationDomain("virtualphonepro.com");
    
    // Print banner
    printBanner();
    
    // Setup logging
    setupLogging();
    
    // Command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "VirtualPhonePro - Anti-Detect Android Emulator Manager\n"
        "Manage ReDroid containers with advanced anti-detection features."
    );
    
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Command options
    QCommandLineOption listOption(
        {"l", "list"},
        "List all running instances"
    );
    parser.addOption(listOption);
    
    QCommandLineOption startOption(
        {"s", "start"},
        "Start a new instance",
        "profile",
        "anti-detect"
    );
    parser.addOption(startOption);
    
    QCommandLineOption stopOption(
        {"S", "stop"},
        "Stop a running instance",
        "instance-id"
    );
    parser.addOption(stopOption);
    
    QCommandLineOption removeOption(
        {"r", "remove"},
        "Remove an instance completely",
        "instance-id"
    );
    parser.addOption(removeOption);
    
    QCommandLineOption statusOption(
        {"t", "status"},
        "Show status of an instance",
        "instance-id"
    );
    parser.addOption(statusOption);
    
    QCommandLineOption gpuOption(
        {"g", "gpu"},
        "Set GPU mode (host, guest, auto)",
        "mode",
        "host"
    );
    parser.addOption(gpuOption);
    
    // Parse command line
    parser.process(app);
    
    // Create controller
    ReDroidController controller;
    
    // Check prerequisites
    if (!controller.isDockerAvailable()) {
        qCritical() << "ERROR: Docker is not available. Please ensure Docker Desktop is running.";
        return 1;
    }
    
    if (!controller.isWslAvailable()) {
        qCritical() << "ERROR: WSL2 is not available. Please install WSL2.";
        return 1;
    }
    
    qDebug() << "Docker: Available";
    qDebug() << "WSL2: Available";
    
    // Handle commands
    if (parser.isSet(listOption)) {
        QStringList running = controller.getRunningInstances();
        qDebug() << "Running instances:";
        if (running.isEmpty()) {
            qDebug() << "  (none)";
        } else {
            for (const QString& id : running) {
                ContainerInfo info = controller.getContainerInfo(id);
                qDebug() << "  " << id 
                         << " | Port:" << info.adbPort
                         << " | Model:" << info.profile.model;
            }
        }
        return 0;
    }
    
    if (parser.isSet(startOption)) {
        QString profileName = parser.value(startOption);
        QString instanceId = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
        
        qDebug() << "Starting instance:" << instanceId;
        qDebug() << "Profile:" << profileName;
        
        DeviceProfile profile;
        
        if (profileName == "banking") {
            profile = DeviceProfile::createBankingProfile();
        } else if (profileName == "security") {
            profile = DeviceProfile::createSecurityProfile();
        } else if (profileName == "social") {
            profile = DeviceProfile::createSocialMediaProfile();
        } else if (profileName == "gaming") {
            profile = DeviceProfile::createGamingProfile();
        } else {
            profile = DeviceProfile(); // Default anti-detect
        }
        
        if (controller.startInstance(instanceId, profile)) {
            qDebug() << "Instance started successfully!";
            qDebug() << "Instance ID:" << instanceId;
            qDebug() << "Connect with: adb connect localhost:" 
                     << controller.getContainerInfo(instanceId).adbPort;
        } else {
            qCritical() << "Failed to start instance";
            return 1;
        }
        
        return 0;
    }
    
    if (parser.isSet(stopOption)) {
        QString instanceId = parser.value(stopOption);
        qDebug() << "Stopping instance:" << instanceId;
        
        if (controller.stopInstance(instanceId)) {
            qDebug() << "Instance stopped successfully";
        } else {
            qCritical() << "Failed to stop instance";
            return 1;
        }
        return 0;
    }
    
    if (parser.isSet(removeOption)) {
        QString instanceId = parser.value(removeOption);
        qDebug() << "Removing instance:" << instanceId;
        
        if (controller.removeInstance(instanceId)) {
            qDebug() << "Instance removed successfully";
        } else {
            qCritical() << "Failed to remove instance";
            return 1;
        }
        return 0;
    }
    
    if (parser.isSet(statusOption)) {
        QString instanceId = parser.value(statusOption);
        ContainerInfo info = controller.getContainerInfo(instanceId);
        
        qDebug() << "Instance:" << instanceId;
        qDebug() << "Status:" << (int)info.status;
        qDebug() << "ADB Port:" << info.adbPort;
        qDebug() << "Model:" << info.profile.model;
        qDebug() << "Brand:" << info.profile.brand;
        qDebug() << "Serial:" << info.profile.identity.serialNumber;
        qDebug() << "Android ID:" << info.profile.identity.androidId;
        return 0;
    }
    
    // No command specified, show help
    parser.showHelp();
    return 0;
}
