# Debug Setup in VS Code

Debugging NS-3 with VSCode (built with waf) is divided into the following steps: first compile NS-3, then configure the debugging tasks in VSCode (launch.json and tasks.json), and finally you can set breakpoints, single-step debugging, and view variables in the IDE.

## 1. Compiling NS-3
Before debug NS-3 make sure you have successfully compile the NS-3 by follwing the steps in [ vm-env.md](#vm-env.md)

## 2. Configuring Debugging Tasks for VS Code

Configuring `tasks.json`: `tasks.json` is used to define build tasks that can automatically invoke build commands before starting debugging. Create a `.vscode/tasks.json` file with content similar to the following:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build NS-3",
            "type": "shell",
            "command": "./ns3",
            "args": [
                "build"
            ],
            "options": {
                "cwd": "${workspaceFolder}"
            },
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": []
        }
    ]
}
```
Configuring `launch.json`: `launch.json` is used to tell VSCode how to launch the debugger to run the NS-3 emulation program. Since NS-3 uses waf scripts to manage execution, you can launch the emulation via waf and debug the emulation scripts you specify internally. Create a `.vscode/launch.json` file with content similar to the following (change the `exampleA` to the code name, which you want to debug):

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug NS-3: exampleA",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/ns3.39/build/contrib/p4sim/examples/exampleA",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}/ns3.39",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable GDB beautification printing",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                },
                {
                    "description": "Set the disassembly format to Intel",
                    "text": "-gdb-set disassembly-flavor intel",
                    "ignoreFailures": true
                }
            ]
        }
    ]
}
```
 **program**: here you specify the path to the waf script. Since NS-3 simulation is started by waf, debugging the waf process directly will start the corresponding executable file inside waf.

**args**: the running parameter passed to waf. You can change it to the path of the script you want to run or other parameters according to the actual situation.

## 3.Starting Debugging
In VSCode's debugging panel (the “Run and Debug” icon in the sidebar), select the “Debug NS-3 Simulation” configuration you just configured.
Set the breakpoints you want.
Click on the Start Debug button. The debugger calls the pre-configured build task (tasks.json), then starts waf and executes the specified NS-3 simulation program.
When it reaches the breakpoint, VSCode pauses the program and you can view the call stack, variable values, and single-step execution code.