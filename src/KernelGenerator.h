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

#ifndef KERNEL_GENERATOR_H
#define KERNEL_GENERATOR_H

#include "Utils.h"
#include "clang/Tooling/Tooling.h"
#include <string>

void generateKernel(clang::tooling::ClangTool *, std::string,
                    std::map<int, std::string>, std::list<struct Declaration>,
                    std::list<struct Declaration>,
                    std::list<struct OutputDeclaration>);

#endif
