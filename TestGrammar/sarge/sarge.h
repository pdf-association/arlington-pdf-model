///
/// @file
/// @brief Interface for the Sarge command line argument parser project.
///
/// @version 1
///
/// @remark Originally from: https://github.com/MayaPosch/Sarge
///
/// @author 2019/03/16, Maya Posch
/// @author 2021/06, Peter Wyatt - modified for Level 4 warning free compilation
///

#include <string>
#include <vector>
#include <map>
#include <memory>


struct Argument {
    Argument() : hasValue(false), parsed(false) { /* */ }
    std::string arg_short;
    std::string arg_long;
    std::string description;
    bool hasValue;
    std::string value;
    bool parsed;
};



class Sarge {
    std::vector<std::unique_ptr<Argument> > args;
    std::map<std::string, Argument*> argNames;
    bool parsed = false;
    int flagCounter = 0;
    std::string execName;
    std::string description;
    std::string usage;
    std::vector<std::string> textArguments;

public:
    void setArgument(std::string arg_short, std::string arg_long, std::string desc, bool hasVal);
    void setArguments(std::vector<Argument> vargs);
    void setDescription(std::string desc) { this->description = desc; }
    void setUsage(std::string use) { this->usage = use; }
    bool parseArguments(int argc, char** argv);
    bool getFlag(std::string arg_flag, std::string &arg_value);
    bool exists(std::string arg_flag);
    bool getTextArgument(uint32_t index, std::string &value);
    void printHelp();
    int flagCount() { return flagCounter; }
    std::string executableName() { return execName; }
};
