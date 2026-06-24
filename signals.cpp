#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    SmallShell& shell = SmallShell::getInstance();
    cout << "smash: got ctrl-C" << endl;

    ///Avoid killing the Shell and an empty foreground job
    if(shell.m_foregroundPID == -1 || shell.m_foregroundPID == shell.m_Pid)
        return;
    else
    {
        if(kill(shell.m_foregroundPID,SIGKILL) == -1)
        {
            perror("smash error: kill failed");
            return;
        }
        ///Killing the foreground job
        cout << "smash: process " << shell.m_foregroundPID << " was killed" << endl;
        shell.m_foregroundPID = -1;
    }
}
