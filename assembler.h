
/*
    A two pass assembler for the SIC machine.

    Pass 1 obtains the assembly source and writes to an
    intermediate file which splits the key data per instruction.
    Pass 1 also creates a symbol table which associates a symbol
    with some address. Error checking is done here.

    Pass2 then uses the intermediate file to create a listing file
    and an object file for the assembly source.
    The listing file contains the loading addresses for each instruction
    along with the generated object code, source line, and any errors
    associated with that source line.
    The object file contains the machine code translation of the assembly
    source in hex.
    Error checking is also done here.
*/

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iomanip>

extern "C"{
    #include "sicengine.h"
}

using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::unordered_map;

class Assembler{

    private:
        //the location counter
        int locctr;
        int startingAddress;
        int programLength;

        //the number of digits per error code
        //ex) "0001"
        const int errorCodeSize = 4;

        //format padding for the listing file
        const int addressPadding = 4;
        const int objectCodePadding = 8;

        //format padding for the object file
        const int basicPadding = 6;             //for the name, address, start execution, load address, and program size
        const int sizePadding = 2;              //for byte size
        const int machineCodePadding = 60;      //for machine code section
        const int opcodePadding = 2;

        const int maxProgramSizeBytes = MSIZE;

        bool anyErrors;

        //builds up a string of error codes for a
        //line in the asssembly source;
        string errors;

        //label and their address
        unordered_map<string, unsigned> symbolTable;

        //Mneumonic and their opcode in hex
        unordered_map<string, unsigned> opcodeTable;

        //errors
        unordered_map<string, string> errorCodes;

        void createErrorCodes(){
            errorCodes.insert(std::make_pair("0001", "Invalid Operand"));
            errorCodes.insert(std::make_pair("0002", "Duplicate Symbol"));
            errorCodes.insert(std::make_pair("0003", "Invalid Opcode"));
            errorCodes.insert(std::make_pair("0004", "Invalid Symbol"));

            //BYTE operand errors
            errorCodes.insert(std::make_pair("0005", "Missing Quotes"));
            errorCodes.insert(std::make_pair("0006", "Odd number of hex digits"));
            errorCodes.insert(std::make_pair("0007", "String too long"));
            errorCodes.insert(std::make_pair("0008", "Hex too long"));
            errorCodes.insert(std::make_pair("0009", "Specifier must be C or X"));

            errorCodes.insert(std::make_pair("0010", "Symbol too long"));
            errorCodes.insert(std::make_pair("0011", "Symbol starts with a non-letter character"));
            errorCodes.insert(std::make_pair("0012", "Symbol contains non-alphanumeric characters"));

            errorCodes.insert(std::make_pair("0013", "Operand contains non-alphanumeric characters"));

            errorCodes.insert(std::make_pair("0014","Missing START operand"));
            errorCodes.insert(std::make_pair("0015","Misplaced/Duplicate START"));
            errorCodes.insert(std::make_pair("0016","Illegal START Operand"));

            errorCodes.insert(std::make_pair("0017","Illegal END operand"));
        }

        void createOpTable(){
            opcodeTable.insert(std::make_pair("ADD", 0x18));
            opcodeTable.insert(std::make_pair("AND", 0x58));
            opcodeTable.insert(std::make_pair("COMP", 0x28));
            opcodeTable.insert(std::make_pair("DIV", 0x24));
            opcodeTable.insert(std::make_pair("J", 0x3C));

            opcodeTable.insert(std::make_pair("JEQ", 0x30));
            opcodeTable.insert(std::make_pair("JGT", 0x34));
            opcodeTable.insert(std::make_pair("JLT", 0x38));
            opcodeTable.insert(std::make_pair("JSUB", 0x48));
            opcodeTable.insert(std::make_pair("LDA", 0x00));

            opcodeTable.insert(std::make_pair("LDCH", 0x50));
            opcodeTable.insert(std::make_pair("LDL", 0x08));
            opcodeTable.insert(std::make_pair("LDX", 0x04));
            opcodeTable.insert(std::make_pair("MUL", 0x20));
            opcodeTable.insert(std::make_pair("OR", 0x44));

            opcodeTable.insert(std::make_pair("RD", 0xD8));
            opcodeTable.insert(std::make_pair("RSUB", 0x4C));
            opcodeTable.insert(std::make_pair("STA", 0x0C));
            opcodeTable.insert(std::make_pair("STCH", 0x54));
            opcodeTable.insert(std::make_pair("STL", 0x14));

            opcodeTable.insert(std::make_pair("STX", 0x10));
            opcodeTable.insert(std::make_pair("SUB", 0x1C));
            opcodeTable.insert(std::make_pair("TD", 0xE0));
            opcodeTable.insert(std::make_pair("TIX", 0x2C));
            opcodeTable.insert(std::make_pair("WD", 0xDC));
        }

        //For BYTE directive
        //Returning -1 signifies invalid operand which is taken into
        //account outside this function;
        int getConstantLength(const string& operand){
            unsigned operlen = operand.length();

            //must have at least a length of 4
            if(operlen < 4)
                return -1;

            unsigned stringLimit = 30;   //30 character limit
            unsigned hexLimit = 32;      //32 hex digit limit

            //first character of operand
            char c = operand.at(0);

            //Invalid specifier for BYTE directive. Has something other than
            //C or X
            if(c != 'C' && c != 'X'){
                errors += "0009" ;
                return -1;
            }
            //if there no quotes then there is an error
            if(operand.at(1) != '\'' || operand.at(operlen-1) != '\''){
                errors += "0005";
                return -1;
            }
            //the true operand length - exclude the specifier and the two quotes
            operlen -= 3;

            //check for formatting
            if(c == 'C'){
                //too many characters
                if(operlen > stringLimit){
                    errors += "0007";
                    return -1;
                }
                return operlen;
            }
            else if(c == 'X'){
                //start at the first hex digit and stop right before the last quote
                //Test that the characters in between are valid hex numbers
                for(unsigned i = 2; i < operand.length()-1; i++){
                    //not a hex digit
                    if(!Util::isHexDigit(operand[i])){
                        return -1;
                    }
                }
                //too many digits
                if(operlen > hexLimit){
                    errors += "0008";
                    return -1;
                }
                //odd number of hex digits
                if(operlen % 2 == 1){
                    errors += "0006";
                    return -1;
                }

                //each hex digit takes up 4 bits. Half a byte
                return operlen/2;
            }
            //invalid operand
            return -1;
        }

        void getColumns(string& srcLine, const string& delims, string& label, string& opcode, string& operand){
            //convert to uppercase
            Util::toUpperCase(srcLine);

            DynamicArray<string> parsedLine;
            Util::parseLine(parsedLine, srcLine, delims);
            parsedLine.pad("");
            parsedLine.forceSize(4);

            //parsed line should now have the format:
            //  0      1      2      4
            //LABEL OPCODE OPERAND COMMENT

            //if there isnt a label (there is white space)
            if(Util::findChar(delims, srcLine.at(0)))
                //add empty label field
                parsedLine.push_front("");

            label = parsedLine.at(0);
            opcode = parsedLine.at(1);
            operand = parsedLine.at(2);
        }
        //symbol must be of size 6 or less, must start with a letter, can be alphanumeric
        bool isValidSymbol(const string& src){
            if(src.length() > 6){
                errors += "0010";
                return false;
            }

            if( !Util::isAlpha(src.at(0)) ){
                errors += "0011";
                return false;
            }

            for(unsigned i = 1; i < src.length(); i++)
                if(!Util::isAlphaNumeric(src[i])){
                    errors += "0012";
                    return false;
                }

            return true;
        }

        //This does not apply to the BYTE directive. GetConstantLength()
        //takes care of the BYTE directive operand correctness.
        bool isValidOperand(const string& src){
            //empty operands are invalid
            if(src.empty())
                return false;

            //check if it a hex operand
            if(src[0] == '0')
                if(!isHexSymbol(src))
                    return false;

            //if there is going to be indexing then there
            //must be at least 3 characters "c,X"

            //check for possible indexing
            int size = src.length();
            if(size >= 3){
                char last = src.at(size-1);
                char secondLast = src.at(size-2);

                //indexing
                if(last == 'X' && secondLast == ','){
                    //check for alpha numeric operand
                    for(int i = 0; i < size-2; i++)
                        if(!Util::isAlphaNumeric(src[i])){
                            errors += "0013";
                            return false;
                        }
                    return true;
                }
            }
            //test for non-indexed operand
            for(int i = 0; i < size; i++)
                if(!Util::isAlphaNumeric(src[i])){
                        errors += "0013";
                        return false;
                    }
            return true;
        }

        //For indexing.
        //SIC has a maximum memory of 32K bytes which requires 16 bits.
        //Address values will not exceed 16 bits so we can use
        //16 bits and set the MSB to 1 to indicate indexing
        //The parameter is a symbol value which is some address.
        void setMSB(int& address){
            //The MSB is the 15th bit, since we start from 0.
            //So, 16 - 1 = 15
            address |= 1 << (15);
        }

        bool isIndexedOperand(const string& operand){
            unsigned end = operand.length() - 1;

            //at least 3 characters in length "B,X"
            if(end >= 2){
                if(operand[end] == 'X' && operand[end-1] == ',')
                    return true;
            }
            return false;
        }

        //gets the operand value and ignores the ",X" indexed specifier
        string getOperandFromIndexed(const string& source){
            string buffer = "";
            for(char c : source)
                if(c != ',')
                    buffer.push_back(c);

                //comma reached. We are done
                else break;
            return buffer;
        }

        //checks if a symbol is a hex address
        bool isHexSymbol(const string& symbol){
            //must start with a digit
            return symbol.length() > 0 && Util::isDigit(symbol[0]) && Util::hasHexFormat(symbol);
        }

        //extracts the string or hex value from a BYTE operand. Ignores the specifier and quotes.
        string getByteOperand(const string& operand){
            string operandValue = "";
            unsigned end = operand.length() - 1;
            if(operand.length() > 3)
                //start right after the 1st quote
                //stop before the last quote
                for(unsigned i = 2; i < end; i++)
                    operandValue.push_back(operand[i]);
            return operandValue;
        }

        //returns a string version of the objectCode in hex
        //opcodes should already be in base 16. They are being read from the
        //intermediate file
        string createObjectCode(const string& opcode, string& operand){

            //no object code is produced for these two directives.
            if(opcode == "RESB" || opcode == "RESW")
                return "";

            stringstream objectCodeStream("");

            //For the BYTE operand (strings and hex numbers)
            stringstream byteDataStream("");

            bool objectCodeGenerated = false;
            int addressObjectCode = -1;
            bool isIndexed = isIndexedOperand(operand);

            //check for indexing. Last two characters are ,X
            if(isIndexed)
                //extract pure operand (without ",X")
                operand = getOperandFromIndexed(operand);

            //find the operand symbol from the symbol table
            unordered_map<string, unsigned>::const_iterator symItr;
            symItr = symbolTable.find(operand);

            //convert constants to proper object code
            if(opcode == "BYTE"){
                char type = operand[0];
                string operandValue = getByteOperand(operand);

                //string - convert characters to ascii values for the object code
                if(type == 'C'){
                    for(char c : operandValue)
                        byteDataStream << std::hex << (int)c;
                }

                //hex - give direct values to the object code
                else if(type == 'X'){
                    for(char c : operandValue)
                        byteDataStream << c;
                }
                objectCodeGenerated = true;
            }
            else if(opcode == "WORD"){
                //obtain a base 10 number
                Util::stringToInt(operand, addressObjectCode, 10);
                objectCodeStream << std::setw(basicPadding) << std::setfill('0');
                objectCodeStream << std::hex << addressObjectCode;
            }
            //a hex address - must start with 0 (zero)
            //this value should be associated with an instruction
            else if(isHexSymbol(operand)){

                //give direct values to the object code - convert from base 16 to 10
                Util::stringToInt(operand, addressObjectCode, 16);
                objectCodeStream << std::setw(opcodePadding) << std::setfill('0');
                objectCodeStream << opcode;
                objectCodeGenerated = true;
            }
            //symbol from sym table
            //these symbols should be associated to an instruction. Not a directive.
            else if(symItr != symbolTable.end()){
                //give symbol value to the object code
                //This numeric value is in base 10
                addressObjectCode = symItr->second;

                //modify operand value if indexing is set
                if(isIndexed)
                    setMSB(addressObjectCode);

                objectCodeStream << std::setw(opcodePadding) << std::setfill('0');
                objectCodeStream << opcode;
                objectCodeGenerated = true;
            }
            //special cases
            else{
                /*RSUB case*/
                unordered_map<string, unsigned>::const_iterator rsubItr;
                rsubItr = opcodeTable.find("RSUB");
                int iOpcode = -1;

                //rsub opcode is in base 16
                if(Util::stringToInt(opcode, iOpcode, 16)){

                    //check if opcode matches to RSUB's
                    if(iOpcode == (int)rsubItr->second){
                        objectCodeStream << std::left << std::setw(basicPadding) << std::setfill('0');
                        objectCodeStream << opcode;
                        objectCodeGenerated = true;
                    }
                }
            }
            //WORD is handles in its else if block
            //Insert the address objectCode.
            if(objectCodeGenerated){
                //Handle instructions
                if(addressObjectCode != -1){
                    objectCodeStream << std::setw(addressPadding) << std::setfill('0') << std::hex;
                    objectCodeStream << addressObjectCode;
                }
                //Handle BYTE directive - only contains data (string or hex number)
                else objectCodeStream << byteDataStream.str();
            }
            return objectCodeStream.str();
        }

        //Each error code is of size "errorCodeSize" within the errorList string
        bool reportErrors(ofstream& listingFile, const string& errorList){
            unsigned len = errorList.length();
            if(len > 0){
                unordered_map<string, string>::const_iterator itr;
                listingFile << "\tErrors: ";
                string error;
                int counter = 1;
                for(unsigned i = 0; i < len; i++){
                    error.push_back(errorList[i]);
                    counter++;
                    //reset counter and add error to object file
                    if(counter > errorCodeSize){
                        counter = 1;

                        //if error was found in table
                        itr = errorCodes.find(error);
                        if(itr != errorCodes.end())
                            listingFile << itr->second << ", ";
                        else listingFile << "Unknown error reported. Something went wrong in the intermediate file.\t";
                        error = "";
                    }
                }
                //errors were found
                return true;
            }
            //no errors were found
            return false;
        }

        void createHeaderRecord(ofstream& objectfile, string progName, string address, int progLen){
            objectfile << "H" << std::left << std::setw(basicPadding) << std::setfill(' ');
            objectfile << progName;

            objectfile << std::right << std::setw(basicPadding) << std::setfill('0');

            Util::toUpperCase(address);
            objectfile << address;

            objectfile << std::setw(basicPadding) << std::setfill('0');
            objectfile << std::uppercase << std::hex;
            objectfile << progLen << endl;
        }

        void createEndRecord(ofstream& objectfile, int startingAddress){
            objectfile << "E" << std::setw(basicPadding) << std::setfill('0');
            objectfile << std::uppercase <<  std::hex;
            objectfile << startingAddress;
        }

        //Sets up the "T" and the address
        void startTextRecord(ofstream& objectfile, string address){
            objectfile << "T" << std::setw(basicPadding) << std::setfill('0');
            Util::toUpperCase(address);
            objectfile << address;
        }

        //Adds the size and the machine code/data to the text record
        void finishTextRecord(ofstream& objectfile, int machineBufferSize,
                                const stringstream& machineCodeStreamBuffer)
        {
            objectfile << std::setw(sizePadding) << std::setfill('0');
            objectfile << std::uppercase << std::hex;

            //convert to bytes
            objectfile << machineBufferSize/2;

            string code = machineCodeStreamBuffer.str();
            Util::toUpperCase(code);
            objectfile << code << endl;
        }

        void writeToListingFile(ofstream& listingfile, string address, string objectCode,
                                    const string& sourceLine, const string& errorList)
        {
            Util::toUpperCase(address);
            Util::toUpperCase(objectCode);

            char addressFill = '0';
            if(address.empty())
                addressFill = ' ';

            listingfile << std::setw(addressPadding) << std::setfill(addressFill);
            listingfile << address << " ";

            listingfile << std::setw(objectCodePadding) << std::setfill(' ');
            listingfile << objectCode << " ";

            listingfile << sourceLine;
            reportErrors(listingfile, errorList);
            listingfile << endl;
        }

    public:
        Assembler(){
            createOpTable();
            createErrorCodes();
            programLength = 0;
            startingAddress = 0;
            anyErrors = false;
        }

        void pass1(const string& src){
            ifstream source(src);
            ofstream intermediate("intermediate.txt");

            if(!source.is_open()){
                cout << "Failed to load specified file\n";
                return;
            }
            //read entire file
            string delims = "\t ";
            string srcLine;

            string label;
            string operand;
            string opcode;

            bool startFound = false;

            while(getline(source, srcLine)){
                errors.clear();
                //ignore empty lines
                if(srcLine.empty()) continue;

                //ignore comments
                char firstChar = srcLine.at(0);
                if(firstChar == '.') continue;

                getColumns(srcLine, delims, label, opcode, operand);

                //empty columns
                if(label.length() + opcode.length() + operand.length() == 0)
                    continue;

                /*Find the START directive*/
                if(opcode == "START"){
                    //Misplaced START or multiple ones.
                    if(startFound){
                        errors += "0015";
                        anyErrors = true;
                    }
                    startFound = true;

                    //check symbol validity
                    if(label.length() > 0)
                        if(!isValidSymbol(label))
                            errors += "0004";

                    //set locctr to OPERAND of START if possible
                    if(operand.empty() || !Util::stringToInt(operand, locctr, 16)){
                        //failed to give a proper value for locctr
                        locctr = startingAddress = 0;
                        errors += "0001";
                    }
                    else{
                        startingAddress = locctr;
                    }
                    intermediate << srcLine << endl;
                    intermediate << opcode << endl;
                    intermediate << std::hex << locctr << endl;
                    intermediate << operand << endl;
                    intermediate << errors << endl;
                    continue;
                }
                //No START directive found
                //We do not do a "continue" here because the current line is some instruction.
                else if(!startFound){
                    locctr = startingAddress = 0;
                    startFound = true;
                }

                //check for valid operand for instruction opcodes
                //that do not have the following directives
                if(opcode != "BYTE" && opcode != "WORD" && opcode != "RESW" && opcode != "RESB")
                    //not a valid symbol name or hex value
                    if(!isValidOperand(operand))
                        errors += "0001";

                if(opcode == "END"){
                    //write last line to intermediate file and save the program length
                    intermediate << srcLine << endl;
                    intermediate << opcode << endl;
                    intermediate << std::hex << locctr << endl;
                    intermediate << operand << endl;

                    //end contains an invalid operand
                    if(!isValidSymbol(operand) && !isHexSymbol(operand)){
                        errors += "0017";
                    }
                    intermediate << errors << endl;
                    programLength = locctr - startingAddress;
                    break;
                }
                else{
                    //to flag that an opcode was found
                    bool opFound = false;

                    //the increment for the location counter
                    int increment = 0;

                    //if there is a label
                    if(!label.empty()){
                        //seach for label in symbol table
                        if(symbolTable.find(label) != symbolTable.end()){
                            //duplicate symbol
                            errors += "0002";
                        }
                        //add new label into symbol table
                        else{
                            if(!isValidSymbol(label))
                                errors += "0004";
                            symbolTable.insert( std::make_pair(label, locctr) );
                        }
                    }
                    //search for opcode in optable
                    if (opcode == "WORD"){
                        int tmp;
                        //If the operand for WORD is not a decimal number then
                        //it is an invalid operand.
                        if(!Util::stringToInt(operand, tmp, 10))
                            errors += "0001";

                        //word takes 3 bytes
                        increment = 3;
                    }
                    else if (opcode == "RESW"){
                        int operandValue = -1;
                        if(Util::stringToInt(operand, operandValue, 10))
                            increment = 3 * operandValue;

                        else errors += "0001";
                    }
                    else if (opcode == "RESB"){
                        int operandValue = -1;
                        if(Util::stringToInt(operand, operandValue, 10))
                            increment = operandValue;

                        else errors += "0001";
                    }
                    else if (opcode == "BYTE"){
                        int length = getConstantLength(operand);
                        if(length != -1)
                            increment = length;

                        else errors += "0001";
                    }
                    //opcode found - not a directive
                    else if( opcodeTable.find(opcode) != opcodeTable.end() ){
                        opFound = true;
                        increment = 3;
                    }
                    //unknown opcode/unknown directive
                    else errors += "0003";

                    //write to intermediate file
                    intermediate << srcLine << endl;

                    //convert opcodes to hex if they were found
                    if(opFound) intermediate << std::hex << opcodeTable.at(opcode) << endl;
                    else        intermediate << opcode << endl;

                    //locctr is base 16 by default
                    intermediate << locctr << endl;
                    intermediate << operand << endl;
                    intermediate << errors << endl;

                    //update locctr
                    locctr += increment;
                }
            }
            source.close();
            intermediate.close();
        }

        /*
            Intermediate file structure for a block
                0-source line
                1-opcode
                2-address (locctr)
                3-operand (may be a numeric value or symbol)
                4-error list (empty means no errors)
        */
        void pass2(){
            ifstream intermediate("intermediate.txt");
            ofstream listingfile("listing.txt");
            ofstream objectfile("object.txt");

            if(!intermediate.is_open()){
                cout << "Failed to load the intermediate file!\n";
                return;
            }
            //Accumulates machine codes for a text record
            stringstream machineCodeStreamBuffer;

            bool startSet = false;
            bool endFound = false;
            bool makeNewTextRec = false;

            //read entire intermediate file
            while(true){
                string sourceLine;
                string opcode;
                string address;
                string operand;
                string errorList;

                //check if eof or some error is detected
                if(!getline(intermediate, sourceLine))
                    break;
                getline(intermediate, opcode);
                getline(intermediate, address);
                getline(intermediate, operand);
                getline(intermediate, errorList);

                //no errors yet
                if(!anyErrors)
                    //check if there are errors
                    if(!errorList.empty())
                        anyErrors = true;

                /*Find start should be the first block in the intermediate file*/

                //check for START
                if(opcode == "START"){
                    writeToListingFile(listingfile, address, "", sourceLine, errorList);

                    if(!startSet){
                        //Get the program name which is the label
                        string programName = "";
                        for(char c : sourceLine){
                            if(c != ' ')
                                programName.push_back(c);
                            else break;
                        }
                        createHeaderRecord(objectfile, programName, address, programLength);
                        startTextRecord(objectfile, address);
                    }
                    startSet = true;
                }
                else{
                    //If no Start was specified
                    if(!startSet){
                        startSet = true;

                        //Create default header - NONAME, with loading address of zero
                        createHeaderRecord(objectfile, "NONAME", "00000", programLength);
                        startTextRecord(objectfile, address);
                    }
                    //calculate current size of machine code buffer
                    machineCodeStreamBuffer.seekg(0, std::ios::end);
                    int machineBufferSize = machineCodeStreamBuffer.tellg();

                    if(opcode == "END"){
                        //save data into the object file if buffer isn't empty
                        if(machineBufferSize != 0)
                            //insert size and machine code to the current text record
                            finishTextRecord(objectfile, machineBufferSize, machineCodeStreamBuffer);

                        writeToListingFile(listingfile, "", "", sourceLine, errorList);

                        //create end record
                        createEndRecord(objectfile, startingAddress);

                        //terminate loop
                        endFound = true;
                        break;
                    }

                    /*Some other instruction besides END or START*/

                    string objectCode = "------";

                    //Produce object code if there are no errors
                    if(errorList.empty())
                        //create object code for instruction
                        objectCode = createObjectCode(opcode, operand);

                    writeToListingFile(listingfile, address, objectCode, sourceLine, errorList);

                    //calculate the number of characters in the machine code section
                    int totalMachineCodeChars = objectCode.length() + machineBufferSize;

                    //We must create a new text record since we encountered a RESW or RESB.
                    //That way, we add the correct address of the next instruction that is not
                    //a reserve.
                    if(!objectCode.empty() && makeNewTextRec){
                        startTextRecord(objectfile, address);
                        makeNewTextRec = false;
                    }
                    //Object code does not fit in text record OR if a RESW or RESB was
                    //detected (empty objectCode) which means, we must save machineCodeStreamBuffer data
                    //into the object file, if any.
                    if(objectCode.empty() || (totalMachineCodeChars > machineCodePadding)){

                        //save data into the object file if buffer isn't empty
                        if(machineBufferSize != 0){
                            //insert size and machine code to the current text record
                            finishTextRecord(objectfile, machineBufferSize, machineCodeStreamBuffer);

                            //start new record containing the address of a non-reserve instruction
                            if(!objectCode.empty())
                                startTextRecord(objectfile, address);

                            //reserve directive detected, don't save its address for the text record
                            else makeNewTextRec = true;

                            //reset machine codes buffer for the next record
                            machineCodeStreamBuffer.clear();
                            machineCodeStreamBuffer.str(std::string());
                        }
                    }
                    //Add object code machineCodesBuffer but if the objectCode is empty
                    //then do not write anything to the buffer since that signifies
                    //that there is a RESW or RESB
                    if(!objectCode.empty())
                        machineCodeStreamBuffer << objectCode;
                }
            }//while loop end

            //locctr is in bytes
            if(locctr > maxProgramSizeBytes){
                listingfile << "\nFATAL ERROR\nProgram exceeds maximum memory capacity of " << maxProgramSizeBytes;
                listingfile << " bytes\n";
                listingfile << " Last program address is: " << locctr;
                anyErrors = true;
            }

            //missing end
            if(!endFound){
                listingfile << "Error: Missing END directive\n";
                anyErrors = true;
            }

            //clean up
            intermediate.close();
            listingfile.close();
            objectfile.close();

            //delete object file if there are any errors
            if(anyErrors)
                remove("object.txt");
        }

        void displaySymbolTable(){
            cout << "Symbol Table: \n";
            unordered_map<string, unsigned>::const_iterator itr;
            for(itr = symbolTable.begin(); itr != symbolTable.end(); itr++)
                cout << itr->first << "\t" << itr->second << endl;
        }
};

#endif
