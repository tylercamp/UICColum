using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace QuickData.UX
{
    public class QuickDataAPI
    {
        struct CommandResult
        {
            public bool CommandSucceeded;
            public String CommandName;

            public string OutputLocation;
            public bool OutputContainsMultipleFiles;
        }

        public String ExecutablePath { get; private set; }
        public bool IsExecuting { get; private set; }

        CommandResult? PendingResult;
        Thread TaskThread;
        Process ExecutingProcess;

        //  A QuickDataAPI object can only run one command at a time
        public QuickDataAPI(String quickdataExecutablePath)
        {
            ExecutablePath = quickdataExecutablePath;
        }

        /// <summary>
        /// Triggered when 
        /// </summary>
        public event Action<string> OnOutputReceived;

        private bool VerifyCommandName(String commandName)
        {
            return true; // TODO - commandName should be alphanumeric, no whitespace/other chars
        }

        private void InvokeCommand(Action<CommandResult> callback, Action<int> progressCallback, String commandName, params string[] args)
        {
            StringBuilder commandString = new StringBuilder();
            if (!VerifyCommandName(commandName))
                throw new Exception("Invalid command name!");

            String separator = "";
            foreach (var arg in args)
            {
                commandString.Append(separator);
                commandString.Append(arg);
                separator = "|";
            }

            ProcessStartInfo startInfo = new ProcessStartInfo
            {
                CreateNoWindow = true,
                UseShellExecute = false,
                ErrorDialog = false,
                FileName = ExecutablePath,
                RedirectStandardOutput = true,
                Arguments = "-stringcommand:" + commandString.ToString()
            };

            ExecutingProcess = Process.Start(startInfo);
            ExecutingProcess.BeginOutputReadLine();

            PendingResult = new CommandResult { CommandName = commandName };

            ExecutingProcess.OutputDataReceived += (sender, e) =>
            {
                if (OnOutputReceived != null)
                    OnOutputReceived(e.Data);
            };

            //  Spawn waiting thread
            IsExecuting = true;
            TaskThread = new Thread(() =>
            {
                //  Update PendingResult


                ExecutingProcess.WaitForExit();

                callback(PendingResult.Value);
                PendingResult = null;
                ExecutingProcess = null;
                IsExecuting = false;
            });

            
        }

        private void ExecutingProcess_OutputDataReceived(object sender, DataReceivedEventArgs e)
        {
            throw new NotImplementedException();
        }
    }
}
