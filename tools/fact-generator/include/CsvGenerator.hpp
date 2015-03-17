#ifndef CSV_GENERATOR_H__
#define CSV_GENERATOR_H__

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/filesystem/fstream.hpp>
#include <string>

#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"

#include "AuxiliaryMethods.hpp"
#include "PredicateNames.hpp"
#include "DirInfo.hpp"
#include "Singleton.hpp"

class CsvGenerator : public Singleton<CsvGenerator> {
public:

    void writeEntityToCsv(const char *filename, const std::string& entityRefmode);

    void writeOperandPredicateToCsv(const char *predName, const std::string& entityRefmode, 
                                    const std::string& operandRefmode,
                                    bool operandType, int index = -1);

    template<class ValType>
    void writePredicateToCsv(const char *predName, const std::string& entityRefmode, 
                             const ValType& valueRefmode, int index = -1){
        boost::filesystem::ofstream *csvFile = getCsvFile(predNameToFilename(predName));
        if(index == -1)
            (*csvFile) << entityRefmode << delim << valueRefmode << "\n";
        else
            (*csvFile) << entityRefmode << delim << index << delim << valueRefmode << "\n";
    }

    void processModule(const llvm::Module *Mod, std::string& path);

    void writeVarsTypesAndImmediates();

    static void setDelim(const char newDelim){
        delim = newDelim;
    }

protected:

    friend class Singleton<CsvGenerator>;

    CsvGenerator();
    CsvGenerator(const CsvGenerator&);
    CsvGenerator& operator= (const CsvGenerator&);

    ~CsvGenerator(){
        for(boost::unordered_map<std::string, boost::filesystem::ofstream*>::iterator it = csvFiles.begin(),
                end = csvFiles.end(); it != end; it++){
            boost::filesystem::ofstream *file = it->second;
            file->flush();
            file->close();
            delete file;
        }
    }

private:
    //auxiliary methods

    void identifyType(const llvm::Type *elementType, boost::unordered_set<const llvm::Type *> &componentTypes);

    void identifyStructType(const llvm::Type *structType, boost::unordered_set<const llvm::Type *> &componentTypes);

    void identifyFunctionType(const llvm::Type *funcType, boost::unordered_set<const llvm::Type *> &componentTypes);

    const char *writeLinkage(llvm::GlobalValue::LinkageTypes LT);

    const char *writeVisibility(llvm::GlobalValue::VisibilityTypes Vis);

    void writeGlobalVar(const llvm::GlobalVariable *gv, std::string globalName);

    void writeGlobalAlias(const llvm::GlobalAlias *ga, std::string globalAlias);

    std::string getRefmodeForValue(const llvm::Module * Mod, const llvm::Value * Val, std::string& path){
        return "<" + path + ">:" + auxiliary_methods::valueToString(Val, Mod);
    }

    std::string predNameToFilename(const char * predName) {
        boost::unordered_map<const char*, std::string>::iterator cachedValue = simplePredFilenames.find(predName);
        if(cachedValue != simplePredFilenames.end()){
            return cachedValue->second;
        }

        DirInfo * info = DirInfo::getInstance();
        std::string filename = std::string(predName);
        std::string folder = info->getEntitiesDir();
        size_t pos = 0;
        while ((pos = filename.find(':', pos)) != std::string::npos) {
            filename[pos] = '-';
            folder = info->getPredicatesDir();
        }
        filename = folder + "/" + filename + ".dlm";
        simplePredFilenames[predName] = filename;
        return filename;
    }

    std::string predNameWithOperandToFilename(const char * predName, bool operand) {
        //TODO: yikes make a typedef
        boost::unordered_map<const char*,
                             std::pair<std::string, std::string> >::iterator
            cachedValue = operandPredFilenames.find(predName);

        if(cachedValue != operandPredFilenames.end()){
            if(operand)
                return cachedValue->second.first;
            else
                return cachedValue->second.second;
        }

        std::string filename = predName;
        size_t pos = 0;
        while ((pos = filename.find(':', pos)) != std::string::npos) {
            filename[pos] = '-';
        }
        // imm: operand = 0, var: operand = 1
        filename = DirInfo::getInstance()->getPredicatesDir() + "/" + filename;

        std::string varVersion = filename + "-by_variable.dlm", immVersion = filename + "-by_immediate.dlm";
        operandPredFilenames[predName] = std::pair<std::string, std::string>(varVersion, immVersion);
        if(operand)
            return varVersion;
        else
            return immVersion;
    }

    boost::filesystem::ofstream* getCsvFile(std::string filename){
        boost::filesystem::ofstream *rv;
        if(csvFiles.find(filename) == csvFiles.end())
            csvFiles[filename] = new boost::filesystem::ofstream(filename.c_str(), std::ios_base::out);
        return csvFiles[filename];
    }

    boost::unordered_set<const llvm::DataLayout *> layouts;
    boost::unordered_set<const llvm::Type *> types;
    boost::unordered_set<const llvm::Type *> componentTypes;
    boost::unordered_map<std::string, const llvm::Type *> variable;
    boost::unordered_map<std::string, const llvm::Type *> immediate;
    boost::unordered_map<std::string, boost::filesystem::ofstream*> csvFiles;
    boost::unordered_map<const char*, std::string> simplePredFilenames;
    boost::unordered_map<const char*, std::pair<std::string, std::string> > operandPredFilenames;

    static char delim;
    static CsvGenerator * INSTANCE;
    static const int simplePredicatesNum;
    static const char * simplePredicates[];
    static const int operandPredicatesNum;
    static const char * operandPredicates[];
};

#endif
