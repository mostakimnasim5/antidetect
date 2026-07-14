/**
 * @file Logger.cpp
 * @brief Logger implementation
 */

#include "Logger.hpp"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMutexLocker>

namespace VirtualPhonePro {

// Static instance
Logger* Logger::s_instance = nullptr;
QMutex Logger::s_mutex;

// ============================================================================
// Constructor / Destructor
// ============================================================================

Logger::Logger(QObject* parent)
    : QObject(parent)
    , m_minLevel(LogLevel::Debug)
    , m_logToConsole(true)
    , m_logToFile(true)
{
    s_instance = this;
    
    // Setup default log directory
    m_logDirectory = QDir::homePath() + "/.virtualphonepro/logs";
    QDir().mkpath(m_logDirectory);
    
    // Open log file
    openLogFile();
}

Logger::~Logger() {
    flush();
    closeLogFile();
    s_instance = nullptr;
}

// ============================================================================
// Singleton Access
// ============================================================================

Logger* Logger::instance() {
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new Logger();
    }
    return s_instance;
}

void Logger::shutdown() {
    QMutexLocker locker(&s_mutex);
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

// ============================================================================
// Configuration
// ============================================================================

void Logger::setLogLevel(LogLevel level) {
    m_minLevel = level;
}

void Logger::setLogDirectory(const QString& directory) {
    QMutexLocker locker(&m_mutex);
    m_logDirectory = directory;
    QDir().mkpath(m_logDirectory);
    closeLogFile();
    openLogFile();
}

void Logger::setLogToConsole(bool enabled) {
    m_logToConsole = enabled;
}

void Logger::setLogToFile(bool enabled) {
    m_logToFile = enabled;
}

void Logger::setMaxFileSize(qint64 maxSize) {
    m_maxFileSize = maxSize;
}

void Logger::setMaxBackupFiles(int maxFiles) {
    m_maxBackupFiles = maxFiles;
}

// ============================================================================
// Logging Methods
// ============================================================================

void Logger::log(LogLevel level, const QString& message, const QString& category) {
    if (level < m_minLevel) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = levelToString(level);
    QString categoryStr = category.isEmpty() ? "" : QString("[%1] ").arg(category);
    
    QString fullMessage = QString("[%1] [%2] %3%4")
        .arg(timestamp)
        .arg(levelStr)
        .arg(categoryStr)
        .arg(message);
    
    // Console output
    if (m_logToConsole) {
        printToConsole(level, fullMessage);
    }
    
    // File output
    if (m_logToFile && m_logFile.isOpen()) {
        QTextStream stream(&m_logFile);
        stream << fullMessage << "\n";
        stream.flush();
        
        // Check file size
        if (m_logFile.size() > m_maxFileSize) {
            rotateLogFile();
        }
    }
    
    // Emit signal
    emit logMessage(level, message, category);
}

void Logger::debug(const QString& message, const QString& category) {
    log(LogLevel::Debug, message, category);
}

void Logger::info(const QString& message, const QString& category) {
    log(LogLevel::Info, message, category);
}

void Logger::warning(const QString& message, const QString& category) {
    log(LogLevel::Warning, message, category);
}

void Logger::error(const QString& message, const QString& category) {
    log(LogLevel::Error, message, category);
}

void Logger::critical(const QString& message, const QString& category) {
    log(LogLevel::Critical, message, category);
}

// ============================================================================
// File Operations
// ============================================================================

void Logger::flush() {
    QMutexLocker locker(&m_mutex);
    if (m_logFile.isOpen()) {
        m_logFile.flush();
    }
}

void Logger::openLogFile() {
    QString fileName = QString("virtualphonepro_%1.log")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd"));
    
    QString filePath = m_logDirectory + "/" + fileName;
    
    m_logFile.setFileName(filePath);
    
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to open log file:" << filePath;
        return;
    }
}

void Logger::closeLogFile() {
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

void Logger::rotateLogFile() {
    closeLogFile();
    
    // Remove oldest backup if exceeding max
    QString baseName = m_logFile.fileName();
    QString latestBackup = baseName + ".1";
    
    QFile::remove(latestBackup);
    
    // Rename current to backup
    QFile::rename(baseName, latestBackup);
    
    // Open new file
    openLogFile();
}

// ============================================================================
// Utility Methods
// ============================================================================

QString Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO ";
        case LogLevel::Warning:  return "WARN ";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRIT ";
        default:                 return "UNKN ";
    }
}

void Logger::printToConsole(LogLevel level, const QString& message) {
    QByteArray colored;
    
    switch (level) {
        case LogLevel::Debug:
            colored = message.toUtf8();
            break;
        case LogLevel::Info:
            colored = message.toUtf8();
            break;
        case LogLevel::Warning:
            colored = message.toUtf8();
            break;
        case LogLevel::Error:
            colored = message.toUtf8();
            break;
        case LogLevel::Critical:
            colored = message.toUtf8();
            break;
        default:
            colored = message.toUtf8();
    }
    
    printf("%s\n", colored.constData());
    fflush(stdout);
}

// ============================================================================
// Global Logging Functions
// ============================================================================

void logDebug(const QString& message, const QString& category) {
    Logger::instance()->debug(message, category);
}

void logInfo(const QString& message, const QString& category) {
    Logger::instance()->info(message, category);
}

void logWarning(const QString& message, const QString& category) {
    Logger::instance()->warning(message, category);
}

void logError(const QString& message, const QString& category) {
    Logger::instance()->error(message, category);
}

void logCritical(const QString& message, const QString& category) {
    Logger::instance()->critical(message, category);
}

}
