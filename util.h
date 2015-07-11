/*
    This class holds generic/utilitarian functions that
    can be used whenever needed such as parsing a line with
    the specified delimeters, or converting a string to an integer.
*/

#ifndef UTIL_H
#define UTIL_H

#include "dynamic_array.h"
#include <string>
#include <cmath>
using std::string;

class Util{
    public:
        //parses line into tokens separated by the delimiters
        //and saves each individual token into the destination array
        static void parseLine(DynamicArray<string>& dst, const string& line, string delims){
            //if there is something to parse
            if(line.length() != 0){
                string token = "";

                //iterate through every character in line
                unsigned end = line.length()-1;
                for(unsigned i = 0; i <= end; i++){

                    char c = line[i];

                    //find delimeters
                    bool delimFound = Util::findChar(delims, c);
                    bool nextDelimFound = Util::findChar(delims, line[i+1]);

                    //add valid characters
                    if(!delimFound)
                        token.push_back(c);

                    //end of token
                    if(!token.empty())
                        if((i < end && !delimFound && nextDelimFound) || i == end){
                            //add it to destination array
                            dst.push_back(token);
                            //reset token for next token extraction
                            token = "";
                        }
                }
            }
        }

        //Checks if string a is a prefix of string b.
        //For example "assem" is a prefix of "assemble"
        //string a must be smaller or equal to string b
        static bool isPrefix(const string& a, const string& b){
            if(a.length() > b.length())
                return false;

            for(unsigned i = 0; i < a.length(); i++)
                //mismatch
                if(a[i] != b[i])
                    return false;

            return true;
        }

        static bool findChar(const string& src, char c){
            for(unsigned i = 0; i < src.length(); i++)
                if(src[i] == c)
                    return true;
            return false;
        }

        static void toUpperCase(string& src){
            for(size_t i = 0; i < src.length(); ++i){
                char c = src[i];
                if(isLowerCase(c))
                    src[i] = c - 32;
            }
        }

        static bool isLowerCase(char c){
            return c >= 'a' && c <= 'z';
        }

        static bool isUpperCase(char c){
            return c >= 'A' && c <= 'Z';
        }

        static bool isDigit(char c){
            return c >= '0' && c <= '9';
        }

        static bool isAlpha(char c){
            return isLowerCase(c) || isUpperCase(c);
        }

        static bool isAlphaNumeric(char c){
            return isAlpha(c) || isDigit(c);
        }

        //Takes string and converts it to an integer depending on the specified base
        static bool stringToInt(const string& src, int& dst, int base){
            //nothing to process is an error
            if(src.empty())
                return false;

            int sum = 0;
            for(unsigned i = 0; i < src.length(); i++){
                char c = src[i];

                //convert letters to upper case
                if(isLowerCase(c))
                    c -= 32;

                if(isAlphaNumeric(c)){

                    //invalid letter digits for bases smaller than 10
                    if(base <= 10 && !isDigit(c))
                        return false;

                    //an invalid hex digit
                    if(base == 16 && !isHexDigit(c))
                        return false;

                    int digit = 0;
                    if(isDigit(c))
                        digit = (int)(c - '0');
                    //must be a letter
                    else
                        digit = (int)(c - 'A' + 10);

                    sum += digit * (int)pow(base, src.length()-1 - i);
                }
                else return false;
            }
            dst = sum;
            return true;
        }

        static bool hasHexFormat(const string& hex){
            for(unsigned i = 0; i < hex.length(); i++){
                char c = hex[i];

                //a letter
                if(isAlphaNumeric(c)){
                    //invalid letters for hex
                    if(isAlpha(c) && (c < 'A' || c > 'F'))
                        return false;
                }
                else return false;
            }
            //contains numbers and letters A to F , inclusive.
            return true;
        }

        //Test if a character is a valid hexadecimal digit
        static bool isHexDigit(char c){
            //convert to upper case if needed
            if(isLowerCase(c))
                c -= 32;

            //must be a digit or a letter between A through F, inclusive
            return isDigit(c) || (c >= 'A' && c <= 'F');
        }
};

#endif
