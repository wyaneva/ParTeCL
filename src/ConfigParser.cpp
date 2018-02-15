/*
 * Copyright 2016 Vanya Yaneva, The University of Edinburgh
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ConfigParser.h"
#include "Constants.h"
#include <fstream>
#include <sstream>

int parseType(const std::string &line, std::istringstream &iss,
              struct Declaration &decl) {
  std::string parse;
  iss >> parse;

  // if a const, set flag and parse to the next
  decl.isConst = false;
  if (parse == "const") {
    decl.isConst = true;
    iss >> parse;
  }

  // if a pointer, set flag and consume the '*' char
  decl.isPointer = false;
  auto starChar = parse.find("*");
  if (starChar != std::string::npos) { // declared as `type* name`
    decl.isPointer = true;
    parse.erase(starChar);
  } else {     // declared as `type *name` or `type * name`
    iss.get(); // whitespace
    char next = iss.peek();
    if (next == '*') {
      decl.isPointer = true;
      iss.get(next);
    }
  }

  decl.type = parse;
  return status_constants::SUCCESS;
}

int parseTestedValueFunction(const std::string &line, std::istringstream &iss,
                             struct TestedValue &testedValue) {
  testedValue.type = TestedValueType::functionCall;
  iss >> testedValue.name;

  std::string returnType;
  iss >> returnType;
  if (returnType == config_constants::RET) {
    testedValue.resultArg = -1;
  } else if (returnType == config_constants::ARG) {
    std::string argNumStr;
    iss >> argNumStr;
    int argNum = stoi(argNumStr);
    if (argNum < 1) {
      llvm::outs() << argNum
                   << " is not a valid argument number config file line:\n"
                   << line;
      return status_constants::FAIL;
    }

    testedValue.resultArg = argNum;
  } else {
    llvm::outs() << returnType
                 << " is not a valid result in config file line:\n"
                 << line << "\nPlease enter std::RET or ARG.\n";
    return status_constants::FAIL;
  }

  return status_constants::SUCCESS;
}

int parseTestedValueVariable(std::istringstream &iss,
                             struct TestedValue &testedValue) {
  testedValue.type = TestedValueType::variable;
  iss >> testedValue.name;
  testedValue.resultArg = -1;

  return status_constants::SUCCESS;
}

int parseName(const std::string &line, std::istringstream &iss,
              struct Declaration &declaration) {
  std::string name;
  iss >> name;

  // parse potential array
  declaration.isArray = false;
  declaration.size = -1;
  // read name char by char to see if it is of the form name[array_size]
  for (auto it = name.begin(); it != name.end(); it++) {
    auto c = *it;

    // encountered the array brace or whitespace, so parse array_size if needed
    // and break
    if (c == '[' || c == ' ') {
      if (c == '[') {
        // an array
        declaration.isArray = true;

        // read the length of the array
        std::string size;
        it++;
        while (std::isdigit(*it) && it != name.end()) {
          size += *it;
          it++;
        }

        if (*it != ']') {
          llvm::outs()
              << "Array " << declaration.name
              << " is not correctly defined in line:\n"
              << line << "\nPlease, specify as   'type arrayname[arraysize]'\n";
          return status_constants::FAIL;
        } else {
          declaration.size = std::stoi(size);
        }
      }
      break;
    }

    // build the name and continue parsing
    declaration.name += c;
  }

  return status_constants::SUCCESS;
}

int parseInput(const std::string &line, std::istringstream &iss,
               std::map<int, std::string> &argvIdxToInput,
               std::list<struct Declaration> &inputDeclarations) {
  struct Declaration input;
  parseType(line, iss, input);

  // parse the input
  if (parseName(line, iss, input) == status_constants::FAIL)
    return status_constants::FAIL;

  if (!input.isArray) {
    // read the argv idx
    int argvIdx;
    iss >> argvIdx;
    argvIdxToInput[argvIdx] = input.name;
  }

  inputDeclarations.push_back(input);

  return status_constants::SUCCESS;
}

int parseStdin(const std::string &line, std::istringstream &iss,
               std::list<struct Declaration> &stdinInputs) {
  struct Declaration stdin1;
  parseType(line, iss, stdin1);

  if (parseName(line, iss, stdin1) == status_constants::FAIL)
    return status_constants::FAIL;

  // add to list of inputs
  stdinInputs.push_back(stdin1);

  return status_constants::SUCCESS;
}

int parseResult(const std::string &line, std::istringstream &iss,
                std::list<struct ResultDeclaration> &resultDeclarations) {
  struct ResultDeclaration result;
  parseType(line, iss, result.declaration);

  if (parseName(line, iss, result.declaration) == status_constants::FAIL)
    return status_constants::FAIL;

  std::string annot;
  iss >> annot;

  // parse the tested value as a function call
  if (annot == config_constants::FUNCTION) {
    if (parseTestedValueFunction(line, iss, result.testedValue) ==
        status_constants::FAIL)
      return status_constants::FAIL;
  }

  // parse the tested value as a variable
  else if (annot == config_constants::VARIABLE) {
    if (parseTestedValueVariable(iss, result.testedValue) ==
        status_constants::FAIL)
      return status_constants::FAIL;
  }

  else {
    llvm::outs() << "I don't know how to parse annotation " << annot
                 << " on line: " << line;
    return status_constants::FAIL;
  }

  resultDeclarations.push_back(result);

  return status_constants::SUCCESS;
}

int parseInclude(std::istringstream &iss, std::list<std::string> &includes) {
  std::string include;
  iss >> include;
  includes.push_back(include);

  return status_constants::SUCCESS;
}

// it parses the configuration file and returns all the necessary data
int parseConfig(const std::string &configFilename,
                std::map<int, std::string> &argvIdxToInput,
                std::list<struct Declaration> &stdinInputs,
                std::list<struct Declaration> &inputDeclarations,
                std::list<struct ResultDeclaration> &resultDeclarations,
                std::list<std::string> &includes) {
  llvm::outs() << "Parsing configuration file... ";

  // open the file
  std::ifstream infile(configFilename);
  std::string line;
  if (!infile.is_open()) {
    llvm::outs() << "Error opening config file: " << configFilename << ".\n";
    return status_constants::FAIL;
  }

  // parse line by line
  while (getline(infile, line)) {
    std::istringstream iss(line);
    std::string annot;
    iss >> annot;

    // parse the inputs
    if (annot == config_constants::INPUT) {
      if (parseInput(line, iss, argvIdxToInput, inputDeclarations) ==
          status_constants::FAIL)
        return status_constants::FAIL;
    }

    // parse stdin inputs
    else if (annot == config_constants::STDIN) {
      if (parseStdin(line, iss, stdinInputs) == status_constants::FAIL)
        return status_constants::FAIL;
    }

    // parse results
    else if (annot == config_constants::RESULT) {
      if (parseResult(line, iss, resultDeclarations) == status_constants::FAIL)
        return status_constants::FAIL;
    }

    // parse includes
    else if (annot == config_constants::INCLUDE) {
      if (parseInclude(iss, includes) == status_constants::FAIL)
        return status_constants::FAIL;
    }

    else {
      llvm::outs() << "I don't know how to parse annotation " << annot
                   << " on line: " << line;
      return status_constants::FAIL;
    }
  }

  llvm::outs() << "DONE!\n";
  return status_constants::SUCCESS;
}
