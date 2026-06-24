#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <list>
#include <vector>
#include <regex>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/socket.h>

#define IFNAMSIZ 16

using namespace std;
#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
class Command {
    // TODO: Add your data members
public:
    int m_argsNum;
    char** m_args;
    char* m_cmdLine;
    char* m_userGiven;
    Command(const char *cmd_line,const char* userGiven);

    virtual ~Command();

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line,const char* userGiven);

    virtual ~BuiltInCommand() {
    }
};

class ChPromptCommand : public BuiltInCommand {
    public:
    ChPromptCommand(const char *cmd_line,const char* userGiven);
    virtual ~ChPromptCommand() {}
    void execute() override;
};

class pwdcommand : public BuiltInCommand {
    public:
    pwdcommand(const char *cmd_line,const char* userGiven);
    virtual ~pwdcommand() {}
    void execute() override;
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line,const char* userGiven): Command(cmd_line,userGiven){};

    virtual ~ExternalCommand() {
    }

    void execute() override;
};


class PipeCommand : public Command {
public:
    PipeCommand(const char *cmd_line,const char* userGiven): Command(cmd_line,userGiven){};

    virtual ~PipeCommand() {
    }

    void execute() override;
};

class RedirectionCommand : public Command {
public:
    string m_commandline;
    explicit RedirectionCommand(const char *cmd_line,const char* userGiven);

    virtual ~RedirectionCommand() {
    }
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
    static std::string m_PrevPwd;
    ChangeDirCommand(const char *cmd_line,const char* userGiven);

    virtual ~ChangeDirCommand() {
    }

    void execute() override;
};


class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line,const char* userGiven);

    virtual ~ShowPidCommand() {
    }

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
public:
    QuitCommand(const char *cmd_line,const char* userGiven): BuiltInCommand(cmd_line,userGiven){};

    virtual ~QuitCommand() {
    }

    void execute() override;
};


class JobsList {
public:
    class JobEntry {
    public:
        int m_jobID;
        pid_t m_PID;
        string m_command;
        bool m_isStopped;
        JobEntry(int jobID,string command,pid_t PID,bool isStopped):m_jobID(jobID),m_PID(PID),m_command(command),
        m_isStopped(isStopped){};
    };

public:
    vector<JobEntry*> m_backgroundJobs;
    JobsList() = default;

    ~JobsList() = default;

    void addJob(string command,pid_t PID,bool isStopped = false);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);

    ///ADDED BY BASEL
    void removeJob(int jobID);

};


class JobsCommand : public BuiltInCommand {
public:
    JobsCommand(const char *cmd_line,const char* userGiven): BuiltInCommand(cmd_line,userGiven){};

    virtual ~JobsCommand() {
    }

    void execute() override;
};

class KillCommand : public BuiltInCommand {
public:
    KillCommand(const char *cmd_line,const char* userGiven): BuiltInCommand(cmd_line,userGiven){};

    virtual ~KillCommand() {
    }

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
public:
    ForegroundCommand(const char *cmd_line,const char* userGiven): BuiltInCommand(cmd_line,userGiven){};

    virtual ~ForegroundCommand() {
    }

    void execute() override;
};



class WhoAmICommand : public Command {
public:
    WhoAmICommand(const char *cmd_line,const char* userGiven): Command(cmd_line,userGiven){};

    virtual ~WhoAmICommand() {
    }

    void execute() override;
};
struct linux_dirent {
    ino_t        d_ino;    // 64-bit inode number
    off_t        d_off;    // 64-bit offset to the next dirent
    unsigned short d_reclen; // Length of this dirent

    char           d_name[]; // Filename (null-terminated)
};
class ListDirCommand : public Command {
public:
    ListDirCommand(const char *cmd_line,const char* userGiven);

    virtual ~ListDirCommand() {
    }

    void execute() override;
    void recursivePrint(char *path,int numoftabs);
};


class NetInfo : public Command {
public:
    NetInfo(const char *cmd_line,const char* userGiven): Command(cmd_line,userGiven){};

    virtual ~NetInfo() {
    }

    void execute() override;
};
struct organizedAlias {
    string m_alias;
    string m_shortcut;
    string m_command;
};
class aliasCommand : public BuiltInCommand {
public:
    static std::list<organizedAlias> m_aliases;
    std::string m_commandline;
    aliasCommand(const char *cmd_line,const char* userGiven);

    virtual ~aliasCommand() {
    }

    void execute() override;
};

class unaliasCommand : public BuiltInCommand {
public:
    std::string m_command;
    unaliasCommand(const char *cmd_line,const char* userGiven);

    virtual ~unaliasCommand() {
    }

    void execute() override;
};


class SmallShell {
private:
    SmallShell();

public:
    int m_Pid;
    JobsList m_jobsList;
    string m_currPrompt;
    Command *CreateCommand(const char *cmd_line);
    pid_t m_foregroundPID;

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell() = default;

    void executeCommand(const char *cmd_line);

};

///For Netinfo Command

struct ifreq {
    char ifr_name[IFNAMSIZ];     // Interface name (e.g., "eth0")
    struct sockaddr ifr_addr;    // Address of the interface
    struct sockaddr ifr_netmask; // Subnet mask
};
#endif //SMASH_COMMAND_H_
