#include <iostream>
#include <fstream>
#include <filesystem>
#include <fairlogger/Logger.h>

class GeneratorEPOS4 : public o2::eventgen::GeneratorHepMC
{
    public:
        GeneratorEPOS4() = default;
        ~GeneratorEPOS4() = default;

};

// Next function takes the optns file as argument and edits the maximum number of events to be generated.
// When used as an external generator it is important that the events passed with the -n flag are the same
// or lower of the events set in the optns file, otherwise the generation will crash. That is why the .ini
// example file contains the maximum integer available, assuming less events than that are generated in a real
// life scenario. Unfortunately a larger number cannot be used at the moment since EPOS4 has a signed integer
// type for the nfull parameter. Might be changed in the future.
// When running locally, or on the GRID (not in hyperloop), the default parameters provided in the .ini file of the
// external generation can be overwritten using the confKeyValues option (or similar depending on the tool used).
FairGenerator* generateEPOS4(const std::string &name, const int& nEvents)
{
    // check if the file exists
    auto filename = gSystem->ExpandPathName(name.c_str());
    if (!std::filesystem::exists(filename))
    {
        LOG(fatal) << "Options file " << filename << " does not exist!";
        exit(1);
    }
    // cache all the lines of the optns file and replace the number of events
    std::ifstream file(filename);
    std::string line;
    bool found = false;
    std::stringstream buffer;
    while (std::getline(file, line))
    {
        if (line.find("nfull") != std::string::npos){
            // replace the number of events
            found = true;
            line = "set nfull " + std::to_string(nEvents);
        }
        buffer << line << "\n";
    }
    file.close();
    auto gen = new GeneratorEPOS4();
    auto &param0 = o2::eventgen::GeneratorFileOrCmdParam::Instance();
    auto &param = o2::eventgen::GeneratorHepMCParam::Instance();
    auto &conf = o2::conf::SimConfig::Instance();
    // Randomise seed (useful for multiple instances of the generator)
    int randomSeed = gRandom->Integer(conf.getStartSeed());
    // Write the updated content back to a file in the current directory
    std::string optnsFileName(Form("cfg%d.optns", randomSeed));
    std::ofstream outFile(optnsFileName);
    outFile << buffer.str();
    if (!found)
    {
        outFile << "set nfull " + std::to_string(nEvents);
    }
    outFile.close();
    optnsFileName = optnsFileName.substr(0, optnsFileName.find_last_of('.')); // remove the .optns extension
    // setup the HepMC generator to run with automatic FIFOs
    gen->setup(param0, param, conf);
    // Replace seed and optns file
    gen->setCmd(param0.cmd + " -i " + optnsFileName);
    gen->setSeed(randomSeed);
    return gen;
}
