/**
 * @file Logger.hpp
 * @brief Logger header file
 */

#ifndef VIRTUALPHONEPRO_LOGGER_HPP
#define VIRTUALPHONEPRO_LOGGER_HPP

#include <QObject>
#include <QString>
#include <QFile>
#include <QMutex>
#include <QDir>

namespace VirtualPhonePro {

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
};

class Logger : public QObject {
    Q_OBJECT

public:
    static Logger* instance();
    static void shutdown();
    
    void setLogLevel(LogLevel level);
    void setLogDirectory(const QString& directory);
    void setLogToConsole(bool enabled);
    void setLogToFile(bool enabled);
    void setMaxFileSize(qint64 maxSize);
    void setMaxBackupFiles(int maxFiles);
    
    void log(LogLevel level, const QString& message, const QString& category = QString());
    
    void debug(const QString& message, const QString& category = QString());
    void info(const QString& message, const QString& category = QString());
    void warning(const QString& message, const QString& category = QString());
    void error(const QString& message, const QString& category = QString());
    void critical(const QString& message, const QString& category = QString());
    
    void flush();

signals:
    void logMessage(LogLevel level, const QString& message, const QString& category);

private:
    explicit Logger(QObject* parent = nullptr);
    ~Logger();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void openLogFile();
    void closeLogFile();
    void rotateLogFile();
    QString levelToString(LogLevel level);
    void printToConsole(LogLevel level, const QString& message);
    
    static Logger* s_instance;
    static QMutex s_mutex;
    
    LogLevel m_minLevel;
    bool m_logToConsole;
    bool m_logToFile;
    QString m_logDirectory;
    QFile m_logFile;
    QMutex m_mutex;
    qint64 m_maxFileSize{10 * 1024 * 1024}; // 10MB
    int m_maxBackupFiles{5};
};

// Global logging functions
void logDebug(const QString& message, const QString& category = QString());
void logInfo(const QString& message, const QString& category = QString());
void logWarning(const QString& message, const QString& category = QString());
void logError(const QString& message, const QString& category = QString());
void logCritical(const QString& message, const QString& category = QString());

}

#endif
