/*
    Luis A. Lopez

    CS 3334 - Systems Programming

    This software is a combination of:
        -Command line interpreter
        -SIC simulator
        -SIC assembler

    The user will be able to enter multiple commands.
    For example, a command to show their current directory
    or a command to assemble an assembly source file.

    The interpreter has been designed so that commands
    can be added in a modular way. Create the commands as functions
    and add it to the interpreter via the loadCommands function.
    The current drawback is that the function parameter signature must
    be the same for all commands.

    This can be easily modified by having an overloaded version of
    the loadCommand() from the interpreter.

    Each command takes in a list of strings.
        1st element of the list is the command entered such as load
        The rest of the elements are the paramters. This could be a file path.

    Note: Data in the object file is all in hexadecimal.
*/

#include "interpreter.h"
#include "assembler.h"

extern "C"{
    #include "sicengine.h"
}

/*Globals for convenience among instructions*/
string s_firstAddress = "";

/*These are the implemented commands for the interpreter.*/

//Loads the object file specified (the parameter).
//It will take the data from the object file and load
//the necessary bytes in the SIC memory.
void load(const DynamicArray<string>& command){
    //reset data
    s_firstAddress = "";

    //the object file path is the 1st parameter
    ifstream objectfile(command.at(1));

    if(objectfile.is_open()){
        string s_textRecord;

        //ignore the header record
        getline(objectfile, s_textRecord);

        //NOTE:
        //Every two charcters represents two hex digits which are 1 byte in size.
        //Memory increases per byte.

        //go through all text records
        while(getline(objectfile, s_textRecord)){

            //end record reached
            if(s_textRecord.at(0) == 'E'){
                //save first executable address
                s_firstAddress = s_textRecord.substr(1, 6);
                break;
            }

            //obtain address from the record
            string s_address = s_textRecord.substr(1, 6);

            //get the data portion of the text record
            string s_byteData = s_textRecord.substr(9);

            string s_byte;
            int i_address = 0;
            int i_byte = 0;

            //convert string address to a numerical value
            Util::stringToInt(s_address, i_address, 16);

            //iterate through all byte data of current text record
            for(unsigned i = 0; i < s_byteData.length(); i += 2, i_address++){

                //add two characters which is 1 byte
                s_byte = "";
                s_byte.push_back(s_byteData[i]);
                s_byte.push_back(s_byteData[i+1]);

                //Do necessary conversions
                Util::stringToInt(s_byte, i_byte, 16);
                BYTE b = static_cast<BYTE>(i_byte);
                ADDRESS a = static_cast<ADDRESS>(i_address);

                //load the byte into memory
                PutMem(a, &b, 0);
            }
        }
        objectfile.close();
    }
    //object file failed to open
    else{
         cout << "Error. \"" << command.at(1) << "\" file for source was not found.\n";
    }
}

//Uses the object file loaded from the command "load".
//Assume that the object file has the correct format
//The second line should always be the 1st text record.
//This command will execute the object file prodcued
//by the assembler.
void exec(const DynamicArray<string>& command){
    //there is a specified starting address to start execution
    if(!s_firstAddress.empty()){
        //convert the address (which is base 16) to
        //a numerical value
        int i_address = 0;
        Util::stringToInt(s_firstAddress, i_address, 16);
        ADDRESS a = static_cast<ADDRESS>(i_address);

        //Execute the program
        SICRun(&a, FALSE);
    }
    else cout << "No starting address supplied from the object file.\n";
}

void debug(const DynamicArray<string>& command){
    cout << "'" << command.at(0) << "'" << " has not yet been implemented";
}

//Takes in two parameters which are two hexadecimal values
//that specify the memory range to display the memory content.
void dump(const DynamicArray<string>& command){
    int startAddr = 0;
    int endAddr = 0;

    //convert strings to int
    bool success =
        Util::stringToInt(command.at(1), startAddr, 16) &&
        Util::stringToInt(command.at(2), endAddr, 16);

    if(success){
        if(startAddr <= endAddr){
            int horizontalCols = 16;

            //go through every byte
            for(int address = startAddr; address <= endAddr; address++){

                //create new line at each of these intervals
                if( (address - startAddr) % horizontalCols == 0)
                    cout << endl;

                ADDRESS a = static_cast<ADDRESS>(address);
                BYTE b = 0;

                //fetch byte at the specified memory address;
                GetMem(a, &b, 0);

                //display content
                cout << std::hex << std::setw(6) << std::setfill('0');
                cout << address << " " << std::setw(2) << std::setfill('0');
                cout << (int)b << "   ";
            }
        }
        else cout << "Error. Starting value is greater than the ending value.\n";
    }
    else cout << "Failed to convert specified hexadecimal parameters.\n";
}

void help(const DynamicArray<string>& command){
    cout << "List of available commands:\n";
    cout << "\tload [file]\n\texecute\n\tdebug\n\tdump [start] [end]\n";
    cout << "\thelp\n\tassemble [file]\n\tdirectory\n\texit\n";
}

/*The Assembler*/
void assem(const DynamicArray<string>& command){
    Assembler assem;
    assem.pass1(command.at(1));     //pass in the assembly source file path
    assem.pass2();
}

void dir(const DynamicArray<string>& command){
    system("ls");
}

//Creates the commands for the interpreter.
void loadCommands(Interpreter& i){
    i.addCommand("load",    1, 1, &load);
    i.addCommand("execute", 0, 3, &exec);
    i.addCommand("debug",   0, 2, &debug);
    i.addCommand("dump",    2, 2, &dump);
    i.addCommand("help",    0, 1, &help);
    i.addCommand("assemble",  1, 1, &assem);
    i.addCommand("directory", 0, 2, &dir);
}

int main(){
    //initialize the SIC simulator
    SICInit();

    //Create command line interpreter for the SIC
    Interpreter i;
    loadCommands(i);
    i.run();

    return 0;
}
