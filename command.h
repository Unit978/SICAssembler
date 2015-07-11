/*
    Command object for the interpreter.
    Makes it easier to create new commands and execute
    their respective action.
*/

#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include "util.h"
#include "dynamic_array.h"

typedef DynamicArray<string> array;

class Command{

    private:
        string name;
        unsigned parameters;

        //tolerance for string subset
        //This can be used to differentiate
        //which subsets belong to their super set.
        //For example ex can belong to execute or exit
        //we can set a tolerance of 3 so it can recognize
        //exe as execute and exi as exit.
        unsigned nameTolerance;

        //function pointer to the execution to be
        //done by the command
        void(*execution)(const array&);

    public:
        Command(): name(""), parameters(0), nameTolerance(0){}
        Command(string name) : name(name), parameters(0), nameTolerance(1) {}
        Command(const string& name, unsigned parameters, unsigned nameTol, void(*exe)(const array&)) :
                        name(name), parameters(parameters), nameTolerance(nameTol), execution(exe){}

        //Assumption: Command name has been matched in the interpreter
        //Takes in an array "line" that contains the parsed line,
        //chopped up into its proper components.
        //This function will then process that array and run the
        //specified execution on that array.
        void process(const array& line) const{
            if(!line.empty()){
                //check for number of parameters
                if(line.size()-1 == this->parameters)
                    (*execution)(line);
                else
                    cout << "Error. " << name << " takes " << parameters << " parameter(s).";
            }
        }

        bool operator==(const Command& c) const{
            return name == c.name;
        }

        string getName() const{
            return name;
        }

        unsigned getNameTol() const{
            return nameTolerance;
        }
};

#endif
