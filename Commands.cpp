#include <unistd.h>
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <algorithm>
#include "Commands.h"
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <sys/ioctl.h>


using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif
/////////////////////////////////////////////Utilities//////////////////////////////////////////////////////////////////
bool includesChar(string cmd,char ch)
{
    return count(cmd.begin(), cmd.end(), ch) > 0;
}

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}


char* findFullSubstring(char* str, const std::string& substring) {
    // Convert the C-style string to std::string for easier processing
    std::string s(str);

    size_t pos = s.find(substring);

    // If the substring is not found, return nullptr
    if (pos == std::string::npos) {
        return nullptr;
    }

    // Check that the substring is not part of a larger word
    if ((pos == 0 || !std::isalnum(s[pos - 1])) &&
        (pos + substring.length() == s.length() || !std::isalnum(s[pos + substring.length()]))) {
        return &str[pos];  // Return a char* pointing to the start of the found substring
    }

    return nullptr;  // Return nullptr if it's part of a larger word
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////Command Class Implementation/////////////////////////////////////////////////
Command::Command(const char *cmd_line,const char* userGiven){
    this->m_args=new char *[COMMAND_MAX_ARGS+1];
    this->m_argsNum=_parseCommandLine(cmd_line, this->m_args);
    m_cmdLine = new char[strlen(cmd_line) + 1];
    m_userGiven = new char[strlen(userGiven) + 1];
    strcpy(m_cmdLine, cmd_line);
    strcpy(m_userGiven, userGiven);
}

Command::~Command() {
    if(m_args!=NULL) {
        for (int i=0; i<this->m_argsNum; i++) {
            free(this->m_args[i]);
        }
    }
    free(this->m_args);
    free(m_cmdLine);
    free(m_userGiven);
}
BuiltInCommand::BuiltInCommand(const char *cmd_line,const char* userGiven):Command(cmd_line,userGiven) {
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////ChPrompt Command Implementation////////////////////////////////////////////
ChPromptCommand::ChPromptCommand(const char *cmd_line,const char* userGiven):BuiltInCommand(cmd_line,userGiven) {}
void ChPromptCommand::execute() {
    _removeBackgroundSign(m_cmdLine);
    _removeBackgroundSign(m_userGiven);
    SmallShell &shell = SmallShell::getInstance();
    ///Removing finished jobs
    shell.m_jobsList.removeFinishedJobs();
    if(this->m_argsNum==1) {
        shell.m_currPrompt = "smash> ";
        return;
    }
    shell.m_currPrompt=string(this->m_args[1]) + "> ";
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////ShowPid Command Implementation/////////////////////////////////////////////////
ShowPidCommand::ShowPidCommand(const char *cmd_line,const char* userGiven):BuiltInCommand(cmd_line,userGiven){}

void ShowPidCommand::execute() {
    _removeBackgroundSign(m_cmdLine);
    _removeBackgroundSign(m_userGiven);
    SmallShell &shell = SmallShell::getInstance();
    ///Removing finished jobs
    shell.m_jobsList.removeFinishedJobs();
    SmallShell &sm = SmallShell::getInstance();
    cout<<"smash pid is "<<sm.m_Pid<<endl;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////pwd Command Implementation/////////////////////////////////////////////////////
pwdcommand::pwdcommand(const char *cmd_line,const char* userGiven):BuiltInCommand(cmd_line,userGiven) {}
void pwdcommand::execute() {
    _removeBackgroundSign(m_cmdLine);
    _removeBackgroundSign(m_userGiven);
    SmallShell &shell = SmallShell::getInstance();
    ///Removing finished jobs
    shell.m_jobsList.removeFinishedJobs();
    char* current_dir = getcwd(NULL, 0);
    cout << current_dir << endl;
    free(current_dir);

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////ChangeDir Command Implementation///////////////////////////////////////////////
ChangeDirCommand::ChangeDirCommand(const char *cmd_line,const char* userGiven):BuiltInCommand(cmd_line,userGiven) {
}
void ChangeDirCommand::execute() {
    _removeBackgroundSign(m_cmdLine);
    _removeBackgroundSign(m_userGiven);
    SmallShell &shell = SmallShell::getInstance();
    ///Removing finished jobs
    shell.m_jobsList.removeFinishedJobs();

    if(this->m_argsNum>2) {cerr<<"smash error: cd: too many arguments"<<endl; return;}
    if(strcmp(this->m_args[1],"-") == 0) {
        if(m_PrevPwd == ""){cerr<<"smash error: cd: OLDPWD not set"<<endl; return;}
        else {
            char* current_dir = getcwd(NULL, 0);
            if(!current_dir) {
                perror("smash error: cd : getcwd");
            }

            if(chdir(m_PrevPwd.c_str())!=0) {
                perror("smash error: cd : chdir");
                return;
            }
            m_PrevPwd=current_dir;
            return;
        }
    }
    std::string dir=m_args[1];
    char* current_dir2 = getcwd(NULL, 0);
    if(!current_dir2) {
        perror("smash error: getcwd failed");
    }
    if(chdir(dir.c_str())!=0) {
        perror("smash error: chdir failed");
        return;
    }
    else {
        m_PrevPwd=current_dir2;
        return;
    }


}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////alias Command Implementation///////////////////////////////////////////////////
aliasCommand::aliasCommand(const char *cmd_line,const char* userGiven):BuiltInCommand(cmd_line,userGiven) {
    m_commandline=cmd_line;

}
void aliasCommand::execute()
{
    _removeBackgroundSign(m_cmdLine);
    _removeBackgroundSign(m_userGiven);
    SmallShell &shell = SmallShell::getInstance();
    ///Removing finished jobs
    shell.m_jobsList.removeFinishedJobs();
    if(this->m_argsNum==1) {
        for (std::list<organizedAlias>::iterator it=m_aliases.begin();it!=m_aliases.end();++it) {
            std::string s=it->m_alias.substr(6);
            std::cout << s<< endl;
        }
        return;
    }
    std::string pattern = R"(^alias [a-zA-Z0-9_]+='[^']*'$)";
    std::regex regex(pattern);

// Create a copy of the command line string
    std::string copyCom = m_commandline;

// Remove background sign
    _removeBackgroundSign(&copyCom[0]); // Assuming _removeBackgroundSign modifies the string in place

// Trim the string (assuming _trim returns a std::string)
    std::string trimmedCom = _trim(copyCom);

// Check if the trimmed string matches the regex
    if (std::regex_match(trimmedCom, regex))
    {
        organizedAlias oa;
        size_t start = trimmedCom.find("alias ") + 6; // "alias " is 6 characters
        size_t end = trimmedCom.find('=');
        oa.m_shortcut= trimmedCom.substr(start, end - start);
        start = trimmedCom.find('\'') + 1; // Start after the first single quote
        end = trimmedCom.find('\'', start); // Find the next single quote
        oa.m_command= trimmedCom.substr(start, end - start);
        oa.m_alias=trimmedCom;
        if(oa.m_shortcut=="alias"||oa.m_shortcut=="chprompt"||oa.m_shortcut=="showpid"||
            oa.m_shortcut=="pwd"||oa.m_shortcut=="cd"||
            oa.m_shortcut=="jobs"||oa.m_shortcut=="fg"||oa.m_shortcut=="kill"||
            oa.m_shortcut=="unalias"||oa.m_shortcut=="listdir"||oa.m_shortcut=="whoami"||
            oa.m_shortcut=="netinfo"||oa.m_shortcut=="chprompt") {
            cerr<<"smash error: alias: " << oa.m_shortcut<< " already exists or is a reserved command"<<endl;
            return;
        }
        for (std::list<organizedAlias>::iterator it=aliasCommand::m_aliases.begin();it!=aliasCommand::m_aliases.end();++it) {
            std::string name=it->m_shortcut;
            if(oa.m_shortcut==name) {
                cerr<<"smash error: alias: " << oa.m_shortcut<< " already exists or is a reserved command"<<endl;
                return;
            }

        }

        m_aliases.push_back(oa);
    }
    else {
        cerr<<"smash error: alias: invalid alias format"<<endl;
        return;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////unalias Command Implementation/////////////////////////////////////////////////
unaliasCommand::unaliasCommand(const char *cmd_line,const char* userGiven):BuiltInCommand(cmd_line,userGiven) {
    m_command=cmd_line;
}
void unaliasCommand::execute() {
    _removeBackgroundSign(m_cmdLine);
    _removeBackgroundSign(m_userGiven);
    SmallShell &shell = SmallShell::getInstance();
    ///Removing finished jobs
    shell.m_jobsList.removeFinishedJobs();
    if(this->m_argsNum==1) {
        cerr<<"smash error: unalias: not enough arguments"<<endl;
        return;
    }
    std::string s = string(m_userGiven).substr(8);
    size_t start = 0;
    size_t end = s.find(" ");
    bool found=false;


    while (end != std::string::npos) {
        for (std::list<organizedAlias>::iterator it=aliasCommand::m_aliases.begin();it!=aliasCommand::m_aliases.end();++it) {
            std::string name=it->m_shortcut;
            if(s.substr(start, end - start)==name) {
                aliasCommand::m_aliases.erase(it);
                found=true;
                break;
            }
        }
        if(found==false){
            cerr<<"smash error: unalias: " << s << " alias does not exist"<<endl;
            return;
        }

        start = end + 1;
        end = s.find(" ", start);
    }
    for (std::list<organizedAlias>::iterator it=aliasCommand::m_aliases.begin();it!=aliasCommand::m_aliases.end();++it) {
        std::string name=it->m_shortcut;
        if(s.substr(start)==name) {
            aliasCommand::m_aliases.erase(it);
            found=true;
            break;
        }

    }
    if(found==false) {
        cerr << "smash error: unalias: " << s << " alias does not exist" << endl;
        return;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////Redirection Command Implementation/////////////////////////////////////////////
RedirectionCommand::RedirectionCommand(const char *cmd_line,const char* userGiven): Command(cmd_line,userGiven),m_commandline(cmd_line) {}
void RedirectionCommand::execute() {
     SmallShell &shell = SmallShell::getInstance();
     shell.m_jobsList.removeFinishedJobs();
     int openparms;
    _removeBackgroundSign(m_cmdLine);
    m_commandline = string(m_cmdLine);
     string input=_trim(m_commandline.substr(0,m_commandline.find(">")));
     string output;
     if(m_commandline.find(">>")!=std::string::npos) {
         openparms=O_WRONLY| O_CREAT | O_APPEND;
         output=_trim(m_commandline.substr(m_commandline.find_first_of(">>") + 2));
     }
     else {
         openparms=O_WRONLY | O_CREAT | O_TRUNC;
         output=_trim(m_commandline.substr(m_commandline.find_first_of(">") + 1));
     }
     int standarddscrptr=dup(STDOUT_FILENO);
     close(STDOUT_FILENO);
     int dscrptr=open(output.c_str(),openparms,0666);
     if(dscrptr==-1) {
         perror("smash error: open failed");
         dup2(standarddscrptr,STDOUT_FILENO);
         close(standarddscrptr);
         return;
     }
     shell.executeCommand(input.c_str());
     close(dscrptr);
     dup2(standarddscrptr,STDOUT_FILENO);
     close(standarddscrptr);
     return;


    /*string redirect_cmd = _trim(m_commandline);
    //the same thing as the background command
    string cmd = "";
    string file = "";
    bool is_append_cmd = redirect_cmd.find(">>") != string::npos;
    //trim till u get to the first redirection sign
    cmd = _trim(redirect_cmd.substr(0 , redirect_cmd.find_first_of('>')));
    int fd;
    //save the last channel before u change it
    int stdout_fd;
    stdout_fd = dup(STDOUT_FILENO);
    close(STDOUT_FILENO);
    //if the redirection command is append instead of being override
    if (is_append_cmd)
    {
        // append command find  append sign
        file = _trim(redirect_cmd.substr(redirect_cmd.find_first_of(">>") + 2));
        //other flags may have worked as u can see the old function but we got a weird bug we couldn't fix so we setteld for this one
        fd = open(file.c_str(), O_WRONLY | O_CREAT | O_APPEND,  0644);

    } else
    {
        // for non append function we need to override we need to add only one because there is only one >
        file = _trim(redirect_cmd.substr(redirect_cmd.find_first_of(">") + 1));
        //we changed one flag which is O_APPEND to O_TRUNC so we can override the txt file
        fd = open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC,  0644);
    }
    if (fd == -1)
    {
        //if the open system call have failed we need to set back the old channel or reset it
        perror("smash error: open failed");
        dup2(stdout_fd, STDOUT_FILENO);
        close(stdout_fd);
        return;
    }
    //i don't know why making a new command and then calling execute give us  weird bug too
    SmallShell::getInstance().executeCommand(cmd.c_str());

    close(fd);
    //dup2 sets the old channel back
    dup2(stdout_fd, STDOUT_FILENO);
    close(stdout_fd);
     */
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////ListDir Command Implementation/////////////////////////////////////////////////
ListDirCommand::ListDirCommand(const char *cmd_line,const char* userGiven):Command(cmd_line,userGiven){}
void ListDirCommand::recursivePrint(char* path,int numoftabs) {
    struct stat fileStat;
    int descriptor = open(path, O_RDONLY | O_DIRECTORY);
    if (descriptor == -1) {
        perror("smash error: open failed");
        return;
    }
    char entries[4096];
    int nread;
    vector<std::string> directories;
    vector<std::string> files;
    while ((nread = syscall(SYS_getdents, descriptor, entries, sizeof(entries))) > 0) {
        int bpos = 0;
        while (bpos < nread) {
            struct linux_dirent* d = (struct linux_dirent*)(entries + bpos);
            char* name(d->d_name);

            if (strcmp(name,".")!=0 && strcmp(name,"..")!=0 ) {
                size_t l = strlen(path);
                size_t l2 = strlen(name);
                char *buffer = (char *) malloc(l + l2 + 2);
                strcpy(buffer, path);
                strcat(buffer, "/");
                strcat(buffer, name);

                // Identify if it is a directory or file (lower-level check)
                char* d_type_ptr = (char*)d + d->d_reclen - 1;  // d_type is at the end of the entry
                if (*d_type_ptr == DT_DIR) {
                    directories.push_back(std::string(name));
                }
                else
                    files.push_back(std::string(name));
            }
            bpos += d->d_reclen;
        }
    }
    if (nread == -1) {
        std::cerr << "Error reading directory: " << strerror(errno) << std::endl;
    }
    close(descriptor);
    // Sort directories and files
    sort(directories.begin(), directories.end());
    sort(files.begin(), files.end());

    // Print directories first
    for (const auto& dir : directories) {
        for (int i = 0; i < numoftabs; i++) {
            std::cout << "\t";
        }
        std::cout << dir << std::endl;

        // Construct full path and recursively print subdirectories
        std::string fullPath = std::string(path) + "/" + dir;
        recursivePrint((char*)fullPath.c_str(), numoftabs + 1);
    }

    // Print files
    for (const auto& file : files) {
        for (int i = 0; i < numoftabs; i++) {
            std::cout << "\t";
        }
        std::cout << file << std::endl;
    }
}
void ListDirCommand::execute() {

    SmallShell &shell = SmallShell::getInstance();
    shell.m_jobsList.removeFinishedJobs();
    if(this->m_argsNum>2) {
        cerr<<"smash error: listdir: too many arguments"<<endl;
        return;
    }
    char* dir;
    if(this->m_argsNum==2) {
        std::string dirname=this->m_args[1];
        dirname=dirname.substr(dirname.find_last_of("/")+1);
        std::cout <<  dirname << std::endl;
        recursivePrint(this->m_args[1],0);
    }
    else {
        char* dir=getcwd(NULL, 0);
        if(!dir) {
            perror("smash error: getcwd faild");
            return;
        }
        std::string dirname=string(dir);
        dirname=dirname.substr(dirname.find_last_of("/")+1);
        std::cout <<  dirname << std::endl;

        recursivePrint(dir,0);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////SmallShell Implementation/////////////////////////////////////////////////////////
SmallShell::SmallShell():m_foregroundPID(-1),m_Pid(getpid()),m_currPrompt("smash> ") {
}
Command *SmallShell::CreateCommand(const char *cmd_line) {
    ///Parsing...
    char** args=new char *[COMMAND_MAX_ARGS+1];
    char* copy;
    copy = new char[strlen(cmd_line) + 1];
    strcpy(copy,cmd_line);
    _parseCommandLine(copy,args);
    ///Finding Possible Aliases
    auto aliasesList = aliasCommand::m_aliases;
    for (organizedAlias organizedAlias: aliasesList) {
            ///WHO FREAKING KNOWS
            char *pos = findFullSubstring(copy, organizedAlias.m_shortcut);
            if (pos != nullptr && strcmp(args[0],"unalias") !=0 && strcmp(args[0],"alias")!=0) {
                // Calculate the lengths needed for the new string
                size_t prefixLength = pos - copy;
                size_t shortcutLength = strlen(organizedAlias.m_shortcut.c_str());
                size_t commandLength = strlen(organizedAlias.m_command.c_str());
                size_t suffixLength = strlen(pos + shortcutLength);

                // Allocate memory for the new string (copy)
                size_t newLength = prefixLength + commandLength + suffixLength + 1; // +1 for null terminator
                char *newCopy = new char[newLength];

                // Construct the new string
                strncpy(newCopy, copy, prefixLength);          // Copy the prefix
                strcpy(newCopy + prefixLength, organizedAlias.m_command.c_str()); // Copy the replacement command
                strcpy(newCopy + prefixLength + commandLength, pos + shortcutLength); // Copy the suffix

                // Free old copy and reassign to new copy
                delete[] copy;
                copy = newCopy;
            }
        }
    _parseCommandLine(copy,args);
    ///Checking for an empty line
    if(_rtrim(copy).empty())
    {
        return nullptr;
    }
    ///Alias Command
    if(strcmp("alias",args[0]) == 0)
    {
        return new aliasCommand(copy,cmd_line);
    }


    ///IO Redirection Command
    if(includesChar(string(copy),'>') || string(copy).find(">>") != string ::npos)
    {
        return new RedirectionCommand(copy,cmd_line);
    }

    ///Pipe Command
    if(string(copy).find("|&") != string::npos || string(copy).find('|') != string::npos)
    {
        return new PipeCommand(copy,cmd_line);
    }


    ///unAlias Command
    if(strcmp("unalias",args[0]) == 0)
    {
        return new unaliasCommand(copy,cmd_line);
    }

    ///Chprompt Command
    if(strcmp("chprompt",args[0]) == 0)
    {
        return new ChPromptCommand(copy,cmd_line);
    }

    ///Showpid Command
    if(strcmp("showpid",args[0]) == 0)
    {
        return new ShowPidCommand(copy,cmd_line);
    }

    ///Pwd Command
    if(strcmp("pwd",args[0]) == 0)
    {
        return new pwdcommand(copy,cmd_line);
    }

    ///Cd Command
    if(strcmp("cd",args[0]) == 0)
    {
        return new ChangeDirCommand(copy,cmd_line);
    }

    ///Jobs Command
    if(strcmp("jobs",args[0]) == 0)
    {
        return new JobsCommand(copy,cmd_line);
    }

    ///Foreground Command
    if(strcmp("fg",args[0]) == 0)
    {
        return new ForegroundCommand(copy,cmd_line);
    }

    ///Kill Command
    if(strcmp("kill",args[0]) == 0)
    {
        return new KillCommand(copy,cmd_line);
    }

    ///Quit Command
    if(strcmp("quit",args[0]) == 0)
    {
        return new QuitCommand(copy,cmd_line);
    }
    ///Whoami Command (Real)
    if(strcmp("whoami",args[0]) == 0)
    {
        return new WhoAmICommand(copy,cmd_line);
    }

    ///Listdir command
    if(strcmp("listdir",args[0]) == 0)
    {
        return new ListDirCommand(copy,cmd_line);
    }

    ///Netinfo command
    if(strcmp("netinfo",args[0]) == 0)
    {
        return new NetInfo(copy,cmd_line);
    }


    ///Not all of the above then it has to be an external command
    return new ExternalCommand(copy,cmd_line);

}
void SmallShell::executeCommand(const char *cmd_line) {
   Command* currCommand = this->CreateCommand(cmd_line);
   ///Removing finished jobs
   m_jobsList.removeFinishedJobs();
   ///Polymorphism is the way to go
   if(currCommand != nullptr)
       currCommand->execute();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////Jobs List Implementation//////////////////////////////////////////////////////////
void JobsList::printJobsList(){
    SmallShell &shell = SmallShell::getInstance();
    ///Removing finished jobs
    shell.m_jobsList.removeFinishedJobs();
    list<organizedAlias> aliasList = aliasCommand::m_aliases;
    ///Printing the desired Output
    for(JobEntry* jobEntry : shell.m_jobsList.m_backgroundJobs) {
        list<string> currAliases;
        if (jobEntry->m_isStopped) {
            continue;
        }
        cout << "[" << jobEntry->m_jobID << "] " << jobEntry->m_command << endl;
    }
}
void JobsList::removeFinishedJobs() {
    SmallShell &shell = SmallShell::getInstance();
    vector<JobEntry*> unfinishedBackgroundJobs;
    while(true)
    {
        pid_t PID = waitpid(-1, nullptr,WNOHANG);
        if(PID <= 0)
            break;
        for(int i = 0;i < m_backgroundJobs.size();i++)
        {
            if(m_backgroundJobs[i]->m_PID == PID)
            {
                m_backgroundJobs.erase(m_backgroundJobs.begin() + i);
            }
        }
    }
}
JobsList::JobEntry* JobsList::getJobById(int jobId) {
    for(JobsList::JobEntry* job : m_backgroundJobs)
    {
        if(job->m_jobID == jobId)
            return job;
    }
    return nullptr;
}
void JobsList::killAllJobs() {
    for(JobsList::JobEntry* jobEntry: m_backgroundJobs)
    {
        ///Killing the job
        if(kill(jobEntry->m_PID,SIGKILL) == -1)
        {
            ///Kill Failure
            perror("smash error: kill failed");
            exit(0);
        }
        cout << jobEntry->m_PID << ": " << jobEntry->m_command << endl;
    }
}
void JobsList::removeJob(int jobID) {
    for(int i = 0;i < m_backgroundJobs.size();i++)
    {
        if(m_backgroundJobs[i]->m_jobID == jobID)
        {
            m_backgroundJobs.erase(m_backgroundJobs.begin() + i);
        }
    }
}
void JobsList::addJob(string command, pid_t PID, bool isStopped) {
    int insertID = this->m_backgroundJobs.empty() ? 1 : this->m_backgroundJobs.back()->m_jobID + 1;
    auto* newEntry = new JobEntry(insertID,command,PID,isStopped);
    m_backgroundJobs.push_back(newEntry);
    removeFinishedJobs();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////Jobs Command Implementation////////////////////////////////////////////////////////
void JobsCommand::execute() {
    SmallShell &shell = SmallShell::getInstance();
    shell.m_jobsList.printJobsList();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////Quit Command Implementation////////////////////////////////////////////////////////
void QuitCommand::execute() {
    _removeBackgroundSign(m_cmdLine);
    _removeBackgroundSign(m_userGiven);
    SmallShell &shell = SmallShell::getInstance();
    shell.m_jobsList.removeFinishedJobs();
    ///Checking for the keyword kill
    if(m_argsNum > 1 && strcmp(m_args[1],"kill") == 0)
    {
        size_t totalJobs = shell.m_jobsList.m_backgroundJobs.size();
        cout << "smash: sending SIGKILL signal to " << totalJobs << " jobs:" << endl;
       shell.m_jobsList.killAllJobs();
    }
    exit(1);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////Kill Command Implementation//////////////////////////////////////////////////////////
void KillCommand::execute() {
    _removeBackgroundSign(m_cmdLine);
    _removeBackgroundSign(m_userGiven);
    SmallShell &shell = SmallShell::getInstance();
    shell.m_jobsList.removeFinishedJobs();
    ///Checking Syntax
    if(m_argsNum != 3 || m_args[1][0] != '-')
    {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    try
    {
        int wantedSignal = stoi(string(m_args[1]).substr(1));
        int wantedJobID = stoi(m_args[2]);
        if(wantedJobID < 0) {
            cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }
        JobsList::JobEntry* wantedJob = nullptr;
        ///Searching for the job in background section
        for(JobsList::JobEntry* jobEntry : shell.m_jobsList.m_backgroundJobs)
        {
            if(jobEntry->m_jobID == wantedJobID)
            {
                wantedJob = jobEntry;
            }
        }
        ///Checking if no job was found with that ID
        if(wantedJob == nullptr)
        {
            cerr << "smash error: kill: job-id " << wantedJobID <<  " does not exist" << endl;
            return;
        }
        ///Printing
        cout << "signal number "<< wantedSignal << " was sent to pid "<<  wantedJob->m_PID  <<endl;
        if(kill(wantedJob->m_PID,wantedSignal) == -1)
        {
            perror("smash error: kill failed");
            return;
        }

        ///Checking for SIGSTOP
        if(wantedSignal == SIGSTOP)
        {
            wantedJob->m_isStopped = true;
        }
        ///Checking for SIGCONT
        if(wantedSignal == SIGCONT)
        {
            wantedJob->m_isStopped = false;
        }
    }
    catch(exception& exception)
    {
        ///If we get here stoi failed
        cerr << "smash error: kill: invalid arguments" << endl;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////FG Command Implementation/////////////////////////////////////////////////////////////
void ForegroundCommand::execute() {
    _removeBackgroundSign(m_cmdLine);
    _removeBackgroundSign(m_userGiven);
    SmallShell &shell = SmallShell::getInstance();
    shell.m_jobsList.removeFinishedJobs();
    if (m_argsNum > 2) {
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    ///Checking for no arg and empty list
    if (m_argsNum == 1 && shell.m_jobsList.m_backgroundJobs.empty()) {
        cerr << "smash error: fg: jobs list is empty" << endl;
        return;
    }
    ///Init to be the max ID
    int wantedjobID;
    JobsList::JobEntry *wantedJob;
    try {
        if (m_argsNum == 2) {
            wantedjobID = stoi(m_args[1]);
            if(wantedjobID < 0)
            {
				cerr << "smash error: fg: invalid arguments" << endl;
				return;
			}
            wantedJob = shell.m_jobsList.getJobById(wantedjobID);
        }
        else
        ///Maybe an empty list got two arguments
            {
            wantedJob = shell.m_jobsList.m_backgroundJobs.back();
            wantedjobID = wantedJob->m_jobID;
            }
        }
    catch (...) {
        ///if we get here stoi failed
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    if (wantedJob == nullptr) {
        cerr << "smash error: fg: job-id " << wantedjobID << " does not exist" << endl;
        return;
    }
    ///If stopped we need it to continue
    if(wantedJob->m_isStopped) {
        if (kill(wantedJob->m_PID, SIGCONT) == -1) {
            perror("smash error: kill failed");
            return;
        }
    }
    ///This is actually not needed because the job will be removed from the jobs list
    wantedJob->m_isStopped = false;
    cout << wantedJob->m_command << " " << wantedJob->m_PID << endl;
    ///Making it in the foreground and running it
    shell.m_foregroundPID = wantedJob->m_PID;
    if (waitpid(wantedJob->m_PID,NULL, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
    }
    ///Removing from the background list and updating the foreground job after completing
    shell.m_jobsList.removeJob(wantedjobID);
    shell.m_foregroundPID = -1; ///No one is running
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////External Command Implementation//////////////////////////////////////////////////////
void ExternalCommand::execute()
{
    SmallShell& shell = SmallShell::getInstance();
    bool isBackground = _isBackgroundComamnd(m_args[m_argsNum-1]);

    ///Copying the command line so changes wont effect it
    char* copy = new char [strlen(m_cmdLine)+1];
    strcpy(copy,m_cmdLine);

    ///Copying the args so changes wont effect it
    char** copyArgs=new char *[COMMAND_MAX_ARGS+1];
    _parseCommandLine(copy,copyArgs);

    ///Different handling for background jobs
    if(isBackground)
    {
        _removeBackgroundSign(copy);
        _parseCommandLine(copy,copyArgs);

    }
    pid_t PID = fork();
    if(PID == -1)
    {
        perror("smash error: fork failed");
        return;
    }
    if(PID == 0)
    {
        ///Son
        if(setpgrp() == -1)
        {
            perror("smash error: setpgrp failed");
            exit(0);
        }
        ///Checking if its complex or not
        if(includesChar(copy,'*') || includesChar(copy,'?'))
        {
            ///Is Complex
            if (execl("/bin/bash", "bash", "-c", copy,NULL) == -1) {
                perror("smash error: execl failed");
               exit(0);
            }
        }
        else
        {
            ///Is not complex
            if (execvp(copyArgs[0],copyArgs) == -1) {
                perror("smash error: execvp failed");
                exit(0);
            }
        }
    }
    else
    {
        ///Father
        if(isBackground)
        {
            ///A background job
            shell.m_jobsList.addJob(m_userGiven,PID);
        }
        else
        {
            ///Not background


            ///Setting the foreground job PID
            shell.m_foregroundPID = PID;
            if(waitpid(PID, NULL,0) == -1)
            {
                perror("smash error: waitpid failed");
                shell.m_foregroundPID = -1;
                return;
            }
            ///Foreground job is Done
            shell.m_foregroundPID = -1;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////WHOAMI Command Implementation///////////////////////////////////////////////////////////
void WhoAmICommand::execute() {
    uid_t uid = getuid();
    ///This file contains the needed information
    int fd = open("/etc/passwd", O_RDONLY);
    if (fd == -1) {
        ///Open failure
        perror("smash error: open failed");
        return;
    }

    size_t BUFFER_SIZE = 4096;///Assuming the memory for pages is 4k (ask on piazza)
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    string usersInfo;
    ///Reading from the file
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        usersInfo.append(buffer, bytesRead);
    }

    if (bytesRead == -1) {
        ///Reading failure
        perror("smash error: read failed");
        close(fd);
        return;
    }
    ///Searching for the user info for the specific user
    size_t pos = 0;
    while(pos < usersInfo.size())
    {
        ///Searching for end of line
        size_t newlinePos = usersInfo.find('\n', pos);
        if (newlinePos == string::npos) {
            ///Single line treatment
            newlinePos = usersInfo.size();
        }

        ///Extracting the current line
        string currLine = usersInfo.substr(pos, newlinePos - pos);
        pos = newlinePos + 1;

        /// Extract the places of the information from the current line
        size_t usernameColon = currLine.find(':');
        size_t passwordColon = currLine.find(':', usernameColon + 1);///Not needed for checking
        size_t uidColon = currLine.find(':', passwordColon + 1);
        size_t gidColon = currLine.find(':', uidColon + 1);///Not needed for checking
        size_t infoColon = currLine.find(':', gidColon + 1);///Not needed for checking
        size_t homeDirColon = currLine.find(':', infoColon + 1);


        bool badFormat = usernameColon == string::npos || passwordColon == string::npos || uidColon == string::npos;
        if (badFormat || gidColon == string::npos|| infoColon == string::npos || homeDirColon == string::npos) {
            ///Skipping bad formatted lines (Assume this isn't reached)
            continue;
        }


        ///Taking the corresponding substring based on the already calculated location
        string username = currLine.substr(0, usernameColon);
        string userUidStr = currLine.substr(passwordColon + 1, uidColon - passwordColon - 1);
        string homeDir = currLine.substr(infoColon + 1, homeDirColon - infoColon - 1);

        try
        {
         int userUid = stoi(userUidStr);
         if(userUid == uid) {
             cout << username << " " << homeDir << endl;
         }
        }
        catch (...)
        {
            ///Stoi exception, it's ok to assume we will not get here
        }

    }
    ///Safe to assume that a corresponding user will be found and no need to deal with the user not being found


    ///Closing before exiting
    close(fd);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////NetInfo Command Implementation/////////////////////////////////////////////////
#define IFNAME_SIZE 16
#define BUFFER_SIZE2 4096
#define SIZE 256
int debug_broski(int hi,int bye)
{
    if(hi == bye)
        return 1;
}
void debug_broski2()
{
    debug_broski(1,0);
}
void debug_final_boss(int index)
{
    while (index > 0)
    {
        debug_broski(1,1);
        index--;
    }

}
bool IPhelper(ifreq& ifr, int sock, char* ipAddress) {
    debug_final_boss(18);
    if (syscall(SYS_ioctl, sock, SIOCGIFADDR, &ifr) == 0) {
        unsigned char* ip = (unsigned char*)&ifr.ifr_addr.sa_data[2];
        snprintf(ipAddress, SIZE, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
        return true;
    } else {
        perror("smash error: ioctl failed");
        syscall(SYS_close, sock);
        return false;
    }
    debug_broski(1,1);
}

void extractDnsServers(char* buffer, char dnsServers[][SIZE], int* count) {
    char* line = strtok(buffer, "\n");
    *count = 0;
    debug_broski(2,2);
    while (line) {
        if (strncmp(line, "nameserver", 10) == 0) {
            const char* addr = line + 10;
            while (*addr == ' ' || *addr == '\t') addr++;  // Skip leading spaces/tabs
            strncpy(dnsServers[*count], addr, SIZE - 1);
            dnsServers[*count][SIZE - 1] = '\0';  // Ensure null termination
            (*count)++;
        }
        line = strtok(NULL, "\n");
    }
}

bool subnetHelper(ifreq& ifr, int sock, char* subnetMask) {
    if (syscall(SYS_ioctl, sock, SIOCGIFNETMASK, &ifr) == 0) {
        unsigned char* mask = (unsigned char*)&ifr.ifr_addr.sa_data[2];
        snprintf(subnetMask, SIZE, "%u.%u.%u.%u", mask[0], mask[1], mask[2], mask[3]);
        return true;
    } else {
        perror("smash error: ioctl failed");
        syscall(SYS_close, sock);
        return false;
    }
}

void processRouteTable(char* buffer, char* gateway) {
    char* line = strtok(buffer, "\n");
    debug_final_boss(15);
    while (line) {
        if (strncmp(line, "Iface", 5) == 0) {
            line = strtok(NULL, "\n");
            continue;
        }

        char iface[IFNAME_SIZE];
        char destination[32], gatewayHex[32];

        sscanf(line, "%s %s %s", iface, destination, gatewayHex);

        if (strcmp(destination, "00000000") == 0) {
            unsigned int gw;
            sscanf(gatewayHex, "%x", &gw);
            snprintf(gateway, SIZE, "%u.%u.%u.%u",
                     (gw & 0xFF), ((gw >> 8) & 0xFF), ((gw >> 16) & 0xFF), ((gw >> 24) & 0xFF));
            return;
        }
        line = strtok(NULL, "\n");
    }
    strcpy(gateway, "");
}

void getDefaultGateway(char* gateway) {
    const char* routePath = "/proc/net/route";
    int fd = syscall(SYS_open, routePath, O_RDONLY);
    if (fd == -1) {
        perror("smash error: open failed");
        strcpy(gateway, "Unknown");
        return;
    }

    char buffer[BUFFER_SIZE2];
    ssize_t bytesRead = syscall(SYS_read, fd, buffer, sizeof(buffer) - 1);
    debug_final_boss(22);
    if (bytesRead <= 0) {
        perror("smash error: read failed");
        syscall(SYS_close, fd);
        strcpy(gateway, "Unknown");
        return;
    }
    debug_broski2();
    // Null-terminate the buffer and process the route table
    buffer[bytesRead] = '\0';
    syscall(SYS_close, fd);

    processRouteTable(buffer, gateway);
}

void printNetworkInfo(const char* ipAddress, const char* subnetMask, const char* defaultGateway, char dnsServers[5][SIZE], int dnsCount) {
    debug_broski2();
    printf("IP Address: %s\n", ipAddress);
    printf("Subnet Mask: %s\n", subnetMask);
    printf("Default Gateway: %s\n", defaultGateway);
    printf("DNS Servers: ");
    for (int i = 0; i < dnsCount; ++i) {
        printf("%s", dnsServers[i]);
        if (i < dnsCount - 1) {
            printf(", ");
        }
    }
    printf("\n");
}

bool getInterfaceDetails(const char* interface, char* ipAddress, char* subnetMask) {
    int sock = syscall(SYS_socket, AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("smash error: socket failed");
        return false;
    }
    debug_final_boss(15);
    struct ifreq ifr;
    strncpy(ifr.ifr_name, interface, IFNAME_SIZE - 1);
    ifr.ifr_name[IFNAME_SIZE - 1] = '\0';

    if (!IPhelper(ifr, sock, ipAddress)) {
        syscall(SYS_close, sock);
        return false;
    }
    debug_broski(2,2);
    if (!subnetHelper(ifr, sock, subnetMask)) {
        syscall(SYS_close, sock);
        return false;
    }

    syscall(SYS_close, sock);
    return true;
}

void getDnsServers(char dnsServers[][SIZE], int* count) {
    const char* resolvPath = "/etc/resolv.conf";
    int fd = syscall(SYS_open, resolvPath, O_RDONLY);
    if (fd == -1) {
        perror("smash error: open failed");
        return;
    }

    char buffer[BUFFER_SIZE2];
    ssize_t bytesRead = syscall(SYS_read, fd, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0) {
        perror("smash error: read failed");
        syscall(SYS_close, fd);
        return;
    }
    debug_broski2();
    buffer[bytesRead] = '\0';
    syscall(SYS_close, fd);
    debug_final_boss(43);
    extractDnsServers(buffer, dnsServers, count);  // Calling the helper function to process the buffer
}

void netinfoHelper(const char* interface) {
    char ipAddress[SIZE], subnetMask[SIZE], defaultGateway[SIZE];
    char dnsServers[5][SIZE];
    int dnsCount = 0;
    debug_final_boss(4);
    if (!getInterfaceDetails(interface, ipAddress, subnetMask)) {
        return;
    }

    getDefaultGateway(defaultGateway);
    getDnsServers(dnsServers, &dnsCount);
    debug_final_boss(32);
    printNetworkInfo(ipAddress, subnetMask, defaultGateway, dnsServers, dnsCount);
}

void NetInfo::execute() {
    if (m_argsNum != 2) {
        cerr << "smash error: netinfo: interface not specified" << endl;
        return;
    }
    netinfoHelper(m_args[1]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////Pipe Command Implementation/////////////////////////////////////////////////
void PipeCommand::execute() {
    SmallShell& smallShell = SmallShell::getInstance();
    bool stderrNeeded = string(m_cmdLine).find("|&") != string::npos;
    int pipeFD[2];
    if (pipe(pipeFD) == -1) {
        perror("smash error: pipe failed");
        return;
    }
    pid_t pidCmd1 = fork();
    if (pidCmd1 == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (pidCmd1 == 0) {
        // Child process for Command 1
        // Redirect stdout (and stderr if needed)
        if(setpgrp() == -1)
        {
            perror("smash error: setpgrp failed");
            return;
        }
        if (stderrNeeded) {
            // Redirect stderr to pipe's write end
            if (dup2(pipeFD[1], STDERR_FILENO) == -1) {
                perror("smash error: dup2 failed");
                return;
            }
        } else {
            // Redirect stdout to pipe's write end
            if (dup2(pipeFD[1], STDOUT_FILENO) == -1) {
                perror("smash error: dup2 failed");
                return;
            }
        }
        close(pipeFD[0]);// Close the read end of the pipe
        string cmdLine1 = string(m_cmdLine);  // Convert to string only once
        if (stderrNeeded) {
            size_t pipePos = cmdLine1.find("|&");
            if (pipePos != string::npos) {
                cmdLine1 = cmdLine1.substr(0, pipePos);  // Get substring before "|&"
            }
        } else {
            size_t pipePos = cmdLine1.find('|');
            if (pipePos != string::npos) {
                cmdLine1 = cmdLine1.substr(0, pipePos);  // Get substring before "|"
            }
        }
        Command* cmd1 = smallShell.CreateCommand(cmdLine1.c_str());
        if(cmd1 != nullptr)
            cmd1->execute();
        exit(1);
    }

    // Fork the second child (command2)
    pid_t pidCmd2 = fork();
    if (pidCmd2 == -1) {
        perror("smash error: fork failed");
        return;
    }

    if (pidCmd2 == 0) {
        // Child 2: Redirect stdin to the pipe's read end
        if(setpgrp() == -1)
        {
            perror("smash error: setpgrp failed");
            return;
        }
        if (dup2(pipeFD[0], STDIN_FILENO) == -1) {
            perror("smash error: dup2 failed");
            return;
        }
        close(pipeFD[1]);  // Close the write end of the pipe
        string cmd2Line;
        if (stderrNeeded) {
            size_t pipePos = string(m_cmdLine).find("|&");
            if (pipePos != string::npos) {
                cmd2Line = string(m_cmdLine).substr(pipePos + 2);  // Get substring after "|&" (skip 2 chars for "|&")
            }
        } else {
            size_t pipePos = string(m_cmdLine).find('|');
            if (pipePos != string::npos) {
                cmd2Line = string(m_cmdLine).substr(pipePos + 1);  // Get substring after "|"
            }
        }
        Command* cmd2 = smallShell.CreateCommand(cmd2Line.c_str());
        if(cmd2 != nullptr)
            cmd2->execute();
        exit(1);
    }

// Parent: Close both ends of the pipe and wait for children
    close(pipeFD[0]);
    close(pipeFD[1]);
    waitpid(pidCmd1, nullptr, 0);
    waitpid(pidCmd2, nullptr, 0);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
