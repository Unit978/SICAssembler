
/*
    The command line interpreter.

    This object reads lines entered by the user and parses
    them in order to execute the proper command with the
    respective parameters.
*/

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "util.h"
#include "command.h"

using std::cin;

class Interpreter{
    //container of commands
    private:
        DynamicArray<Command> commands;

        //signals TRUE if a command was executed
        //signals FALSE if command was unrecognized
        bool process(const DynamicArray<string>& parsedLine){
            //find Command
            string first = parsedLine.at(0);
            for(unsigned i = 0; i < commands.size(); i++){
                const Command* comm = &commands.at(i);
                //command found
                if( Util::isPrefix(first, comm->getName()) ){
                    if( first.length() >= comm->getNameTol() ){
                        comm->process(parsedLine);
                        return true;
                    }
                }
            }
            return false;
        }

    public:
        Interpreter(){}

        void run(){
            string delims = "\t ";
            while(true){
                bool errorFlag = false;

                //prompt
                cout << "\ncommand >>> ";

                //read user input
                string line;
                getline(cin, line);

                DynamicArray<string> parsedLine;
                Util::parseLine(parsedLine, line, delims);

                if(parsedLine.size() != 0){
                    //check for exit
                    string com = parsedLine.at(0);
                    if( Util::isPrefix(com, "exit") && com.length() > 2 )
                        break;

                    //execute other commands
                    //if the the process failed then set errorFlag.
                    errorFlag = !process(parsedLine);
                }
                //unrecognized command
                if(errorFlag)
                    cout << "Command not recognized. Enter 'help' for a list of available commands.";
            }
        }

        //Specify the name and number of parameters for the command and
        //pass the function that you want processed by the command
        void addCommand(string name, unsigned params, unsigned nameTol, void(*execution)(const DynamicArray<string>&) ){
            Command c(name, params, nameTol, execution);
            commands.push_back(c);
        }

        void removeCommand(string name){
            commands.remove(Command(name));
        }
};

#endif
