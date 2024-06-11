#include <iostream>
#include <ctime>
#include <codecvt>
#include <unistd.h>
#include <csignal>
#include <QJsonDocument>
#include <QJsonObject>

#include "Insta360.h"

Insta360::Insta360(InstrDataReader &rInstrDataReader, IProgressListener &rProgressListener):
        CameraBase("insta360", ".insv", rInstrDataReader, rProgressListener)
{
}


#define READ 0
#define WRITE 1

static pid_t
popen2(const char *command, int *infp, int *outfp)
{
    int p_stdin[2], p_stdout[2];
    pid_t pid;

    if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0)
        return -1;

    pid = fork();

    if (pid < 0)
        return pid;
    else if (pid == 0)
    {
        close(p_stdin[WRITE]);
        dup2(p_stdin[READ], READ);
        close(p_stdout[READ]);
        dup2(p_stdout[WRITE], WRITE);

        execl("/bin/sh", "sh", "-c", command, NULL);
        perror("execl");
        exit(1);
    }

    if (infp == nullptr)
        close(p_stdin[WRITE]);
    else
        *infp = p_stdin[WRITE];

    if (outfp == nullptr)
        close(p_stdout[READ]);
    else
        *outfp = p_stdout[READ];

    return pid;
}

std::tuple<int, int> Insta360::readClipFile(const std::string &clipFileName, std::list<InstrumentInput> &listInputs) {
    std::string telemetryParserArgs("/Users/sergei/github/telemetry-parser/bin/gyro2bb/target/release/gyro2bb -d ");
    telemetryParserArgs += clipFileName;


    int outfp;
    int pid = popen2((const char *)telemetryParserArgs.c_str(), nullptr, &outfp);
    if (pid == -1) {
        throw std::runtime_error("popen2() failed!");
    }
    std::cout << "gyro2bb started with PID " <<  pid << std::endl;

    FILE *outStream = fdopen(outfp, "r");

    std::string out;
    try {
        std::array<char, 64*1024> buffer{};

        int lineCount = 0;
        while ( fgets(buffer.data(), sizeof(buffer), outStream) != nullptr ) {
            out = std::string(buffer.data());
            if ( lineCount  == 1 ){
                kill(pid, SIGABRT);
                int status;
                waitpid(pid, &status, WNOHANG);
                if (WIFEXITED(status)){
                    std::cout << "gyro2bb with PID " <<  pid << " stopped." << std::endl;
                }
                break;
            }
            lineCount ++;
        }
    } catch (...) {
        fclose(outStream);
        throw;
    }

    int64_t createTimeUtcMs = -1;
    int totalTimeSec = -1;
    int width=-1;
    int height=-1;
    if ( out.find("Metadata") !=std::string::npos ){
        std::size_t jsonStart = out.find('{');
        std::string jsonString = out.substr(jsonStart);
        std::cout << "gyro2bbPID " <<  pid << std::endl;
        QJsonDocument loadDoc(QJsonDocument::fromJson(QByteArray::fromStdString(jsonString)));
        QJsonObject json = loadDoc.object();
        if(  QJsonValue v = json["first_gps_timestamp"]; v.isDouble()){
            createTimeUtcMs = int64_t(v.toDouble());
        }
        if(  QJsonValue v = json["total_time"]; v.isDouble()){
            totalTimeSec = v.toInt();
        }


        auto dimension = json.value("dimension");
        width = dimension["x"].toInt();
        height = dimension["y"].toInt();

        m_ulClipStartUtcMs = createTimeUtcMs;
        m_ulClipEndUtcMs = m_ulClipStartUtcMs + totalTimeSec * 1000;
    }

    return {width,height};
}
